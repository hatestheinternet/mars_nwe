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

#ifndef HAVE_MARS_ROUTER_H
#define HAVE_MARS_ROUTER_H

#include <mars/network.h>
#include <mars/rip.h>
#include <mars/sap.h>

typedef struct _mars_router_context_t {
    int should_run;
    int running;

    mars_router_rip_t rip;
    mars_router_sap_t sap;
 
    pthread_t thread;
} _mars_router_context_t;
extern _mars_router_context_t _mars_router_ctx;

/**
 * Add the specified network to the router.
 * 
 * @param net The network to add
 * @return 1 on success, <0 on failure
 */
int mars_router_add(mars_network_t *net);

/**
 * Remove the specified network from the router.
 * 
 * @param net The network to remove
 */
void mars_router_remove(mars_network_t *net);

/**
 * Start the RIP router
 * 
 * @return < 0 on failure
 */
int mars_router_start(void);

/**
 * Stop the router
 */
void mars_router_stop(void);

int mars_router_add_route(uint32_t network, uint8_t gateway[6], uint32_t via);

#endif