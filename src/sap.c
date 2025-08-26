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
#include <pthread.h>

#include <netinet/in.h>
#include <netipx/ipx.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/router.h>
#include <mars/sap.h>
#include <mars/server.h>

int mars_router_send_sap_file_response(struct sockaddr_ipx *sipx) {
    mars_network_t *net = mars_network_internal();
    pthread_mutex_lock(&_mars_router_ctx.sap.send_mtx);

    memset(&_mars_router_ctx.sap.packet, 0, sizeof(_mars_router_ctx.sap.packet));
    _mars_router_ctx.sap.packet.operation = htons(MARS_SAP_GNS_RESP);
    strncpy(_mars_router_ctx.sap.packet.entries[0].name, mars_config_server_name(), 47);
    memcpy(&_mars_router_ctx.sap.packet.entries[0].node, net->address, sizeof(net->address));
    _mars_router_ctx.sap.packet.entries[0].network = htonl(net->network);
    _mars_router_ctx.sap.packet.entries[0].hops = htons(1);
    _mars_router_ctx.sap.packet.entries[0].type = htons(MARS_SAP_FILE_SERVER);
    _mars_router_ctx.sap.packet.entries[0].port = htons(IPX_NCP_PORT);
    
    if( mars_config_is_true(mars_config_global_str("dump_sap")) ) {
        printf("-----------------------------\n");
        printf("Server: %s\n", _mars_router_ctx.sap.packet.entries[0].name);
        printf("%02X%02X%02X%02X%02X%02X @ %08X\n", MARS_PRINTF_NETP_ADDR, net->network);
    }

    int res = sendto(_mars_router_ctx.sap.fd, (void *)&_mars_router_ctx.sap.packet,
                        2+sizeof(mars_router_sap_entry_t),0,
                        (struct sockaddr *)sipx, sizeof(struct sockaddr_ipx));
    if( res < 0 ) {
        fprintf(stderr, "mars_router_send_sap_file_response: Error %s\n", strerror(errno));
    }

    pthread_mutex_unlock(&_mars_router_ctx.sap.send_mtx);
    return 1;
}

void mars_router_handle_sap(mars_router_sap_packet_t *packet, int len, struct sockaddr_ipx *sipx) {
    int dump_sap = mars_config_is_true(mars_config_global_str("dump_sap"));

    switch( ntohs(packet->operation)) {
        case MARS_SAP_GNS_REQ:
            if( dump_sap ) {
                printf("=============================\n");
                printf("     GET NEAREST SERVER\n");
                printf("-----------------------------\n");
                printf("From: %02X%02X%02X%02X%02X%02X @ %08X\n", MARS_PRINTF_SIPXP_ADDR, ntohl(sipx->sipx_network));
                printf("Server Type: %04X",ntohs(packet->entries[0].type));
            }
            if( mars_server_am_a(ntohs(packet->entries[0].type)) ) {
                if( dump_sap )
                    printf(" (me)\n");

                mars_router_send_sap_file_response(sipx);
            } else if( dump_sap ) {
                printf("\n");
            }
            break;

        default:
            fprintf(stderr,"mars_router_handle_sap: Unhandled SAP type %i\n", ntohs(packet->operation));
            return;
    }

    if( dump_sap ) {
        printf("=============================\n");
    }

    return;
}
