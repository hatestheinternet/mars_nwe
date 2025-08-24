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
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include <mars/config.h>
#include <mars/network.h>
#include <mars/router.h>
#include <mars/server.h>

int _mars_main_should_run = 1;

void signal_handler(int dummy) {
    _mars_main_should_run = 0;
}

int main(void) {
    int ret = 0;

    if( !mars_config_load(NULL) ) {
        return 1;
    }

    if( !mars_network_init() ) {
        ret = 1;
        goto mars_main_do_exit;
    }

    if( !mars_server_init() ) {
        ret = 1;
        goto mars_main_do_exit;
    }

    signal(SIGINT, signal_handler);

    mars_router_start();

    printf("main: I am %s\n", mars_config_server_name());
    while( _mars_main_should_run ) {
        sleep(30);
    }
    
mars_main_do_exit:
    mars_server_stop();
    mars_router_stop();
    mars_network_free();
    mars_config_free();
    return ret;
}