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
#ifndef HAVE_MARS_SAP_H
#define HAVE_MARS_SAP_H

#include <pthread.h>

#ifndef IPX_SAP_PORT
#define IPX_SAP_PORT (0x452U)
#endif

#ifndef IPX_SAP_PTYPE
#define IPX_SAP_PTYPE (4U)
#endif

#define MARS_SAP_REQUEST (1U)
#define MARS_SAP_RESPONSE (2U)
#define MARS_SAP_GNS_REQ (3U)
#define MARS_SAP_GNS_RESP (4U)
#define MARS_SAP_GENERAL_REQ (0xFFFFU)

#define MARS_SAP_FILE_SERVER (4U)

typedef struct mars_router_sap_entry_t {
    unsigned short int type __attribute__ ((packed));
    char name[48];
    uint32_t network __attribute__ ((packed));
    uint8_t node[6];
    unsigned short int port __attribute__ ((packed));
    unsigned short int hops __attribute__ ((packed));
} mars_router_sap_entry_t;

typedef struct mars_router_sap_packet_t {
    unsigned short int operation __attribute__ ((packed));
    mars_router_sap_entry_t entries[7];
} mars_router_sap_packet_t;

typedef struct mars_router_sap_t {
    int fd;

    struct sockaddr_ipx dest;
    mars_router_sap_packet_t packet;
    pthread_mutex_t send_mtx;
} mars_router_sap_t;

void mars_router_handle_sap(mars_router_sap_packet_t *packet, int len, struct sockaddr_ipx *sipx);

#endif