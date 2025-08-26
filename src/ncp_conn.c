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
    // char *ptr = buff;
    mars_server_connection_t *conn;
    mars_server_ncp_connection_request_t *req = (mars_server_ncp_connection_request_t *)buff;
    mars_server_ncp_connection_response_t resp;
    int idx, res;
    
    uint16_t conn_no = (req->conn_high << 8) + req->conn_low;
    
    if( conn_no != 65535 ) {
        fprintf(stderr,"mars_server_ncp_create_connection: Invalid conn number %hu\n", conn_no);
        return -1;
    }

    for( idx = 0; idx < MARS_SERVER_MAX_NCP_CONN; idx++ ) {
        if( srv->connections[idx] == NULL ) {
            break;
        }
    }

    if( idx >= MARS_SERVER_MAX_NCP_CONN ) {
        fprintf(stderr,"mars_server_ncp_create_connection: Max number of connections (%i) reached!\n", idx);
        return -1;
    }

    conn = calloc(1,sizeof(mars_server_connection_t));
    srv->connections[idx++] = conn;

    if( mars_config_is_true(mars_config_global_str("dump_ncp_conn") ) ) {
        printf("mars_server_ncp_create_connection[%i]: %02X%02X%02X%02X%02X%02X@%08X, Conn:%hu, Task:%i, Seq:%i\n", idx, MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network), conn_no, req->task_no, req->sequence);
    }

    conn->task_number = resp.task_no;
    memcpy(conn->address, sipx->sipx_node, sizeof(sipx->sipx_node));
    conn->network = sipx->sipx_network;
    conn->seq_no = req->sequence;

    memset(&resp, 0, sizeof(mars_server_ncp_connection_response_t));
    resp.type = MARS_NCP_REPLY_SVC;
    resp.task_no = 1;
    resp.seq_no = req->sequence;
    resp.conn_low = idx & 0xFF;
    resp.conn_high = (idx >> 8) & 0xFF;
    resp.status = 0;
    resp.completion = 0;

    pthread_mutex_lock(&srv->send_mtx);

    res = sendto(srv->fd, (void *)&resp, sizeof(resp), 0, (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));

    pthread_mutex_unlock(&srv->send_mtx);

    if( res < 0 ) {
        fprintf(stderr, "mars_server_ncp_create_connection: %s\n", strerror(errno));
        srv->connections[--idx] = NULL;
        free(conn);
        idx = -1;
    }

    return idx;
}