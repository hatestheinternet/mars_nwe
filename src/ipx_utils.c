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
#include <unistd.h>
#include <linux/if.h>
#include <linux/route.h>
#include <netinet/in.h>
#include <netipx/ipx.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <mars/config.h>
#include <mars/network.h>

#ifndef MARS_NETWORK_IOCTL_RETRIES
#define MARS_NETWORK_IOCTL_RETRIES 5
#endif

#ifndef MARS_NETWORK_CFG_WAIT
#define MARS_NETWORK_CFG_WAIT 65
#endif

#define MARS_PRINTF_NETP_ADDR net->address[0], net->address[1], net->address[2], net->address[3], net->address[4], net->address[5]

int _mars_network_ioctl(const char *caller, unsigned long req, struct ifreq *id ) {
    int result, s, i=0;

    s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
    if( s < 0 ) {
        fprintf(stderr,"%s: Open socket failed\n", caller);
        return -1;
    }

    do {
        result = ioctl(s, req, id);
        i++;
    } while( (i<MARS_NETWORK_IOCTL_RETRIES) && (result<0) && (errno==EAGAIN) );

    close(s);

    return result!=-1?result:errno;
}

uint32_t mars_network_ipx_net(mars_network_t *net) {
    struct ifreq id;
    struct sockaddr_ipx *sipx = (struct sockaddr_ipx *)&id.ifr_addr;
    int result = 0;
    
    strcpy(id.ifr_name, net->devname);
    sipx->sipx_family = AF_IPX;
    sipx->sipx_type = net->frame;
    sipx->sipx_network = 0L;

    result = _mars_network_ioctl("mars_network_ipx_net", SIOCGIFADDR, &id);
    if( result == 0 ) {
        net->network = sipx->sipx_network;
        if( net->network != 0 ) {
            memcpy(net->address, sipx->sipx_node, sizeof(net->address));
            printf("mars_network_ipx_net: %s on %s: %08X (0x%02X%02X%02X%02X%02X%02X)\n", 
                mars_network_frame_str(net->frame), net->devname, htonl(net->network),
                MARS_PRINTF_NETP_ADDR);

            return net->network;
        }
    }

    return 0;
}

int mars_network_has_ipx(mars_network_t *net) {
    struct ifreq id;
    struct sockaddr_ipx *sipx = (struct sockaddr_ipx *)&id.ifr_addr;
    int result, s;

    if( net->internal )
        return 1;
    
    strcpy(id.ifr_name, net->devname);
    sipx->sipx_family = AF_IPX;
    sipx->sipx_type = net->frame;
    sipx->sipx_network = 0L;
    
    s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
    if( s < 0 ) {
        fprintf(stderr,"mars_network_has_ipx: Open socket failed\n");
        return -1;
    }

    result = _mars_network_ioctl("mars_network_has_ipx", SIOCGIFADDR, &id);
    switch( result ) {
        case 0:
            result = 1;
            break;

        case ENODEV:
            fprintf(stderr,"mars_network_has_ipx: No such device %s\n", net->devname);
            result = -1;
            break;

        case EPROTONOSUPPORT:
            fprintf(stderr,"mars_network_has_ipx: %s does not support %s\n", net->devname, mars_network_frame_str(net->frame));
            result = -1;
            break;

        case EADDRNOTAVAIL:
            result = 0;
            break;

        default:
            fprintf(stderr,"mars_network_has_ipx: %s on %s: %s\n", mars_network_frame_str(net->frame), net->devname, strerror(result));
            result = -1;
            break;
    }

    return result;
}

int mars_network_add_ipx(mars_network_t *net) {
    static struct ifreq	id;
    struct sockaddr_ipx *sipx = (struct sockaddr_ipx *)&id.ifr_addr;
    int result, i=0;

    // Internal interface
    if( net->internal ) {
        sipx->sipx_family = AF_IPX;
        sipx->sipx_action = IPX_CRTITF;
        sipx->sipx_type = IPX_FRAME_NONE;
        sipx->sipx_special = IPX_INTERNAL;
        sipx->sipx_network = htonl(net->network);
        memcpy(sipx->sipx_node, net->address, sizeof(net->address));

        result = _mars_network_ioctl("mars_network_add_ipx", SIOCSIFADDR, &id);
        if( result == 0 ) {
            printf("mars_network_add_ipx: Created internal network\n");
            return 1;
        }

        fprintf(stderr, "mars_network_add_ipx: %s\n", strerror(result));
        return 0;
    }
    
    strcpy(id.ifr_name, net->devname);
    sipx->sipx_family = AF_IPX;
    sipx->sipx_special = net->primary?IPX_PRIMARY:IPX_SPECIAL_NONE;
    sipx->sipx_network = net->network;
    sipx->sipx_type = net->frame;
    sipx->sipx_action = IPX_CRTITF;

    result = _mars_network_ioctl("mars_network_add_ipx", SIOCSIFADDR, &id);
    if( result == 0 ) {
        if( net->network == 0 ) {
            printf("mars_network_add_ipx: Waiting for %s on %s\n", mars_network_frame_str(net->frame), net->devname);
            do {
                result = mars_network_ipx_net(net);
                if( result == 0 ) {
                    sleep(1);
                }
            } while( (result == 0) && ++i < MARS_NETWORK_CFG_WAIT);

            if( !result ) {
                fprintf(stderr,"mars_network_add_ipx: You need to define a network= for %s on %s\n", mars_network_frame_str(net->frame), net->devname);
                result = 0;
            } else {
                result = 1;
            }
        } else {
            mars_network_ipx_net(net);
            printf("mars_network_add_ipx: Configured %s on %s as %08X\n", mars_network_frame_str(net->frame), net->devname, htonl(net->network));
            result = 1;
        }
    } else {
        fprintf(stderr,"mars_network_add_ipx: %s on %s failed: %s\n", mars_network_frame_str(net->frame), net->devname, strerror(result));
        result = 0;
    }
    
    return result;
}

int mars_network_remove_ipx(mars_network_t *net) {
    static struct ifreq	id;
    struct sockaddr_ipx *sipx = (struct sockaddr_ipx *)&id.ifr_addr;
    int result;
    
    if( !net->internal )
        strcpy(id.ifr_name, net->devname);
    
    sipx->sipx_family = AF_IPX;
    sipx->sipx_network = 0L;
    sipx->sipx_action = IPX_DLTITF;

    if( net->internal )
        sipx->sipx_special = IPX_INTERNAL;
    else
        sipx->sipx_type = net->frame;
    
    result = _mars_network_ioctl("mars_network_remove_ipx", SIOCSIFADDR, &id);
    if( result == 0 ) {
        printf("mars_network_remove_ipx: Removed %s from %s\n", mars_network_frame_str(net->frame), net->devname);
        result = 1;
    } else {
        fprintf(stderr,"mars_network_remove_ipx: %s from %s failed: %s\n", mars_network_frame_str(net->frame), net->devname, strerror(errno));
        result = 0;
    }

    return result;
}
