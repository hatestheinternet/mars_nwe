#ifndef HAVE_MARS_NDS_H
#define HAVE_MARS_NDS_H

#include <netipx/ipx.h>
#include <mars/server.h>

int mars_nds_handle(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz);

#endif