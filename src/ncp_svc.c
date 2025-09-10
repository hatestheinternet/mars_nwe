#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <netinet/in.h>
#include <netipx/ipx.h>

#include <mars/server.h>
#include <mars/ncp.h>
#include <mars/nds.h>

mars_server_connection_t *mars_server_ncp_find(mars_server_t *srv, mars_ncp_service_request_t *req, struct sockaddr_ipx *sipx) {
    uint16_t idx = (req->req.conn_high << 8) + (req->req.conn_low & 0xFF);
    mars_server_connection_t *conn = srv->connections[idx-1];

    if( conn != NULL ) {
        if( conn->task_number != req->req.task_no ) {
            fprintf(stderr, "mars_server_ncp_find: Task number mismatch on %hu\n", idx);
            return NULL;
        }

        if( req->req.sequence != conn->seq_no+1 ) {
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

int mars_ncp_handle(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    int ret = 0;
    mars_ncp_service_request_t *req = (mars_ncp_service_request_t *)buff;
    mars_server_connection_t *conn = mars_server_ncp_find(srv, req, sipx);
    
    if( conn != NULL ) {
        conn->last_activity = time(0);
        
        switch( req->function ) {
            case MARS_NCP_SVC_FSERV_INFO:
                conn->seq_no = req->req.sequence;
                ret = mars_ncp_service_fserv_info(srv, conn, sipx, buff+sizeof(mars_ncp_service_request_t), sz-sizeof(mars_ncp_service_request_t));
                break;

            case MARS_NCP_SVC_PACKET_SZ:
                conn->seq_no = req->req.sequence;
                ret = mars_ncp_service_packet_sz(srv, conn, sipx, buff+sizeof(mars_ncp_service_request_t), sz-sizeof(mars_ncp_service_request_t));
                break;

            case MARS_NCP_SVC_BUFFER_SZ:
                conn->seq_no = req->req.sequence;
                ret = mars_ncp_service_buffer_sz(srv, conn, sipx, buff+sizeof(mars_ncp_service_request_t), sz-sizeof(mars_ncp_service_request_t));
                break;

            case MARS_NCP_SVC_BURST_MODE:
                conn->seq_no = req->req.sequence;
                ret = mars_ncp_service_burst_mode(srv, conn, sipx, buff+sizeof(mars_ncp_service_request_t), sz-sizeof(mars_ncp_service_request_t));
                break;

            case MARS_NCP_NDS_SERVER_ADDR:
                conn->seq_no = req->req.sequence;
                ret = mars_nds_handle(srv, conn, sipx, buff, sz);
                break;

            case MARS_NCP_SVC_ENTRY_INFO:
                conn->seq_no = req->req.sequence;
                ret = mars_ncp_service_entry_info(srv, conn, sipx, buff, sz);
                break;

            case MARS_NCP_SVC_DATE_TIME:
                conn->seq_no = req->req.sequence;
                ret = mars_ncp_service_date_time(srv, conn, sipx, buff, sz);
                break;

            case MARS_NCP_SVC_DIRECTORY:
                conn->seq_no = req->req.sequence;
                ret = mars_ncp_service_directory(srv, conn, sipx, buff, sz);
                break;
        }

    }

    return ret;
}