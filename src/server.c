#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/server.h>
#include <mars/ncp.h>
#include <mars/nds.h>
#include <mars/bindery.h>

mars_server_t _mars_server;

#pragma region Function 0x14 - Get server date and time

typedef struct mars_ncp_service_date_time_response_t {
    mars_ncp_response_t resp;

    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t dow;
    uint8_t twenty;
} mars_ncp_service_date_time_response_t;

int mars_ncp_service_date_time(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_service_date_time_response_t resp;
    mars_ncp_response_prepare(conn, &resp, sizeof(mars_ncp_service_date_time_response_t));

    struct tm *tm;
    
    time_t tt = time(0);
    tm = localtime(&tt);

    resp.year = tm->tm_year;
    resp.month = tm->tm_mon+1;
    resp.day = tm->tm_mday;
    resp.hour = tm->tm_hour;
    resp.minutes = tm->tm_min;
    resp.seconds = tm->tm_sec;
    resp.dow = tm->tm_wday;

    return mars_ncp_send(srv, &resp, sizeof(mars_ncp_service_date_time_response_t), (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
}

#pragma endregion

#pragma region Function 0x17 - Get Server Info

typedef struct mars_ncp_service_fserv_info_request_t {
    uint16_t packet_len;
    uint8_t sub_function;
} mars_ncp_service_fserv_info_request_t;

typedef struct mars_ncp_service_fserv_info_response_t {
    mars_ncp_response_t resp;
    uint8_t server[48];
    uint8_t major;
    uint8_t minor;
    uint16_t max_conns __attribute__ ((packed));
    uint16_t conns_in_use __attribute__ ((packed));
    uint16_t max_volumes __attribute__ ((packed));
    uint8_t os_rev;
    uint8_t sft;
    uint8_t tts;
    uint16_t conn_max_used __attribute__ ((packed));
    uint8_t ver_acct;
    uint8_t ver_vap;
    uint8_t ver_qms;
    uint8_t ver_print;
    uint8_t ver_vcons;
    uint8_t ver_secres;
    uint8_t ver_ibridge;
    uint8_t mix_mode_path;
    uint8_t local_login_info;
    uint16_t product_major __attribute__ ((packed));
    uint16_t product_minor __attribute__ ((packed));
    uint16_t product_rev __attribute__ ((packed));
    uint8_t os_lang_id;
    uint8_t sixtyfour;
    uint8_t stype;
    uint8_t ktype;
    uint8_t reserved[48];
} mars_ncp_service_fserv_info_response_t;

int mars_ncp_service_fserv_info(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_service_fserv_info_request_t *req = (mars_ncp_service_fserv_info_request_t *)buff;

    mars_ncp_service_fserv_info_response_t resp;
    
    switch( req->sub_function ) {
        case 0x11:
            mars_ncp_response_prepare(conn, &resp, sizeof(resp));

            // Defaults based on how my Netware 4.2 server responds
            resp.major = 4;
            resp.minor = 11;
            resp.max_conns = htons(7U);
            resp.max_volumes = htons(MARS_SERVER_MAX_VOLS);
            resp.sft = 0x2;
            resp.tts = 1;
            resp.conn_max_used = 1;
            resp.ver_acct = 1;
            resp.ver_qms = 1;
            resp.ver_vap = 1;
            resp.ver_vcons = 1;
            resp.ver_secres = 1;
            resp.ver_ibridge = 1;
            memcpy(resp.server, mars_config_server_name(), 48);

            if( mars_config_is_true(mars_config_global_str("dump_ncp")) ) {
                printf("mars_ncp_service_fserv_info[%i]: " MARS_PRINTF_IPX_ADDR "@%08X requested file server info\n", conn->idx, MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network));
            }
            mars_ncp_send(srv, &resp, sizeof(mars_ncp_service_fserv_info_response_t), (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
            break;

        default:
            printf("Packet Len = %hu, Subfunc = %02X\n", ntohs(req->packet_len), req->sub_function);
            break;
    }

    return 1;
}

#pragma endregion

#pragma region Directory Entry Management

int _mars_server_dirent_next_id(mars_server_volume_t *vol) {
    mars_server_volume_dirent_t *tst = vol->dirents;
    uint32_t max_id = 0;

    if( !tst )
        return 0;

    while( tst ) {
        if( tst->handle > max_id )
            max_id = tst->handle;
        tst = tst->next;
    }

    return max_id+1;
}

mars_server_volume_dirent_t *_mars_server_dirent_create(mars_server_volume_t *vol, char *name, mars_server_volume_dirent_t *parent) {
    mars_server_volume_dirent_t *dirent = calloc(1,sizeof(mars_server_volume_dirent_t));
    dirent->name = strdup(name);

    int path_sz = 0;
    if( parent ) {
        path_sz += strlen(parent->netware_path) + 1;
        dirent->parent = parent;
    }

    path_sz += strlen(name)+1+strlen(vol->name);
    dirent->netware_path = calloc(1,path_sz);

    char *pos = dirent->netware_path;
    memcpy(pos, vol->name, strlen(vol->name));
    pos += strlen(vol->name);
    *pos = '/';
    pos++;

    if( parent ) {
        memcpy(pos, parent->netware_path, strlen(parent->netware_path));
        pos += strlen(parent->netware_path) +1;
        *pos = '/';
        pos++;
    }

    snprintf(pos, path_sz-(pos-dirent->netware_path)+1, "%s", name);
    dirent->handle = _mars_server_dirent_next_id(vol);
    
    dirent->next = vol->dirents;
    vol->dirents = dirent;

    return dirent;
}

mars_server_volume_dirent_t *_mars_server_dirent_get_or_create(mars_server_volume_t *vol, char *name, mars_server_volume_dirent_t *parent) {
    mars_server_volume_dirent_t *dirent = NULL, *tst;

    tst = vol->dirents;
    while( tst ) {
        dirent = tst;
        tst = dirent->next;

        if( dirent->parent == parent && strcmp(dirent->name, name) == 0 )
            return dirent;
    }

    return _mars_server_dirent_create(vol, name, parent);
}

mars_server_volume_dirent_t *mars_server_dirent_get(mars_server_volume_t *vol, int directory) {
    mars_server_volume_dirent_t *ret = NULL;
    mars_server_volume_dirent_t *dirent = vol->dirents;

    while( dirent ) {
        if( dirent->handle == directory) {
            return dirent;
        }
        dirent = dirent->next;
    }

    return ret;
}

mars_server_volume_dirent_t *mars_server_dirent_walk(mars_server_volume_t *vol, char *path) {
    if( path == NULL ) {
        return _mars_server_dirent_get_or_create(vol, vol->name, NULL);
    }
    char *dat = strdup(path), *pos=dat+strlen(path)-strlen(vol->name), *ptr;
    mars_server_volume_dirent_t *parent = NULL;
    printf("Walk %s\n", path);

    if( path != NULL ) {
        ptr = strchr(pos,'/');
        while( ptr ) {
            *ptr = 0;
            printf("Parent %s\n", pos);

            parent = _mars_server_dirent_get_or_create(vol, pos, parent);
            pos = ptr+1;


            ptr = strchr(pos,'/');
        }

        parent = _mars_server_dirent_get_or_create(vol, dat, parent);
    }

    free(dat);
    return parent;
}

#pragma endregion

#pragma region Volume Management

mars_server_volume_t *mars_server_find_volume(char *name) {
    mars_server_volume_t *ret;

    for( int i=0; i<MARS_SERVER_MAX_VOLS; i++ ) {
        ret = _mars_server.volumes[i];

        if( ret && strcmp(ret->name, name) == 0 )
            return ret;
    }

    return NULL;
}

mars_server_volume_t *mars_server_get_volume(int idx) {
    return _mars_server.volumes[idx];
}

int mars_server_add_volume(mars_server_t *srv, mars_server_volume_t *vol) {
    int idx;

    pthread_mutex_lock(&srv->vol_mtx);

    for( idx=0; idx<MARS_SERVER_MAX_VOLS; idx++ ) {
        if( !srv->volumes[idx] ) {
            break;
        }
    }

    if( idx >= MARS_SERVER_MAX_VOLS ) {
        pthread_mutex_unlock(&srv->vol_mtx);
        fprintf(stderr,"mars_server_add_volume: Maximum number of volumes (%i) reached!\n", MARS_SERVER_MAX_VOLS);
        return -1;
    }

    vol->idx = idx;
    srv->volumes[idx] = vol;
    _mars_server_dirent_get_or_create(vol, "", NULL);
    pthread_mutex_unlock(&srv->vol_mtx);
    return idx;
}

mars_server_volume_t *mars_server_add_volume_ex(mars_server_t *srv, char *name, char *path) {
    mars_server_volume_t *vol = calloc(1,sizeof(mars_server_volume_t));
    vol->name = name;
    vol->path = path;
    vol->is_system = 1;

    if( mars_server_add_volume(srv, vol) < 0 ) {
        free(vol);
        return NULL;
    }

    return vol;
}

int mars_server_config_volume(mars_config_section_t *cfg) {
    mars_server_volume_t *vol = calloc(1,sizeof(mars_server_volume_t));
    if( !vol ) {
        fprintf(stderr, "mars_server_config_volume: Failed to allocate memory for %s\n", mars_config_str(cfg,"name"));
        return 0;
    }

    vol->name = mars_config_str(cfg, "name");
    if( !vol->name ) {
        fprintf(stderr, "mars_server_config_volume: Section has no name=\n");
        free(vol);
        return 0;
    }

    vol->path = mars_config_str(cfg, "path");
    if( !vol->path ) {
        fprintf(stderr, "mars_server_config_volume: %s has no path=\n", vol->name);
        free(vol);
        return 0;
    }

    vol->is_system = mars_config_is_true(mars_config_str(cfg, "is_system"));

    // TODO Check if path exists

    if( mars_server_add_volume(&_mars_server, vol) < 0 ) {
        free(vol);
        return 0;
    }

    return 1;
}

#pragma endregion

#pragma region Lifecycle

int mars_server_start(void) {
    if( !mars_ncp_start(&_mars_server) )
        return 0;

    return 1;
}

void mars_server_stop(void) {
    
    if( _mars_server.running ) {
        _mars_server.should_run = 0;
        printf("mars_server_stop: Waiting for file server to stop\n");
        pthread_join(_mars_server.thread, NULL);
    }

    if( _mars_server.fd )
        close(_mars_server.fd);

    for( int i=0; i<MARS_SERVER_MAX_CONN; i++ ) {
        if( _mars_server.volumes[i] ) {
            free(_mars_server.volumes[i]);
            _mars_server.volumes[i] = NULL;
        }
    }

    if( _mars_server.destroy ) {
        _mars_server.destroy(&_mars_server);
    }
}

int mars_server_init(void) {
    int ret = 0;
    mars_config_section_t *tst = mars_config_get_all();
    char *name;

    memset(&_mars_server, 0, sizeof(mars_server_t));

    mars_server_volume_t *sysvol = mars_server_add_volume_ex(&_mars_server, "SYS", MARS_SERVER_SYS_PATH);
    sysvol->is_system = 1;

    tst = mars_config_get_all();
    while( tst ) {
        // Are we going to be a file server?
        if( strcmp(tst->name, "volume") == 0 ) {
            name = mars_config_str(tst, "name");
            if( name == NULL ) {
                fprintf(stderr, "mars_server_init: Volume has no \"name\"\n");
            } else if( mars_server_find_volume(name) ) {
                fprintf(stderr,"mars_server_init: Duplicate volume name \"%s\"\n", name);
            } else {
                ret = mars_server_config_volume(tst);
                if( ret ) {
                    printf("mars_server_init: Added volume %i: \"%s\"\n", ret, name);
                }
            }
            
        // Maybe a directory server?
        } else if( strcmp(tst->name, "bindery") == 0 ) {
            if( _mars_server.bindery ) {
                ret = 0;
                fprintf(stderr,"mars_server_init: Ignoring duplicate [bindery] section\n");
                
            } else {
                mars_server_t *res = mars_bindery_init(&_mars_server, tst);
                if( !res ) {
                    fprintf(stderr,"mars_server_init: Failed to initialize bindery\n");
                    ret = 0;
                    break;
                }
                ret = 1;
            }
        }

        tst = tst->next;
    }

    return ret;
}

#pragma endregion

