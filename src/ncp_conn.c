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

typedef struct mars_server_ncp_connection_request_t {
    uint8_t sequence;
    uint8_t conn_low;
    uint8_t task_no;
    uint8_t conn_high;
    uint8_t completion;
    uint8_t status;
} mars_server_ncp_connection_request_t;

typedef struct mars_server_ncp_connection_response_t {
    uint16_t type;
    uint8_t seq_no;
    uint8_t conn_low;
    uint8_t task_no;
    uint8_t conn_high;
    uint8_t completion;
    uint8_t status;
} mars_server_ncp_connection_response_t;

int mars_server_ncp_create_connection(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_server_connection_t *conn;
    mars_server_ncp_connection_request_t *req = (mars_server_ncp_connection_request_t *)buff;
    mars_server_ncp_connection_response_t resp;
    int idx, res;
    
    uint16_t conn_no = (req->conn_high << 8) + req->conn_low;
    
    if( conn_no != 65535 ) {
        fprintf(stderr,"mars_server_ncp_create_connection: Invalid conn number %hu\n", conn_no);
        return -1;
    }

    pthread_mutex_lock(&srv->conn_mtx);

    for( idx = 0; idx < MARS_SERVER_MAX_NCP_CONN; idx++ ) {
        if( srv->connections[idx] == NULL ) {
            break;
        }
    }

    if( idx >= MARS_SERVER_MAX_NCP_CONN ) {
        pthread_mutex_unlock(&srv->conn_mtx);
        fprintf(stderr,"mars_server_ncp_create_connection: Max number of connections (%i) reached!\n", idx);
        return -1;
    }

    conn = calloc(1,sizeof(mars_server_connection_t));
    conn->last_activity = time(0);
    srv->connections[idx++] = conn;

    pthread_mutex_unlock(&srv->conn_mtx);

    if( mars_config_is_true(mars_config_global_str("dump_ncp") ) ) {
        printf("mars_server_ncp_create_connection[%i]: Created connection for %02X%02X%02X%02X%02X%02X@%08X\n", idx, MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network));
    }

    conn->task_number = resp.task_no;
    memcpy(conn->address, sipx->sipx_node, sizeof(sipx->sipx_node));
    conn->network = sipx->sipx_network;
    conn->seq_no = req->sequence;
    conn->task_number = req->task_no;
    conn->idx = idx;

    memset(&resp, 0, sizeof(mars_server_ncp_connection_response_t));
    resp.type = MARS_NCP_OP_SVC_RESP;
    resp.task_no = 1;
    resp.seq_no = req->sequence;
    resp.conn_low = idx & 0xFF;
    resp.conn_high = (idx >> 8) & 0xFF;
    resp.status = 0;
    resp.completion = 0;

    res = mars_server_ncp_send(srv, (void *)&resp, sizeof(resp), (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
    if( res < 0 ) {
        fprintf(stderr, "mars_server_ncp_create_connection: %s\n", strerror(res));
        srv->connections[--idx] = NULL;
        free(conn);
        idx = -1;
    }

    return idx;
}