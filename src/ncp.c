#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netipx/ipx.h>
#include <sys/select.h>

#include <mars/network.h>
#include <mars/server.h>
#include <mars/ncp.h>

void *_mars_server_ncp_run(void *arg) {
    mars_server_t *srv = (mars_server_t *)arg;
    fd_set read_fds;
    struct timeval timeout;
    struct sockaddr_ipx sipx;
    int max_fd, res;
    uint8_t buff[1500];
    socklen_t addr_len = sizeof(sipx);
    uint16_t ncp_type;

    srv->running = srv->should_run = 1;
    while( srv->should_run ) {
        FD_ZERO(&read_fds);
        
        max_fd = srv->fd;
        FD_SET(srv->fd, &read_fds);

        timeout.tv_sec = MARS_SERVER_NCP_SELECT;
        timeout.tv_usec = 0;

        res = select(max_fd+1, &read_fds, NULL, NULL, &timeout);
        if( res ) {
            if( FD_ISSET(srv->fd, &read_fds) ) {
                res = recvfrom(srv->fd, &buff, sizeof(buff), 0, (struct sockaddr *)&sipx, &addr_len);
                ncp_type = (buff[0] << 8) + buff[1];
                switch( ncp_type ) {
                    case MARS_NCP_OP_CREATE_CON:
                        mars_server_ncp_create_connection(srv, &sipx, buff+2, res-2);
                        break;

                    case MARS_NCP_OP_SVC_REQ:
                        break;

                    default:
                        printf("Type = %04X\n", ncp_type);
                        break;
                }
                
            }
        }
    }
    srv->running = 0;
    printf("mars_server_ncp_run: Stopped\n");

    return NULL;
}

int mars_server_start_ncp(mars_server_t *srv) {
    mars_network_t *net = mars_network_internal();
    struct sockaddr_ipx sipx;
    
    memset(&sipx, 0, sizeof(struct sockaddr_ipx));
    sipx.sipx_family = AF_IPX;
    sipx.sipx_type = IPX_NCP_PTYPE;
    sipx.sipx_port = htons(IPX_NCP_PORT);
    memcpy(sipx.sipx_node, net->address, sizeof(sipx.sipx_node));
    sipx.sipx_network = htonl(net->network);

    if( (srv->fd = socket(AF_IPX, SOCK_DGRAM, AF_IPX)) < 0 ) {
        fprintf(stderr, "mars_server_start_ncp: NCP socket failed: %s\n", strerror(errno));
        return errno;
    }

    if( bind(srv->fd, (struct sockaddr *)&sipx, sizeof(sipx)) < 0 ) {
        fprintf(stderr, "mars_router_start: NCP bind failed : %s\n", strerror(errno));
        close(srv->fd);
        return errno;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    srv->should_run = 1;
    pthread_create(&srv->thread, &attr, &_mars_server_ncp_run, (void *)srv);

    pthread_attr_destroy(&attr);

    printf("mars_server_start_ncp: Listening on %02X%02X%02X%02X%02X%02X @ %08X\n", MARS_PRINTF_SIPX_ADDR, net->network);

    return 1;
}