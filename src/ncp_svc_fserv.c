#include <stdio.h>
#include <string.h>

#include <netinet/in.h>

#include <mars/config.h>
#include <mars/ncp.h>

typedef struct mars_ncp_service_fserv_info_request_t {
    uint16_t packet_len;
    uint8_t sub_function;
} mars_ncp_service_fserv_info_request_t;

typedef struct mars_ncp_service_fserv_info_response_t {
    uint16_t type;
    uint8_t seq_no;
    uint8_t conn_low;
    uint8_t task_no;
    uint8_t conn_high;
    uint8_t completion;
    uint8_t status;
    uint8_t server[48];
    uint8_t major;
    uint8_t minor;
    uint16_t max_conns;
    uint16_t conns_in_use;
    uint16_t max_volumes;
    uint8_t os_rev;
    uint8_t sft;
    uint8_t tts;
    uint16_t conn_max_used;
    uint8_t ver_acct;
    uint8_t ver_vap;
    uint8_t ver_qms;
    uint8_t ver_print;
    uint8_t ver_vcons;
    uint8_t ver_secres;
    uint8_t ver_ibridge;
    uint8_t mix_mode_path;
    uint8_t local_login_info;
    uint16_t product_major;
    uint16_t product_minor;
    uint16_t product_rev;
    uint8_t os_lang_id;
    uint8_t sixtyfour;
    uint8_t stype;
    uint8_t ktype;
    uint8_t reserved[48];
} mars_ncp_service_fserv_info_response_t;

int mars_ncp_service_fserv_handle(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_service_fserv_info_request_t *req = (mars_ncp_service_fserv_info_request_t *)buff;

    // Defaults based on how my Netware 4.11/4.2 server responds
    mars_ncp_service_fserv_info_response_t resp;
    
    switch( req->sub_function ) {
        case 0x11:
            memset(&resp, 0, sizeof(mars_ncp_service_fserv_info_response_t));
            resp.seq_no = conn->seq_no;
            resp.type = 0x3333U;
            resp.task_no = 1;
            resp.major = 4;
            resp.minor = 11;
            resp.max_conns = htons(7U);
            resp.max_volumes = htons(255);
            resp.sft = 0x2;
            resp.tts = 1;
            resp.conn_max_used = 1U;
            resp.ver_acct = 1U;
            resp.ver_qms = 1;
            resp.ver_vap = 1;
            resp.ver_vcons = 1;
            resp.ver_secres = 1;
            resp.ver_ibridge = 1;
            resp.conn_high = (conn->idx >> 8) & 0xFF;
            resp.conn_low = conn->idx & 0xFF;
            memcpy(resp.server, mars_config_server_name(), 48);

            resp.conn_high = 
            
            pthread_mutex_lock(&srv->send_mtx);

            sendto(srv->fd, &resp, sizeof(mars_ncp_service_fserv_info_response_t), 0, (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));

            pthread_mutex_unlock(&srv->send_mtx);
            break;

        default:
            printf("Packet Len = %hu, Subfunc = %02X\n", ntohs(req->packet_len), req->sub_function);
            break;
    }

    return 0;
}