#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mars/config.h>
#include <mars/server.h>
#include <mars/bindery.h>

mars_server_t *_mars_servers = NULL;

int mars_server_has_volume(char *name) {
    mars_server_t *srv = _mars_servers;
    mars_server_volume_t *vol;

    while( srv ) {
        if( srv->type == MARS_SERVER_TYPE_FILE ) {
            vol = srv->volumes;
            while( vol ) {
                if( strcmp(vol->name, name) == 0 ) {
                    return 1;
                }
                vol = vol->next;
            }
        }
    }

    return 0;
}

int mars_server_add_volume(mars_server_volume_t *vol) {
    mars_server_t *srv = _mars_servers;

    while( srv ) {
        if( srv->type == MARS_SERVER_TYPE_FILE ) {
            break;
        }
        srv = srv->next;
    }

    if( !srv ) {
        srv = calloc(1,sizeof(mars_server_t));
        srv->type = MARS_SERVER_TYPE_FILE;
        srv->volumes = vol;
        srv->next = _mars_servers;
        _mars_servers = srv;

        printf("mars_server_config_volume: Will be masquerading as a file server\n");
    } else {
        vol->next = srv->volumes;
        srv->volumes = vol;
    }

    return 1;
}

int mars_server_config_volume(mars_config_section_t *cfg) {
    mars_server_volume_t *vol = calloc(1,sizeof(mars_server_volume_t));
    if( !vol ) {
        fprintf(stderr, "mars_server_config_volume: Failed to allocate memory for %s\n", mars_config_str(cfg,"name"));
        return 0;
    }

    vol->name = mars_config_str(cfg, "name");
    if( !vol->name ) {
        fprintf(stderr, "mars_server_config_volume: Section has no name=\n");
        free(vol);
        return 0;
    }

    vol->path = mars_config_str(cfg, "path");
    if( !vol->path ) {
        fprintf(stderr, "mars_server_config_volume: %s has no path=\n", vol->name);
        free(vol);
        return 0;
    }

    // TODO Check if path exists

    if( !mars_server_add_volume(vol) ) {
        free(vol);
        return 0;
    }

    return 1;
}

int mars_server_start(void) {
    mars_server_t *srv = _mars_servers;

    while( srv ) {
        switch( srv->type ) {
            case MARS_SERVER_TYPE_FILE:
                if( !mars_server_start_ncp(srv) )
                    return 0;
                break;
        }
        srv = srv->next;
    }

    return 1;
}

void mars_server_stop(void) {
    mars_server_t *srv = _mars_servers, *tsrv;

    while( srv ) {
        tsrv = srv;
        srv = srv->next;

        switch( tsrv->type ) {
            case MARS_SERVER_TYPE_FILE:
                if( tsrv->running ) {
                    tsrv->should_run = 0;
                    printf("mars_server_stop: Waiting for file server to stop\n");
                    pthread_join(tsrv->thread, NULL);
                }

                if( tsrv->fd )
                    close(tsrv->fd);

                mars_server_volume_t *vol = tsrv->volumes, *tmp;
                while( vol ) {
                    tmp = vol;
                    vol = vol->next;

                    free(tmp);
                }
                free(tsrv);
                break;

            case MARS_SERVER_TYPE_DIR:
                if( tsrv->destroy ) {
                    tsrv->destroy(tsrv);
                } else {
                    free(tsrv);
                }
                break;
        }
    }
}

int mars_server_init(void) {
    int ret = 0;
    mars_config_section_t *tst = mars_config_get_all();
    char *name;

    tst = mars_config_get_all();
    while( tst ) {
        // Are we going to be a file server?
        if( strcmp(tst->name, "volume") == 0 ) {
            name = mars_config_str(tst, "name");
            if( name == NULL ) {
                fprintf(stderr, "mars_server_init: Volume has no \"name\"\n");
            } else if( mars_server_has_volume(name) ) {
                fprintf(stderr,"mars_server_init: Duplicate volume name \"%s\"\n", name);
            } else {
                if( mars_server_config_volume(tst) ) {
                    ret = 1;
                    printf("mars_server_init: Added volume \"%s\"\n", name);
                }
            }
            
        // Maybe a directory server?
        } else if( strcmp(tst->name, "bindery") == 0 ) {
            if( mars_server_am_a(MARS_SERVER_TYPE_DIR) ) {
                ret = 0;
                fprintf(stderr,"mars_server_init: Ignoring duplicate [bindery] section\n");
                
            } else {
                mars_server_t *res = mars_bindery_init(tst);
                if( !res ) {
                    fprintf(stderr,"mars_server_init: Failed to initialize bindery\n");
                    ret = 0;
                    break;
                }

                res->next = _mars_servers;
                _mars_servers = res;
            }
        }

        tst = tst->next;
    }

    return ret;
}

int mars_server_am_a(uint16_t type) {
    mars_server_t *srv = _mars_servers;
    while( srv ) {
        if( srv->type == type )
            return 1;

        srv = srv->next;
    }

    return 0;
}
