/* 
 * This file is part of the mars_minwe distribution (https://github.com/hatestheinternet/mars_minwe).
 * Copyright (c) 2025 Jason Powell.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netipx/ipx.h>
#include <linux/route.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

#include <mars/ipx_utils.h>
#include <mars/router.h>
#include <mars/config.h>
#include <mars/rip.h>

_mars_router_context_t _mars_router_ctx;

int mars_router_add_route(uint32_t network, uint8_t gateway[6], uint32_t via) {
    struct rtentry rt; 
    struct sockaddr_ipx *sr;
    struct sockaddr_ipx *st;

    if( mars_network_is_local_net(network) ) {
        return 1;
    }

    memset(&rt, 0, sizeof(struct rtentry));
    sr = (struct sockaddr_ipx *)&rt.rt_gateway;
    st = (struct sockaddr_ipx *)&rt.rt_dst;

    sr->sipx_family = st->sipx_family = AF_IPX;
    st->sipx_network = network;
    sr->sipx_network = via;
    memcpy(sr->sipx_node, gateway, sizeof(sr->sipx_node));

    rt.rt_flags = RTF_GATEWAY;
    if( ioctl(_mars_router_ctx.rip.fd, SIOCADDRT, (void *)&rt) != 0 ) {
        fprintf(stderr, "mars_router_add_route: %s\n", strerror(errno));
    }
    
    return 0;
}

void *_mars_router_run(void *arg) {
    _mars_router_context_t *ctx = (_mars_router_context_t *)arg;

    mars_router_rip_packet_t rip_packet;
    mars_router_sap_packet_t sap_packet;

    int rip_fd = ctx->rip.fd;
    int sap_fd = ctx->sap.fd;
    int max_fd = (sap_fd>rip_fd)?sap_fd:rip_fd;
    int res;

    fd_set read_fds;
    struct timeval timeout;
    
    struct sockaddr_ipx sipx;
    socklen_t addr_len = sizeof(sipx);

    ctx->running = ctx->should_run = 1;
    while( ctx->should_run ) {
        if( ctx->rip.last_announce > 0 )
            mars_router_rip_announce();

        FD_ZERO(&read_fds);
        FD_SET(rip_fd, &read_fds);
        FD_SET(sap_fd,&read_fds);

        timeout.tv_sec = MARS_ROUTER_SELECT_TIMEOUT;
        timeout.tv_usec = 0;

        res = select(max_fd+1, &read_fds, NULL, NULL, &timeout);
        if( res ) {
            if( FD_ISSET(rip_fd, &read_fds) ) {
                res = recvfrom(rip_fd, &rip_packet, sizeof(rip_packet), 0, (struct sockaddr *)&sipx, &addr_len);
                mars_router_handle_rip(&rip_packet, res, &sipx);
                continue;
            }

            if( FD_ISSET(sap_fd, &read_fds) ) {
                res = recvfrom(sap_fd, &sap_packet, sizeof(sap_packet), 0, (struct sockaddr *)&sipx, &addr_len);
                mars_router_handle_sap(&sap_packet, res, &sipx);
                continue;
            }
        }

    }
    ctx->running = 0;

    return NULL;
}

int mars_router_start(void) {
    struct sockaddr_ipx sipx;
    mars_network_t *net;

    memset(&_mars_router_ctx, 0, sizeof(_mars_router_context_t));

    _mars_router_ctx.rip.dest.sipx_family = _mars_router_ctx.sap.dest.sipx_family = AF_IPX;
    _mars_router_ctx.rip.dest.sipx_type = IPX_RIP_PTYPE;
    _mars_router_ctx.rip.dest.sipx_port = htons(IPX_RIP_PORT);

    _mars_router_ctx.sap.dest.sipx_type = IPX_SAP_PTYPE;
    _mars_router_ctx.sap.dest.sipx_port = htons(IPX_SAP_PORT);

    memset(&sipx, 0, sizeof(sipx));
    sipx.sipx_family = AF_IPX;
    memcpy(sipx.sipx_node, IPX_THIS_NODE, sizeof(sipx.sipx_node));
    sipx.sipx_port = htons(IPX_RIP_PORT);
    sipx.sipx_type = IPX_RIP_PTYPE;

    if( (_mars_router_ctx.rip.fd = socket(AF_IPX, SOCK_DGRAM, AF_IPX)) < 0 ) {
        fprintf(stderr, "mars_router_start: RIP socket failed: %s\n", strerror(errno));
        return errno;
    }

    if( bind(_mars_router_ctx.rip.fd, (struct sockaddr *)&sipx, sizeof(sipx)) < 0 ) {
        fprintf(stderr, "mars_router_start: RIP bind failed : %s\n", strerror(errno));
        close(_mars_router_ctx.rip.fd);
        return errno;
    }

    int opt=1;
    if( setsockopt(_mars_router_ctx.rip.fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) == -1 ) {
        fprintf(stderr, "mars_router_start: SO_BROADCAST failed: %s\n", strerror(errno));
        close(_mars_router_ctx.rip.fd);
        return errno;
    }

    memset(&sipx, 0, sizeof(sipx));
    sipx.sipx_family = AF_IPX;
    memcpy(sipx.sipx_node, IPX_THIS_NODE, sizeof(sipx.sipx_node));
    sipx.sipx_port = htons(IPX_SAP_PORT);
    sipx.sipx_type = IPX_SAP_PTYPE;

    if( (_mars_router_ctx.sap.fd = socket(AF_IPX, SOCK_DGRAM, AF_IPX)) < 0 ) {
        fprintf(stderr, "mars_router_start: SAP socket failed: %s\n", strerror(errno));
        return errno;
    }

    if( bind(_mars_router_ctx.sap.fd, (struct sockaddr *)&sipx, sizeof(sipx)) < 0 ) {
        fprintf(stderr, "mars_router_start: SAP bind failed : %s\n", strerror(errno));
        close(_mars_router_ctx.sap.fd);
        return errno;
    }

    if( setsockopt(_mars_router_ctx.sap.fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) == -1 ) {
        fprintf(stderr, "mars_router_start: SO_BROADCAST failed: %s\n", strerror(errno));
        close(_mars_router_ctx.sap.fd);
        return errno;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    _mars_router_ctx.should_run = 1;
    pthread_create(&_mars_router_ctx.thread, &attr, &_mars_router_run, (void *)&_mars_router_ctx);

    pthread_attr_destroy(&attr);

    while( !_mars_router_ctx.running ) {
        printf("mars_router_start: Waiting for router to start\n");
        if( _mars_router_ctx.should_run )
            sleep(1);
    }

    if( _mars_router_ctx.running ) {
        net = mars_network_all();
        while( net ) {
            if( !net->internal ) {
                mars_router_send_rip_req(net->network);
            }
            net = net->next;
        }
        mars_router_rip_announce();
    }

    return 0;
}

void mars_router_stop(void) {
    if( _mars_router_ctx.running ) {
        _mars_router_ctx.should_run = 0;

        printf("mars_router_stop: Waiting for router to stop\n");
        pthread_join(_mars_router_ctx.thread, NULL);
    }

    if( _mars_router_ctx.rip.fd > 0 )
        close(_mars_router_ctx.rip.fd);

    if( _mars_router_ctx.sap.fd > 0 )
        close(_mars_router_ctx.sap.fd);
}
