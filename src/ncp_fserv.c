#include <stdio.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>
#include <sys/stat.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/ncp.h>

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

#pragma region Function 0x57 - File or Directory Info

typedef enum mars_ncp_entry_search_e {
    NCP_SEARCH_ALLOW_HIDDEN = 2,
    NCP_SEARCH_WANT_SYSTEM = 4,
    NCP_SEARCH_WANT_EXE = 8,
    NCP_SEARCH_WANT_DIR = 16
} mars_ncp_entry_search_e;

typedef struct mars_ncp_service_entry_request_t {
    mars_ncp_service_request_t req;
    uint8_t sub_func;
    uint8_t src_ns;
    uint8_t dest_ns;
    uint16_t search_attr __attribute__ ((packed));
    uint16_t ret_info __attribute__ ((packed));
    uint16_t ret_info_ex __attribute__ ((packed));
    uint8_t vol_no;
    uint32_t dir_base __attribute__ ((packed));
    uint8_t handle_flag;
    uint8_t path_count;
} mars_ncp_service_entry_request_t;

typedef struct mars_ncp_service_entry_reponse_t {
    mars_ncp_response_t *resp;
    uint8_t reserved_head[4];
    uint32_t attributes __attribute__ ((packed));
    uint16_t attr_flags __attribute__ ((packed));
    uint8_t reserved_mid[38];
} mars_ncp_service_entry_reponse_t;

int mars_ncp_service_entry_info(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_service_entry_request_t *req = (mars_ncp_service_entry_request_t *)buff;
    
    switch( req->sub_func ) {
        case 6:
            uint8_t vol_name[32], path[1024];    
            uint8_t *ptr = buff + sizeof(mars_ncp_service_entry_request_t);
            unsigned char *ppos = path;
            
            memset(&vol_name, 0, sizeof(vol_name));
            memset(&path, 0, sizeof(path));

            uint8_t pe_sz = *ptr;
            ptr++;

            memcpy(&vol_name, ptr, pe_sz);
            ptr += pe_sz;

            mars_server_volume_t *vol = mars_server_find_volume((char *)vol_name);
            if( !vol ) {
                fprintf(stderr, "mars_ncp_service_entry_info[%i]: " MARS_PRINTF_IPX_ADDR "@%08X requested non-existent volume \"%s\"\n", conn->idx, MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network), vol_name);
                break;
            }

            // TODO Not found?
            if( (req->search_attr & NCP_SEARCH_WANT_SYSTEM) && !vol->is_system ) {
                fprintf(stderr, "mars_ncp_service_entry_info[%i]: " MARS_PRINTF_IPX_ADDR "@%08X \"%s\" is not a system volume\n", conn->idx, MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network), vol_name);
                break;
            }

            memcpy(ppos, vol->path, strlen(vol->path));
            ppos += strlen(vol->path);

            if( req->path_count > 1 ) {
                for( int i=0; i<req->path_count-1; i++ ) {
                    *ppos = '/';
                    ppos++;
                    
                    pe_sz = *ptr;
                    memcpy(ppos, ptr+1, pe_sz);
                    ptr += pe_sz + 1;
                }
            }

            struct stat finfo;
            if( stat((char *)path, &finfo) == -1 ) {
                break;
            }
            
            if( (req->search_attr & NCP_SEARCH_WANT_DIR) && !S_ISDIR(finfo.st_mode) ) {
                printf("want directory but %s is not\n", path);
                break;
            }
            
            uint8_t dat[4096];
            mars_ncp_service_entry_reponse_t *resp = (mars_ncp_service_entry_reponse_t *)dat;
            mars_ncp_response_prepare(conn, resp, sizeof(dat));

            resp->attr_flags = 4;

            if( S_ISDIR(finfo.st_mode) ) {
                resp->attributes += 16;
            }

            ptr = dat;
            ppos = ptr + sizeof(mars_ncp_service_entry_reponse_t);

            // Directory entry number 12
            *ppos = 12;
            ppos += 4;

            // DOS directory 1?
            *ppos = 1;
            ppos += 4;

            *ppos = vol->idx & 0xFF;
            ppos++;

            size_t send_sz = (ppos - ptr) + 20;
            return mars_ncp_send(srv, ptr, send_sz, (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));

            break;
    }

    return 1;
}

#pragma endregion