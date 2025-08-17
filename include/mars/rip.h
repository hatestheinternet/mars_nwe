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
#ifndef HAVE_MARS_RIP_H
#define HAVE_MARS_RIP_H

#include <stdint.h>
#include <pthread.h>

#define IPX_RIP_OP_REQUEST (1U)
#define IPX_RIP_OP_RESPONSE (2U)

#ifndef MARS_ROUTER_SELECT_TIMEOUT
#define MARS_ROUTER_SELECT_TIMEOUT 2
#endif

#ifndef IPX_RIP_PORT
#define IPX_RIP_PORT (0x453U)
#endif

#ifndef IPX_RIP_PTYPE
#define IPX_RIP_PTYPE (1U)
#endif

#ifndef IPX_RIP_MAX_ENTRIES
#define IPX_RIP_MAX_ENTRIES 50U
#endif

/**
 * Entry in a RIP packet received from the network
 */
typedef struct mars_router_rip_entry_t {
    uint32_t network __attribute__ ((packed));
    unsigned short int hops __attribute__ ((packed));
    unsigned short int ticks __attribute__ ((packed));
} mars_router_rip_entry_t;

/**
 * RIP packet received from the network
 */
typedef struct mars_router_rip_packet_t {
    unsigned short int operation __attribute__ ((packed));
    mars_router_rip_entry_t entries[IPX_RIP_MAX_ENTRIES];
} mars_router_rip_packet_t;

/**
 * RIP-related things
 */
typedef struct mars_router_rip_t {
    int fd;

    struct sockaddr_ipx dest;
    mars_router_rip_packet_t packet;
    pthread_mutex_t send_mtx;

    time_t last_announce;
} mars_router_rip_t;

void mars_router_rip_announce(void);
void mars_router_send_rip_resp(struct sockaddr_ipx *sipx);
void mars_router_send_rip_req(uint32_t network);
void mars_router_handle_rip(mars_router_rip_packet_t *packet, int len, struct sockaddr_ipx *sipx);

#endif