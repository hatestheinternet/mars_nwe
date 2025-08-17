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
 #include <stdlib.h>
 #include <string.h>
 #include <mars/config.h>

 #ifndef MARS_CONFIG_BUFFSZ
 #define MARS_CONFIG_BUFFSZ 8192
 #endif

 #ifndef MARS_CONFIG_FILE
 #define MARS_CONFIG_FILE "mars_minwe.ini"
 #endif

 mars_config_section_t *_mars_config = NULL;
 mars_config_section_t *_mars_config_global = NULL;

char *mars_util_ltrim(char *what) {
    char *first = what;
    
    while( *first && (*first == ' ' || *first == '\t' || *first == '\n' || *first == '\r') ) {
        first++;
    }

    return first;
}

char *mars_util_rtrim(char *what) {
    char *last = what + strlen(what)-1;
    while( last > what && (*last == ' ' || *last == '\r' || *last == '\n') ) {
        *last = 0;
        last--;
    }
    return what;
}

char *mars_util_trim(char *what) {
    return mars_util_ltrim(mars_util_rtrim(what));
}

int mars_config_is_true(char *what) {
    return what && ( !strcasecmp(what,"true") || !strcasecmp(what,"yes") || !strcasecmp(what,"on") || *what == '1' );
}

int mars_config_load(char *filename) {
    FILE *f = fopen(filename?filename:MARS_CONFIG_FILE, "rb");
    int loaded = 0;
    char *buff = NULL, *trimmed, *ptr;

    if( f == NULL ) {
        fprintf(stderr, "mars_config_load: Unable to open %s!\n", filename?filename:MARS_CONFIG_FILE);
        return -1;
    }

    printf("mars_config_load: Loading configuration from %s\n", filename?filename:MARS_CONFIG_FILE);
    buff = calloc(1, MARS_CONFIG_BUFFSZ);
    mars_config_section_t *sect = NULL;
    mars_config_item_t *item = NULL;
    size_t bsz = MARS_CONFIG_BUFFSZ;

    while( getline(&buff, &bsz, f) > 0 ) {
        trimmed = mars_util_trim(buff);
        
        // Ignore blank lines and commends
        if( *trimmed != 0 && *trimmed != ';') {
            // Beginning of a section
            if( *trimmed == '[' ) {
                if( sect != NULL ) {
                    sect->next = _mars_config;
                    _mars_config = sect;

                    if( !strcmp(sect->name, "global") ) {
                        _mars_config_global = sect;
                    }
                }
                sect = NULL;

                ptr = strchr(trimmed, ']');
                if( *ptr ) {
                    *ptr = 0;

                    sect = calloc(1, sizeof(mars_config_section_t));
                    sect->name = strdup(trimmed+1);
                    
                    loaded++;

                } else {
                    fprintf(stderr,"mars_config_load: Unclosed section %s\n", trimmed+1);
                }

            } else if( sect != NULL ) {
                ptr = strchr(trimmed,'=');
                
                if( ptr ) {
                    *ptr = 0;

                    item = calloc(1, sizeof(mars_config_item_t));
                    item->name = strdup(mars_util_trim(trimmed));
                    item->value = strdup(mars_util_trim(ptr+1));
                    item->next = sect->items;
                    sect->items = item;

                } else {
                    fprintf(stderr, "mars_config_load: %s has no =\n", trimmed);
                }
            }
        }
    }

    if( sect != NULL ) {
        sect->next = _mars_config;
        _mars_config = sect;
    }

    free(buff);
    fclose(f);

    if( loaded == 0 ) {
        printf("mars_config_load: Failed to load config file\n");
    }
    return loaded;
}

void mars_config_free(void) {
    mars_config_item_t *citem, *nitem;
    mars_config_section_t *cur = _mars_config, *nxt = cur;

    while( cur ) {
        nxt = cur->next;

        citem = nitem = cur->items;
        while( citem ) {
            nitem = citem->next;
            free(citem->name);
            free(citem->value);
            free(citem);

            citem = nitem;
        }

        free(cur->name);
        free(cur);

        cur = nxt;
    }
}

mars_config_section_t *mars_get_config(void) {
    return _mars_config;
}

char *mars_config_str(mars_config_section_t *section, char *key) {
    if( section == NULL )
        return NULL;

    mars_config_item_t *tst = section->items;

    while( tst ) {
        if( strcmp(tst->name, key) == 0 ) {
            return tst->value;
        }

        tst = tst->next;
    }

    return NULL;
}

uint32_t mars_config_uint32(mars_config_section_t *section, char *key) {
    char *tst = mars_config_str(section, key);
    if( !tst )
        return 0;

    return strtoul(tst,NULL,16);
}

char *mars_config_global_str(char *key) {
     return mars_config_str(_mars_config_global, key);
}

uint32_t mars_config_global_uint32(char *key) {
    return mars_config_uint32(_mars_config_global, key);
}