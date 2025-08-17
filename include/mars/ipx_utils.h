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
#ifndef HAVE_MARS_IPX_UTILS_H
#define HAVE_MARS_IPX_UTILS_H

#include <stdint.h>
#include <mars/network.h>

/**
 * Check if the given network has IPX configured.
 * 
 * @param net The network to check
 * @return Negative on error, 1 or 0 if IPX is present or not
 */
int mars_network_has_ipx(mars_network_t *net);

/**
 * Add the specified IPX configuration.
 * 
 * @param net The network to add
 * @return Negative on error, 1 on success
 */
int mars_network_add_ipx(mars_network_t *net);

/**
 * Remove the specified IPX configuration.
 * 
 * @param net The network to remove
 * @return Negative on error, 1 on success
 */
int mars_network_remove_ipx(mars_network_t *net);

/**
 * Get the given network's address
 * 
 * @param net The network we want to check
 * @return The network or 0
 */
uint32_t mars_network_ipx_net(mars_network_t *net);

#endif