/*
 * MorceNOX-ASTRO™
 * Copyright (C) 2026 Amilcar Antonio Mesquita Rizk amilcar.rizk@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://gnu.org>.
 */

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "environment.h"

// Function to expand environment variables in a string
char* expand_env_vars(const char* input) {
    if (!input) return NULL;
    
    // Allocate buffer for expanded string (larger than input to handle expansion)
    char* expanded = malloc(strlen(input) * 2 + 1); // Double size as a safe estimate
    if (!expanded) return NULL;
    
    char* result = expanded;
    const char* src = input;
    
    while (*src) {
        // If we encounter a dollar sign, check for variable
        if (*src == '$') {
            src++; // Skip the dollar sign
            
            // Handle special case: $$ becomes $
            if (*src == '$') {
                *result++ = '$';
                src++;
                continue;
            }
            
            // Find the end of variable name (alphanumeric + underscore)
            const char* var_start = src;
            while (*src && (isalnum(*src) || *src == '_')) {
                src++;
            }
            
            // If we found a variable name
            if (src > var_start) {
                // Create temporary variable name
                int var_len = src - var_start;
                char* var_name = malloc(var_len + 1);
                if (var_name) {
                    strncpy(var_name, var_start, var_len);
                    var_name[var_len] = '\0';
                    
                    // Get the environment variable value
                    const char* var_value = getenv(var_name);
                    if (var_value) {
                        // Copy the value
                        strcpy(result, var_value);
                        result += strlen(var_value);
                    } else {
                        // If not found, copy the variable name as is
                        memcpy(result, var_start, var_len);
                        result += var_len;
                    }
                    
                    free(var_name);
                }
            } else {
                // Single $, treat as literal
                *result++ = '$';
            }
        } else {
            // Regular character
            *result++ = *src++;
        }
    }
    
    *result = '\0';
    return expanded;
}

// Function to read environment variables from .env file
int load_env_file(char *env_path) {
    FILE *file = fopen(env_path, "r");
    if (!file) {
        fprintf(stderr, "Warning: Could not open .env file at %s\n", env_path);
        return 0;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline characters
        line[strcspn(line, "\r\n")] = 0;
        
        // Skip empty lines and comments
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }
        
        // Parse key=value format
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char *key = line;
            char *value = eq + 1;
            
            // Remove quotes if present
            if (*value == '"' || *value == '\'') {
                char quote = *value;
                value++;
                char *end = strrchr(value, quote);
                if (end) *end = '\0';
            }
            
            // Expand environment variables in value
            char *expanded_value = expand_env_vars(value);
            if (expanded_value) {
                // Set environment variable
                setenv(key, expanded_value, 1);
                free(expanded_value);
            } else {
                // If expansion failed, set original value
                setenv(key, value, 1);
            }
        }
    }
    
    fclose(file);
    return 1;
}

float get_env_float(const char *key, float default_value) {
    const char *value = getenv(key);
    if (value) {
        return atof(value);
    }
    return default_value;
}

int get_env_int(const char *key, int default_value) {
    const char *value = getenv(key);
    if (value) {
        return atoi(value);
    }
    return default_value;
}