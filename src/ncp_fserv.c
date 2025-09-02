#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>
#include <sys/stat.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/ncp.h>

typedef enum mars_ncp_entry_search_e {
    NCP_SEARCH_ALLOW_HIDDEN = 2,
    NCP_SEARCH_WANT_SYSTEM = 4,
    NCP_SEARCH_WANT_EXE = 8,
    NCP_SEARCH_WANT_DIR = 16
} mars_ncp_entry_search_e;

#pragma region Function 0x57:0x06 - File or sub directory info

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

int _mars_ncp_service_entry_info_for(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    uint8_t vol_name[32], path[1024];    
    mars_ncp_service_entry_request_t *req = (mars_ncp_service_entry_request_t *)buff;
    mars_server_volume_t *vol;
    mars_server_volume_dirent_t *dirent;
    uint8_t *ptr;
    ptr = buff + sizeof(mars_ncp_service_entry_request_t);
    unsigned char *ppos = path;
    
    memset(&vol_name, 0, sizeof(vol_name));
    memset(&path, 0, sizeof(path));

    uint8_t pe_sz = *ptr;
    ptr++;

    memcpy(&vol_name, ptr, pe_sz);
    ptr += pe_sz;

    vol = mars_server_find_volume((char *)vol_name);
    if( !vol ) {
        fprintf(stderr, "mars_ncp_service_entry_info[%i]: " MARS_PRINTF_IPX_ADDR "@%08X requested non-existent volume \"%s\"\n", conn->idx, MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network), vol_name);
        return -1;
    }

    // TODO Not found?
    if( (req->search_attr & NCP_SEARCH_WANT_SYSTEM) && !vol->is_system ) {
        fprintf(stderr, "mars_ncp_service_entry_info[%i]: " MARS_PRINTF_IPX_ADDR "@%08X \"%s\" is not a system volume\n", conn->idx, MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network), vol_name);
        return -1;
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
        return -1;
    }
    
    if( (req->search_attr & NCP_SEARCH_WANT_DIR) && !S_ISDIR(finfo.st_mode) ) {
        printf("want directory but %s is not\n", path);
        return -1;
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

    char *walk = (char *)path;
    walk += strlen(vol->path) + 1;
    dirent = mars_server_dirent_walk(vol, walk);
    
    // Directory entry number 12
    *ppos = dirent->handle & 0xFF;
    *(ppos+1) = (dirent->handle >> 8) & 0xFF;
    *(ppos+2) = (dirent->handle >> 16) & 0xFF;
    *(ppos+3) = (dirent->handle >> 24) & 0xFF;
    // *ppos = 12;
    ppos += 4;

    // DOS directory 1?
    *ppos = 1;
    ppos += 4;

    *ppos = vol->idx & 0xFF;
    *(ppos+1) = (vol->idx >> 8) & 0xFF;
    *(ppos+2) = (vol->idx >> 16) & 0xFF;
    *(ppos+3) = (vol->idx >> 24) & 0xFF;
    ppos+=4;

    size_t send_sz = (ppos - ptr) + 20;
    return mars_ncp_send(srv, ptr, send_sz, (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
}

#pragma endregion

#pragma region Function 0x57:0x1C - Get full path string

typedef struct mars_ncp_service_path_request_t {
    mars_ncp_service_request_t req;
    uint8_t sub_func;
    uint8_t ns_source;
    uint8_t ns_dest;
    uint16_t cookie_flags __attribute__ ((packed));
    uint32_t cookie_one __attribute__ ((packed));
    uint32_t cookie_two __attribute__ ((packed));
    uint8_t vol_no;
    uint32_t dir_base __attribute__ ((packed));
    uint8_t dir_handle;
    uint8_t path_count;
} mars_ncp_service_path_request_t;

typedef struct mars_ncp_service_path_response_t {
    mars_ncp_response_t resp;
    uint16_t path_flags;
    uint32_t cookie_one __attribute__ ((packed));
    uint32_t cookie_two __attribute__ ((packed));
    uint16_t path_sz __attribute__ ((packed));
    uint16_t path_count __attribute__ ((packed));
} mars_ncp_service_path_response_t;

int _mars_ncp_service_entry_info_path(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    uint8_t rdat[8192], wpath[1024];
    uint8_t *ptr;
    char *tmp, *cptr;

    mars_server_volume_t *vol;
    mars_server_volume_dirent_t *dirent;

    mars_ncp_service_path_request_t *preq = (mars_ncp_service_path_request_t *)buff;
    mars_ncp_service_path_response_t *presp = (mars_ncp_service_path_response_t *)rdat;
    mars_ncp_response_prepare(conn, presp, sizeof(rdat));

    memset(&wpath, 0, sizeof(wpath));

    vol = mars_server_get_volume(preq->vol_no);
    if( vol == NULL ) {
        fprintf(stderr, "Request for invalid volume %i\n", preq->vol_no);
        return -1;
    }
    
    dirent = mars_server_dirent_get(vol, preq->dir_base);
    if( dirent == NULL ) {
        fprintf(stderr, "Request for invalid dirent %04X\n", preq->dir_base);
        return -1;
    }
    ptr = wpath;

    memcpy(ptr, vol->name, strlen(vol->name));
    ptr += strlen(vol->name);
    *ptr = '/';
    ptr++;

    presp->cookie_one = 0U;
    presp->cookie_two = preq->cookie_two;

    presp->path_count = 0;
    presp->path_sz = 0U;

    ptr = (uint8_t *)presp + sizeof(mars_ncp_service_path_response_t);

    tmp = dirent->netware_path;
    cptr = strrchr(tmp, '/');
    while( cptr ) {
        *cptr = 0;

        presp->path_sz += strlen(cptr+1) + 1;
        presp->path_count++;

        *ptr = strlen(cptr+1) & 0xFF;
        memcpy(ptr+1, cptr+1, strlen(cptr+1)&0xFF);
        ptr += (strlen(cptr+1)&0xFF) + 1;

        cptr = strrchr(tmp, '/');
    }

    presp->path_sz += strlen(vol->name)+1;
    presp->path_count++;

    *ptr = strlen(vol->name) & 0xFF;
    memcpy(ptr+1, vol->name, strlen(vol->name)&0xFF);

    size_t resp_sendsz = sizeof(mars_ncp_service_path_response_t) + presp->path_sz;
    int ret = mars_ncp_send(srv, presp, resp_sendsz, (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
    free(tmp);
    return ret;
}

#pragma endregion

int mars_ncp_service_entry_info(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_service_entry_request_t *req = (mars_ncp_service_entry_request_t *)buff;
    
    switch( req->sub_func ) {

        case MARS_NCP_SVC_ENTRY_INFO_FOR:
            return _mars_ncp_service_entry_info_for(srv, conn, sipx, buff, sz);
            break;

        case MARS_NCP_SVC_ENTRY_INFO_PATH:
            return _mars_ncp_service_entry_info_path(srv, conn, sipx, buff, sz);
            break;
    }

    return 1;
}
