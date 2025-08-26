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
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/ipx_utils.h>
#include <mars/router.h>

mars_network_t *_mars_networks_internal;
mars_network_t *_mars_networks = NULL;
const char *mars_minwe_frame_s[] = {"None", "SNAP", "802.2", "ETHERII", "802.3"};

const unsigned char IPX_THIS_NODE[6] = {0,0,0,0,0,0};
unsigned char IPX_BROADCAST_NODE[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const unsigned char IPX_TEST_NW[6] = {0xBC, 0x24, 0x11, 0xEF, 0x85, 0xB8};


const char *mars_network_frame_str(int what) {
    return mars_minwe_frame_s[what];
}

uint8_t _mars_network_char_to_val(char dig) {
    char digit = tolower(dig);
    if ((digit >= '0') && (digit <= '9')) {
            return digit - '0';
    } else if ((digit >= 'a') && (digit <= 'f')) {
            return (10 + (digit - 'a'));
    }

    return 0;
}

mars_network_t *mars_network_internal(void) {
    return _mars_networks_internal;
}

int mars_network_init(void) {
    mars_config_section_t *tst = mars_config_get_all();
    mars_network_t *net = NULL;
    char *tmp;//, *ptr;

    int loaded = 0;//,i;

    // TODO Internal Network
    net = calloc(1,sizeof(mars_network_t));
    net->internal = 1;
    net->network = mars_config_global_uint32("internal_net");

    // tmp = mars_config_global_str("internal_node");
    // if( !tmp || strlen(tmp) != 12 ) {
    //     fprintf(stderr, "mars_network_init: Internal node must be 12 characters\n");
    //     free(net);
    //     return 0;
    // }

    // for( ptr=tmp,i=0;i<6;i++ ) {
    //     net->address[i] = _mars_network_char_to_val(*ptr++);
    //     net->address[i] <<= 4;
    //     net->address[i] |= _mars_network_char_to_val(*ptr++);
    // }

    if( !mars_network_start(net) ) {
        free(net);
        return 0;
    }

    _mars_networks = _mars_networks_internal = net;

    while( tst ) {
        if( !strcmp(tst->name, "network") ) {
            net = calloc(1, sizeof(mars_network_t));
            net->devname = mars_config_str(tst,"device");
            // net->primary = mars_config_is_true(mars_config_str(tst,"primary"));
            net->network = htonl(mars_config_uint32(tst,"network"));
            
            tmp = mars_config_str(tst,"frame");
            switch( *(tmp+strlen(tmp)-1) ) {
                case '2': net->frame = IPX_FRAME_8022; break;
                case '3': net->frame = IPX_FRAME_8023; break;
                case 'I': case 'i': net->frame = IPX_FRAME_ETHERII; break;
                case 'P': case 'p': net->frame = IPX_FRAME_SNAP; break;
            }

            if( !net->frame ) {
                fprintf(stderr,"mars_network_init: Unknown frame type %s\n", tmp);
                free(net);
            } else {
                if( net->devname ) {
                    if( mars_network_start(net) ) {
                        net->next = _mars_networks;
                        _mars_networks = net;
                        loaded++;
                    }
                } else {
                    fprintf(stderr,"mars_network_init: Network did not have a device=\n");
                    free(net);
                }
            }
        }
        tst = tst->next;
    }

    if( loaded == 0 ) {
        printf("mars_network_init: Failed to intialize any networks\n");
    }
    return loaded;
}

int mars_network_start(mars_network_t *net) {
    int res = mars_network_has_ipx(net);

    if( net->internal )
        return mars_network_add_ipx(net);

    if( res < 0 ) {
        return 0;

    } else if( res ) {
        if( !mars_config_is_true(mars_config_global_str("takeover_ipx")) ) {
            fprintf(stderr, "mars_network_start: IPX active on %s %s and not taking over\n", net->devname, mars_network_frame_str(net->frame) );
            return 0;
        }

        if( !mars_network_stop(net) ) {
            return 0;
        }
    }

    res = mars_network_add_ipx(net);
    return res;
}

int mars_network_stop(mars_network_t *net) {
    return mars_network_remove_ipx(net)>0;
}

void mars_network_free(void) {
    mars_network_t *cur = _mars_networks, *tst;

    while( cur ) {
        tst = cur;
        cur = tst->next;

        mars_network_stop(tst);
        free(tst);
    }
}

int mars_network_is_local_net(uint32_t net) {
    mars_network_t *tst = _mars_networks;

    while( tst ) {
        if( tst->network == net )
            return 1;

        tst = tst->next;
    }

    return 0;
}

int mars_network_is_local_addr(unsigned char addr[6]) {
    mars_network_t *tst = _mars_networks;

    while( tst ) {
        if( memcmp(tst->address, addr, sizeof(tst->address)) == 0 )
            return 1;

        tst = tst->next;
    }

    return 0;
}

int mars_network_is_me(uint32_t net, uint8_t addr[6]) {
    return mars_network_is_local_net(net) && mars_network_is_local_addr(addr);
}

mars_network_t *mars_network_all(void) {
    return _mars_networks;
}