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
#ifndef HAVE_MARS_NETWORK_H
#define HAVE_MARS_NETWORK_H

#include <stdint.h>
#include <time.h>
#include <netipx/ipx.h>

/** 
 * A network.
 * 
 * Note that, to prevent this thing from potentially causing problems, a 
 * [network] section in the ini requires at least a device (device=eth0) 
 * and a frame type (frame=802.2|3,ETHERNET_II,SNAP).
 */
typedef struct mars_network_t {
    char *devname;      /** The name of the network device (eg eth0)    */
    int frame;          /** Frame type to use                           */
    int primary;        /** Use this as the primary interface?          */
    int internal;       /** Is this our internal interface?             */
    uint32_t network;   /** The IPX network address                     */
    uint8_t address[6]; /** Our node address (usually MAC)              */
    
    int ticks;          /** Hops (ticks), usually 1, 7+ for ISDN        */
    int is_up;          /** Whether this network is up                  */
    int wild_mask;
    time_t updated;     /** The last time the network updated           */
    int dirty;          /** If this network needs updating              */

    struct mars_network_t *next;
} mars_network_t;

extern const unsigned char IPX_THIS_NODE[6];
extern unsigned char IPX_BROADCAST_NODE[6];

#ifndef MARS_PRINTF_SIPX_ADDR
#define MARS_PRINTF_SIPX_ADDR sipx.sipx_node[0], sipx.sipx_node[1], sipx.sipx_node[2], sipx.sipx_node[3], sipx.sipx_node[4], sipx.sipx_node[5]
#endif

#ifndef MARS_PRINTF_SIPXP_ADDR
#define MARS_PRINTF_SIPXP_ADDR sipx->sipx_node[0], sipx->sipx_node[1], sipx->sipx_node[2], sipx->sipx_node[3], sipx->sipx_node[4], sipx->sipx_node[5]
#endif

#ifndef MARS_PRINTF_NETP_ADDR
#define MARS_PRINTF_NETP_ADDR net->address[0], net->address[1], net->address[2], net->address[3], net->address[4], net->address[5]
#endif

/**
 * Resolve an IPX frame type to a string.
 * 
 * @param what The frame type to query
 * @return The IPX frame type, or NULL
 */
const char *mars_network_frame_str(int what);

/**
 * Load networks from configuration.
 * 
 * Walk through the configuration looking for "device" sections
 * 
 * @return The number of networks loaded
 */
int mars_network_init(void);

/**
 * Configure a network interface.
 * 
 * @param net The network to configure
 * @return 1 on success, negative on failure
 */
int mars_network_start(mars_network_t *net);

/**
 * Shut down a network.
 * 
 * Remove IPX configuration from the specified interface (and frame type)
 * 
 * @param net The network to stop
 * @return 1 on success, negative on failure
 */
int mars_network_stop(mars_network_t *net);

/**
 * Stop all networks and release data.
 */
void mars_network_free(void);

/**
 * Get our internal network
 * 
 * @return Our internal network, or NULL
 */
mars_network_t *mars_network_internal(void);

/**
 * Check if the given network is directly attached
 * 
 * @param net The network to check
 * 
 * @return 1 if it is, 0 if not
 */
int mars_network_is_local_net(uint32_t net);

/**
 * Check if the given node and network are this host
 * 
 * @param net The network to check
 * @param addr The address to check
 * 
 * @return 1 if it is, 0 if not
 */
int mars_network_is_me(uint32_t net, uint8_t addr[6]);

/**
 * Return all networks
 * 
 * @return Our linked list of networks
 */
mars_network_t *mars_network_all(void);

#endif