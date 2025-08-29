#include <stdio.h>
#include <string.h>

#include <netinet/in.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/ncp.h>

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
