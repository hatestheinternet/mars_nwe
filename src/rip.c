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
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <netipx/ipx.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/router.h>
#include <mars/rip.h>

void _mars_router_prepare_rip_req(mars_router_rip_packet_t *packet) {
    memset(packet, 0, sizeof(mars_router_rip_packet_t));
    
    packet->operation = ntohs(IPX_RIP_OP_REQUEST);
    packet->entries[0].network = 0xFFFFFFFF;
    packet->entries[0].hops = 0;
    packet->entries[0].ticks = 0;
}

void _mars_router_prepare_rip_resp(mars_router_rip_packet_t *packet) {
    memset(packet, 0, sizeof(mars_router_rip_packet_t));
    
    packet->operation = ntohs(IPX_RIP_OP_RESPONSE);
}

void mars_router_send_rip_req(uint32_t network) {
    pthread_mutex_lock(&_mars_router_ctx.rip.send_mtx);
    _mars_router_prepare_rip_req(&_mars_router_ctx.rip.packet);

    _mars_router_ctx.rip.dest.sipx_network = network;
    memcpy(_mars_router_ctx.rip.dest.sipx_node, IPX_BROADCAST_NODE, sizeof(IPX_BROADCAST_NODE));
    
    int res = sendto(_mars_router_ctx.rip.fd, (void *)&_mars_router_ctx.rip.packet,
                        2+sizeof(mars_router_rip_entry_t), 0,
                        (struct sockaddr *)&_mars_router_ctx.rip.dest, sizeof(struct sockaddr_ipx));
    if( res < 0 ) {
        fprintf(stderr, "mars_router_send_rip_req: Error %s\n", strerror(errno));
    }
    pthread_mutex_unlock(&_mars_router_ctx.rip.send_mtx);
}

void mars_router_send_rip_resp(struct sockaddr_ipx *sipx) {
    int ent = 0;
    mars_network_t *net;

    pthread_mutex_lock(&_mars_router_ctx.rip.send_mtx);
    _mars_router_prepare_rip_resp(&_mars_router_ctx.rip.packet);

    // TODO Maybe someday more than our internal net
    net = mars_network_all();
    while( net ) {
        if( net->internal ) {
            _mars_router_ctx.rip.packet.entries[ent].network = htonl(net->network);
            _mars_router_ctx.rip.packet.entries[ent].hops = htons(1);
            _mars_router_ctx.rip.packet.entries[ent++].ticks = htons(2);
        }
        net = net->next;
    }

    int res = sendto(_mars_router_ctx.rip.fd, (void *)&_mars_router_ctx.rip.packet,
                        2+(ent*sizeof(mars_router_rip_entry_t)),0,
                        (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
    if( res < 0 ) {
        fprintf(stderr, "mars_router_send_rip_resp: Error %s\n", strerror(errno));
    }

    pthread_mutex_unlock(&_mars_router_ctx.rip.send_mtx);
}

void mars_router_rip_announce(void) {
    mars_network_t *net = mars_network_all();
    struct sockaddr_ipx sipx;

    if( time(0) - _mars_router_ctx.rip.last_announce > 30 ) {
        memset(&sipx, 0, sizeof(struct sockaddr_ipx));
        sipx.sipx_family = AF_IPX;
        sipx.sipx_type = IPX_RIP_PTYPE;
        sipx.sipx_port = htons(IPX_RIP_PORT);
        memcpy(sipx.sipx_node, IPX_BROADCAST_NODE, sizeof(sipx.sipx_node));

        while( net ) {
            if( !net->internal ) {
                sipx.sipx_network = net->network;

                mars_router_send_rip_resp(&sipx);
            }
            net = net->next;
        }
        _mars_router_ctx.rip.last_announce = time(0);
    }
}

void mars_router_handle_rip(mars_router_rip_packet_t *packet, int len, struct sockaddr_ipx *sipx) {
    mars_router_rip_entry_t *re = packet->entries;
    int ent = (len-2) / sizeof(mars_router_rip_entry_t);
    int dump_rip = mars_config_is_true(mars_config_global_str("dump_rip"));
    int is_me = mars_network_is_me(sipx->sipx_network, sipx->sipx_node);

    switch( ntohs(packet->operation) ) {
        case IPX_RIP_OP_REQUEST:
            if( dump_rip ) {
                printf("==================================\n");
                printf("       RECEIVED RIP REQUEST\n");
                printf("----------------------------------\n");
                printf("Network: %08X\n", ntohl(sipx->sipx_network));
                printf("Gateway: %02X%02X%02X%02X%02X%02X%s\n", MARS_PRINTF_SIPXP_ADDR, is_me?" (me)":"");
                printf("Packet: Op %i, len %i, entries %i\n", ntohs(packet->operation), len, ent);
                for( ; ent>0; ent-=1) {
                   if( dump_rip )
                        printf("%08X - %i hops, %i ticks\n", ntohl(re->network), ntohs(re->hops), ntohs(re->ticks));

                    re++;
                }
                printf("----------------------------------\n");
            }
            if( !is_me ) {
                mars_router_send_rip_resp(sipx);
                printf("mars_router_handle_rip: Sent response to %02X%02X%02X%02X%02X%02X\n", MARS_PRINTF_SIPXP_ADDR);
            }
            break;

        case IPX_RIP_OP_RESPONSE:
            if( dump_rip ) {
                printf("=================================\n");
                printf("       RECEIVED RIP PACKET\n");
                printf("---------------------------------\n");
                printf("Network: %08X\n", ntohl(sipx->sipx_network));
                printf("Gateway: %02X%02X%02X%02X%02X%02X%s\n", MARS_PRINTF_SIPXP_ADDR, is_me?" (me)":"");
                printf("Packet: Op %i, len %i, entries %i\n", ntohs(packet->operation), len, ent);
            }

            for( ; ent>0; ent-=1) {
                if( dump_rip )
                    printf("%08X - %i hops, %i ticks\n", ntohl(re->network), ntohs(re->hops), ntohs(re->ticks));

                if( !is_me )
                    mars_router_add_route(re->network, sipx->sipx_node, sipx->sipx_network);
                
                re++;
            }

            if( dump_rip )
                printf("=================================\n");
            break;

        default:
            fprintf(stderr, "mars_router_handle_rip: Unknown operation %i\n", ntohs(packet->operation));
            break;
    }
}