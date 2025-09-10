#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>

#include <netipx/ipx.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/server.h>
#include <mars/ncp.h>

#pragma region Function 0x21 - Buffer Size

typedef struct mars_ncp_buffer_size_request_t {
    uint8_t size_high;
    uint8_t size_low;
} mars_ncp_buffer_size_request_t;

typedef struct mars_ncp_buffer_size_response_t {
    mars_ncp_response_t resp;
    uint16_t size __attribute__ ((packed));
} mars_ncp_buffer_size_response_t;

int mars_ncp_service_buffer_sz(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_buffer_size_request_t *req = (mars_ncp_buffer_size_request_t *)buff;
    mars_ncp_buffer_size_response_t resp;

    uint16_t bsz = (req->size_high << 8) + req->size_low;
    conn->buff_sz = bsz;

    mars_ncp_response_prepare(conn, &resp, sizeof(resp));
    resp.size = htons(conn->buff_sz);

    if( mars_config_is_true(mars_config_global_str("dump_ncp")) ) {
        printf("mars_ncp_service_buffer_sz[%i]: " MARS_PRINTF_IPX_ADDR "@%08X set buffer size to %hu\n", conn->idx, MARS_PRINTF_SIPXP_ADDR, htonl(sipx->sipx_network), conn->buff_sz);
    }

    mars_ncp_send(srv, &resp, sizeof(mars_ncp_buffer_size_response_t), sipx);
    return 1;
}

#pragma endregion

#pragma region Function 0x61 - Max Packet Size

typedef struct mars_ncp_packet_size_request_t {
    uint8_t size_high;
    uint8_t size_low;
    uint8_t sec_flags;
} mars_ncp_packet_size_request_t;

typedef struct mars_ncp_packet_size_response_t {
    mars_ncp_response_t resp;
    uint16_t size __attribute__ ((packed));
    uint16_t echo_sock __attribute__ ((packed));
    uint8_t sec_flag;
    uint8_t padding[3];
} mars_ncp_packet_size_response_t;

int mars_ncp_service_packet_sz(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_packet_size_request_t *req = (mars_ncp_packet_size_request_t *)buff;
    mars_ncp_packet_size_response_t resp;

    uint16_t psz = (req->size_high << 8) + req->size_low;
    conn->packet_sz = psz;

    mars_ncp_response_prepare(conn, &resp, sizeof(resp));
    resp.size = htons(conn->packet_sz);
    resp.sec_flag = req->sec_flags;
    resp.echo_sock = htons(0x4002U);
    resp.padding[0] = 0x20;
    resp.padding[1] = 0x20;
    resp.padding[2] = 0x20;

    if( mars_config_is_true(mars_config_global_str("dump_ncp")) ) {
        printf("mars_ncp_service_packet_sz[%i]: " MARS_PRINTF_IPX_ADDR "@%08X set packet size to %hu\n", conn->idx, MARS_PRINTF_SIPXP_ADDR, htonl(sipx->sipx_network), conn->packet_sz);
    }

    mars_ncp_send(srv, &resp, sizeof(mars_ncp_packet_size_response_t), sipx);
    return 1;
}

#pragma endregion

#pragma region Function 0x65 - Packet Burst Mode


// TODO Figure this out

int mars_ncp_service_burst_mode(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_response_t resp;
    mars_ncp_response_prepare(conn, &resp, sizeof(resp));
    resp.completion = MARS_NCP_SVC_UNKONWN;

    if( mars_config_is_true(mars_config_global_str("dump_ncp") ) )
        printf("mars_ncp_service_burst_mode[%i]: " MARS_PRINTF_IPX_ADDR "@%08X has been refused burst mode\n", conn->idx, MARS_PRINTF_SIPXP_ADDR, htonl(sipx->sipx_network));
    
    mars_ncp_send(srv, &resp, sizeof(mars_ncp_response_t), sipx);
    return 1;
}

#pragma endregion

int mars_ncp_create_connection(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_server_connection_t *conn;
    mars_ncp_request_t *req = (mars_ncp_request_t *)buff;
    mars_ncp_response_t resp;
    int idx, res;
    
    uint16_t conn_no = (req->conn_high << 8) + req->conn_low;
    
    if( conn_no != 65535 ) {
        fprintf(stderr,"mars_ncp_create_connection: Invalid conn number %hu\n", conn_no);
        return -1;
    }

    pthread_mutex_lock(&srv->conn_mtx);

    for( idx = 0; idx < MARS_SERVER_MAX_CONN; idx++ ) {
        if( srv->connections[idx] == NULL ) {
            break;
        }
    }

    if( idx >= MARS_SERVER_MAX_CONN ) {
        pthread_mutex_unlock(&srv->conn_mtx);
        fprintf(stderr,"mars_ncp_create_connection: Max number of connections (%i) reached!\n", idx);
        return -1;
    }

    conn = calloc(1,sizeof(mars_server_connection_t));
    conn->last_activity = time(0);
    srv->connections[idx++] = conn;

    pthread_mutex_unlock(&srv->conn_mtx);

    if( mars_config_is_true(mars_config_global_str("dump_ncp") ) ) {
        printf("mars_ncp_create_connection[%i]: Created connection for %02X%02X%02X%02X%02X%02X@%08X\n", idx, MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network));
    }

    conn->task_number = resp.task_no;
    memcpy(conn->address, sipx->sipx_node, sizeof(sipx->sipx_node));
    conn->network = sipx->sipx_network;
    conn->seq_no = req->sequence;
    conn->task_number = req->task_no;
    conn->idx = idx;

    mars_ncp_response_prepare(conn, &resp, sizeof(resp));
    res = mars_ncp_send(srv, (void *)&resp, sizeof(resp), sipx);
    if( res < 0 ) {
        fprintf(stderr, "mars_ncp_create_connection: %s\n", strerror(res));
        srv->connections[--idx] = NULL;
        free(conn);
        idx = -1;
    }

    return idx;
}