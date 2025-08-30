#ifndef HAVE_MARS_NCP_H
#define HAVE_MARS_NCP_H

#define MARS_NCP_OP_CREATE_CON 0x1111U
#define MARS_NCP_OP_SVC_REQ 0x2222U

#define MARS_NCP_OP_SVC_RESP 0x3333U

#define MARS_NCP_SVC_DATE_TIME 0x14U
#define MARS_NCP_SVC_FSERV_INFO 0x17U
#define MARS_NCP_SVC_ENTRY_INFO 0x57U
#define MARS_NCP_SVC_PACKET_SZ 0x61U
#define MARS_NCP_SVC_BURST_MODE 0x65U
#define MARS_NCP_SVC_BUFFER_SZ 0x21U

#define MARS_NCP_NDS_SERVER_ADDR 0x68U

#define MARS_NCP_SVC_UNKONWN 0xFBU

#include <mars/server.h>
#include <netipx/ipx.h>
#include <netinet/in.h>

typedef struct mars_ncp_request_t {
    uint8_t sequence;
    uint8_t conn_low;
    uint8_t task_no;
    uint8_t conn_high;
} mars_ncp_request_t;

typedef struct mars_ncp_service_request_t {
    mars_ncp_request_t req;
    uint8_t function;
} mars_ncp_service_request_t;

typedef struct mars_ncp_response_t {
    uint16_t type __attribute__ ((packed));
    uint8_t seq_no;
    uint8_t conn_low;
    uint8_t task_no;
    uint8_t conn_high;
    uint8_t completion;
    uint8_t status;
} mars_ncp_response_t;


int mars_ncp_start(mars_server_t *srv);
int mars_ncp_handle(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);

int mars_ncp_create_connection(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);

int mars_ncp_response_prepare(mars_server_connection_t *conn, void *resp, size_t sz);

int mars_ncp_send(mars_server_t *srv, void *buff, size_t sz, struct sockaddr *saddr, socklen_t len);

int mars_ncp_service_packet_sz(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);
int mars_ncp_service_buffer_sz(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);
int mars_ncp_service_burst_mode(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);
int mars_ncp_service_fserv_info(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);
int mars_ncp_service_entry_info(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);
int mars_ncp_service_date_time(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);

#endif