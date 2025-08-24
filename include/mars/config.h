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
#ifndef HAVE_MARS_CONFIG_H
#define HAVE_MARS_CONFIG_H

#include <stdint.h>

typedef struct mars_config_item_t {
    char *name;
    char *value;

    struct mars_config_item_t *next;
} mars_config_item_t;

typedef struct mars_config_section_t {
    char *name;

    mars_config_item_t *items;

    struct mars_config_section_t *next;
} mars_config_section_t;

/** 
 * Load the mars_minwe configuration file (INI-style).
 * 
 * This is an incredibly basic, incredibly stupid parser that will handle most 
 * basic cases so it's best not to get too creative with the syntax.
 * 
 * @param filename The file to open, or "mars_minwe.ini" if NULL.
 */
int mars_config_load(char *filename);

/**
 * Free all memory used by the configuration.
 * 
 * Free and destroy all configuration items and sections. It's probably best 
 * to call this last.
 */
void mars_config_free();

/**
 * Return all configuration sections.
 * 
 * Basically return _mars_config.
 */
mars_config_section_t *mars_config_get_all(void);

/**
 * Get a string value from a config section.
 *
 * @param section The section to search
 * @param key The key we're after 
 * @return The value of the specified key in the given section, or NULL
 */
char *mars_config_str(mars_config_section_t *section, char *key);

/**
 * Get a string value from [global]
 * 
 * @param key The key we're after
 * @return The value, or NULL
 */
char *mars_config_global_str(char *key);

/**
 * Is the specified string "true"?
 * 
 * Considers: true, yes, on, 1
 */
int mars_config_is_true(char *what);

uint32_t mars_config_uint32(mars_config_section_t *section, char *key);
uint32_t mars_config_global_uint32(char *key);

char *mars_config_server_name(void);

#endif