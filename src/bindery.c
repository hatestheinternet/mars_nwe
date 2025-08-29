#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mars/config.h>
#include <mars/server.h>
#include <mars/bindery.h>

typedef struct mars_server_bindery_pam_t {
    char *group_name;
} mars_server_bindery_pam_t;

void _mars_bindery_pam_free(mars_server_t *srv) {
    if( srv->bindery ) {
        free(srv->bindery);
        srv->bindery = srv->destroy = NULL;
        printf("mars_bindery_pam_free: Shutdown\n");
    }
}

mars_server_t *_mars_bindery_pam_init(mars_server_t *srv, mars_config_section_t *cfg) {
    char *group_name = mars_config_str(cfg, "group");
    if( !group_name ) {
        fprintf(stderr,"mars_bindery_pam_init: No consider \"group\" defined\n");
        return NULL;
    }

    mars_server_bindery_pam_t *pam = calloc(1,sizeof(mars_server_bindery_pam_t));
    pam->group_name = group_name;

    srv->bindery = pam;
    srv->destroy = &_mars_bindery_pam_free;

    printf("mars_bindery_pam_init: Initialized PAM bindery\n");
    return srv;
}

mars_server_t *mars_bindery_init(mars_server_t *srv, mars_config_section_t *cfg) {
    char *type = mars_config_str(cfg, "source");
    
    if( strcmp(type,"pam") == 0 ) {
        return _mars_bindery_pam_init(srv, cfg);
    }

    return NULL;
}