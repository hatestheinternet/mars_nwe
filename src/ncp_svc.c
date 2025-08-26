#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <netinet/in.h>
#include <netipx/ipx.h>

#include <mars/server.h>
#include <mars/ncp.h>

typedef struct mars_ncp_service_request_t {
    uint8_t sequence;
    uint8_t conn_low;
    uint8_t task_no;
    uint8_t conn_high;
    uint8_t function;
} mars_ncp_service_request_t;

mars_server_connection_t *mars_server_ncp_find(mars_server_t *srv, mars_ncp_service_request_t *req, struct sockaddr_ipx *sipx) {
    uint16_t idx = (req->conn_high << 8) + (req->conn_low & 0xFF);
    mars_server_connection_t *conn = srv->connections[idx-1];

    if( conn != NULL ) {
        if( conn->task_number != req->task_no ) {
            fprintf(stderr, "mars_server_ncp_find: Task number mismatch on %hu\n", idx);
            return NULL;
        }

        if( req->sequence != conn->seq_no+1 ) {
            fprintf(stderr, "mars_server_ncp_find: Sequence mismatch on connection %hu\n", idx);
            return NULL;
        }

        if( memcmp(conn->address, sipx->sipx_node, sizeof(conn->address) != 0) || (conn->network != sipx->sipx_network) ) {
            fprintf(stderr, "mars_server_ncp_find: Node or network mismatch on %hu\n", idx);
            return NULL;
        }

        return conn;
    }

    return NULL;
}

int mars_server_ncp_service_request(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_service_request_t *req = (mars_ncp_service_request_t *)buff;
    mars_server_connection_t *conn = mars_server_ncp_find(srv, req, sipx);

    if( conn != NULL ) {
        switch( req->function ) {
            case MARS_NCP_SVC_FSERV_INFO:
                conn->seq_no = req->sequence;
                mars_ncp_service_fserv_handle(srv, conn, sipx, buff+sizeof(mars_ncp_service_request_t), sz-sizeof(mars_ncp_service_request_t));
                break;
        }
    }

    return 0;
}