#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#include <netipx/ipx.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/server.h>
#include <mars/ncp.h>
#include <mars/nds.h>

void long_to_mem(uint8_t *where, uint32_t what) {
    *where = what & 0xFF;
    *(where+1) = (what >> 8) & 0xFF;
    *(where+2) = (what >> 16) & 0xFF;
    *(where+3) = (what >> 24) & 0xFF;
}

#pragma region Function 0x68U:2 - NDS Get Server Address

typedef struct mars_nds_server_addr_request_t {
    mars_ncp_service_request_t req;
    uint8_t sub_func;
    uint32_t frag_handle __attribute__ ((packed));
    uint32_t frag_sz __attribute__ ((packed));
    uint32_t message_sz __attribute__ ((packed));
    uint32_t nds_flags __attribute__ ((packed));
    uint32_t verb __attribute__ ((packed));
    uint32_t reply_buff __attribute__ ((packed));
} mars_nds_server_addr_request_t;

typedef struct mars_nds_server_addr_response_t {
    mars_ncp_response_t resp;
    uint32_t frag_sz __attribute__ ((packed));
    uint32_t frag_handle __attribute__ ((packed));
    uint32_t completion_code __attribute__ ((packed));
} mars_nds_server_addr_response_t;

int mars_nds_server_addr(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_nds_server_addr_request_t *req = (mars_nds_server_addr_request_t *)buff;
    
    uint8_t dat[4096], tmp[128];
    uint8_t *ptr = dat, *pos;
    mars_nds_server_addr_response_t *resp = (mars_nds_server_addr_response_t *)ptr;
    mars_ncp_response_prepare(conn, resp, sizeof(dat));

    snprintf((char *)tmp, sizeof(tmp)-1, "CN=%s.O=%s",mars_config_server_name(),mars_config_global_str("organization"));
    uint32_t sname_bytes = strlen((char *)tmp) * 2;

    pos = ptr + sizeof(mars_nds_server_addr_response_t);
    long_to_mem(pos, sname_bytes);
    pos += 4;

    // "unicode"
    for(int i=0;i<strlen((char *)tmp);i++) {
        *pos = tmp[i];
        pos += 2;
    }
    
    // 1 referral record
    *pos = 0x01;
    pos += 8; // 4 + 4 for IPX protocol (0 long)

    // IPX referral = 12 bytes
    *pos = 0x0C;
    pos += 4;

    mars_network_t *net = mars_network_internal();

    // Next four = internet network
    *pos = (net->network >> 24) & 0xFF;
    *(pos+1) = (net->network >> 16) & 0xFF;
    *(pos+2) = (net->network >> 8) & 0xFF;
    *(pos+3) = net->network & 0xFF;
    pos += 4;

    // Node address
    memcpy(pos, net->address, 6);
    pos += 6;

    // Port (short)
    *pos = IPX_NCP_PORT >> 8;
    *(pos+1) = IPX_NCP_PORT & 0xFF;
    pos += 2;

    size_t send_sz = pos - ptr;
    resp->frag_handle = req->frag_handle;
    resp->frag_sz = send_sz - sizeof(mars_nds_server_addr_response_t);

    return mars_ncp_send(srv, resp, send_sz, (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
}

#pragma endregion

#pragma region Function 0x68U:1 - NDS Ping

typedef struct mars_nds_ping_request_t {
    mars_ncp_service_request_t req;
    uint8_t sub_func;
} mars_nds_ping_request_t;

typedef struct mars_nds_ping_response_t {
    mars_ncp_response_t resp;
    uint8_t nds_ver[4];
    uint8_t nds_len[4];
    uint8_t nds_tree[33];
    uint8_t nds_dist[4];
    uint8_t nds_rev[4];
    uint32_t flags __attribute__ ((packed));
} mars_nds_ping_response_t;

int mars_nds_ping(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    // mars_nds_ping_request_t *req = (mars_nds_ping_request_t *)buff;
    mars_nds_ping_response_t resp;
    mars_ncp_response_prepare(conn, &resp, sizeof(mars_nds_ping_response_t));

    resp.nds_ver[0] = 0x09U;
    resp.nds_rev[3] = 0x63U;
    resp.flags = 0x01000002U;

    resp.nds_len[0] = 33U;
    memset(&resp.nds_tree, 0x5F, 33);

    char *org = mars_config_global_str("organization");
    memcpy(&resp.nds_tree, org, strlen(org));

    return mars_ncp_send(srv, &resp, sizeof(mars_nds_ping_response_t), (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
}

#pragma endregion

int mars_nds_handle(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_nds_server_addr_request_t *req = (mars_nds_server_addr_request_t *)buff;
    mars_ncp_response_t resp;
    mars_ncp_response_prepare(conn, &resp, sizeof(resp));
    resp.completion = MARS_NCP_SVC_UNKONWN;

    switch( req->sub_func ) {
        case 1:
            return mars_nds_ping(srv, conn, sipx, buff, sz);    
            break;

        case 2:
            return mars_nds_server_addr(srv, conn, sipx, buff, sz);
            break;
    }

    printf("frag sz %u, msg sz %u, verb %u\n", req->frag_sz, req->message_sz, req->verb);
    
    mars_ncp_send(srv, &resp, sizeof(mars_ncp_response_t), (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
    return 1;
}