#ifndef HAVE_MARS_NCP_H
#define HAVE_MARS_NCP_H

#define MARS_NCP_OP_CREATE_CON 0x1111U
#define MARS_NCP_OP_SVC_REQ 0x2222U

#define MARS_NCP_REPLY_SVC 0x3333U

#define MARS_NCP_SVC_FSERV_INFO 0x17U

#include <mars/server.h>
#include <netipx/ipx.h>

typedef struct mars_server_ncp_request_t {
    uint8_t sequence;
    uint8_t conn_low;
    uint8_t task_no;
    uint8_t conn_high;
} mars_server_ncp_request_t;

int mars_server_ncp_create_connection(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);
mars_server_connection_t *mars_server_ncp_find_connection(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t task_no, uint8_t seq_no);

int mars_server_ncp_service_request(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);

int mars_ncp_service_fserv_handle(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);

#endif