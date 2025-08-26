#ifndef HAVE_MARS_NCP_H
#define HAVE_MARS_NCP_H

#define MARS_NCP_OP_CREATE_CON 0x1111U
#define MARS_NCP_OP_SVC_REQ 0x2222U

#define MARS_NCP_REPLY_SVC 0x3333U

#include <mars/server.h>
#include <netipx/ipx.h>

int mars_server_ncp_create_connection(mars_server_t *srv, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);

#endif