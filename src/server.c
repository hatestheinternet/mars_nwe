#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mars/config.h>
#include <mars/server.h>
#include <mars/ncp.h>
#include <mars/bindery.h>

mars_server_t *_mars_servers = NULL;
mars_server_t _mars_server;

int mars_server_has_volume(char *name) {
    mars_server_t *srv = _mars_servers;
    
    while( srv ) {
        if( srv->type == MARS_SERVER_TYPE_FILE ) {
            for( int i=0; i<MARS_SERVER_MAX_VOLS; i++ ) {
                if( srv->volumes[i] && strcmp(srv->volumes[i]->name, name) == 0 ) {
                    return 1;
                }
            }
        }

        srv = srv->next;
    }

    return 0;
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

    srv->volumes[idx++] = vol;
    vol->idx = idx;

    pthread_mutex_unlock(&srv->vol_mtx);
    return idx;
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

    // TODO Check if path exists

    if( !mars_server_add_volume(&_mars_server, vol) ) {
        free(vol);
        return 0;
    }

    return 1;
}

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

    tst = mars_config_get_all();
    while( tst ) {
        // Are we going to be a file server?
        if( strcmp(tst->name, "volume") == 0 ) {
            name = mars_config_str(tst, "name");
            if( name == NULL ) {
                fprintf(stderr, "mars_server_init: Volume has no \"name\"\n");
            } else if( mars_server_has_volume(name) ) {
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
            }
        }

        tst = tst->next;
    }

    return ret;
}

int mars_server_am_a(uint16_t type) {
    return 1;
}
