// common/src/config.c — minimal key=value config file loader.
// TODO: implement vom_config_load() that tolerates comments (# and ;) and
//       supports [section] headers, returning a populated vom_config.
// TODO: implement vom_config_defaults() that fills a sane baseline
//       (heartbeat = 1s, log level = INFO, master endpoint = loopback).

#include "common/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//so these are standart values to keep everything neat.
#define MAX_LİNE_LEN 256
#define MAX_NAME_LEN 64
#define MAX_VAL_LEN 128
#define MAX_ITEMS 100

//structure layout
typedef struct {
    char section[MAX_NAME_LEN];
    char key[MAX_NAME_LEN];
    char value[MAX_VAL_LEN];
} vom_config_entry;

typedef struct{
    vom_config_entry entries[MAX_ITEMS];
    int count;
} vom_config;

//helper function
static char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end +1) = '\0';
    return str;
}

//comment stripper
static void strip_comment(char *str) {
    char *hash = strchr(str, '#');
    char *semi = strchr(str, ';');
    if (hash && semi) {
        * (hash < semi ? hash : semi) = '\0';
    } else if (hash) {
        *hash = '\0';
    } else if (semi) {
        *semi = '\0';
    }
}

// helper to inject entries into vom_config
static void add_entry(vom_config *config, const char *sec, const char *key, const char *val) {
    if (config->count >= MAX_ITEMS) return;

    strncpy(config->entries[config->count].section, sec, MAX_NAME_LEN - 1);
    strncpy(config->entries[config->count].key, key, MAX_NAME_LEN -1);
    strmcpy(config->entries[config->count].value, val, MAX_VAL_LEN - 1);

    config->entries[config->count].section[MAX_NAME_LEN -1] = '\0';
    config->entries[config->count].key[MAX_NAME_LEN - 1] = '\0';
    config->entries[config->count].value[MAX_NAME_LEN -1] = '\0';

    config->count++;
}

// config for system constrains
vom_config vom_config_defaults(void) {
    vom_config config;
    config.count = 0;

    add_entry(&config, "system", "heartbeat", "1s");
    add_entry(&config, "logging", "level", "INFO");
    add_entry(&config, "network", "master_endpoint", "127.0.0.1");

    return config;
}

vom_config vom_config_load(const char *filename) {
    vom_config config;
    config.count = 0;

    FILE *file = fopen(filename, "r");
    if (!file) {
        return config; 
        //return empty if it doesnt find anything (hopefully)
    }
}