#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN 256
#define MAX_NAME_LEN 64
#define MAX_VAL_LEN  128
#define MAX_ITEMS    100

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_STR_MAX 128

typedef struct {
    char cluster_name[CONFIG_STR_MAX];
    char master_endpoint[CONFIG_STR_MAX];
    char worker_id[CONFIG_STR_MAX];
    uint32_t heartbeat_interval_sec;
    char advertised_capabilities[CONFIG_STR_MAX];
    char log_level[CONFIG_STR_MAX];
} vom_config_t;

/* --- Core Function Declarations --- */
void vom_config_defaults(vom_config_t *out);
bool vom_config_load(const char *filename, vom_config_t *out);

static char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

static void strip_comment(char *str) {
    char *hash = strchr(str, '#');
    char *semi = strchr(str, ';');
    if (hash && semi) {
        *(hash < semi ? hash : semi) = '\0';
    } else if (hash) {
        *hash = '\0';
    } else if (semi) {
        *semi = '\0';
    }
}

static void set_config_entry(vom_config_t *config, const char *section, const char *key, const char *val) {
    if (strcmp(section, "cluster") == 0 && strcmp(key, "name") == 0) {
        strncpy(config->cluster_name, val, CONFIG_STR_MAX - 1);
    } else if (strcmp(section, "master") == 0 && strcmp(key, "endpoint") == 0) {
        strncpy(config->master_endpoint, val, CONFIG_STR_MAX - 1);
    } else if (strcmp(section, "worker") == 0 && strcmp(key, "id") == 0) {
        strncpy(config->worker_id, val, CONFIG_STR_MAX - 1);
    } else if (strcmp(section, "worker") == 0 && strcmp(key, "heartbeat_interval") == 0) {
        config->heartbeat_interval_sec = (uint32_t)strtoul(val, NULL, 10);
    } else if (strcmp(section, "worker") == 0 && strcmp(key, "capabilities") == 0) {
        strncpy(config->advertised_capabilities, val, CONFIG_STR_MAX - 1);
    } else if (strcmp(section, "logging") == 0 && strcmp(key, "level") == 0) {
        strncpy(config->log_level, val, CONFIG_STR_MAX - 1);
    }
}

void vom_config_defaults(vom_config_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(vom_config_t));
    strncpy(out->cluster_name, "vom-default-cluster", CONFIG_STR_MAX - 1);
    strncpy(out->master_endpoint, "tcp://127.0.0.1:5555", CONFIG_STR_MAX - 1);
    strncpy(out->worker_id, "worker-default", CONFIG_STR_MAX - 1);
    out->heartbeat_interval_sec = 5;
    strncpy(out->log_level, "info", CONFIG_STR_MAX - 1);
}

bool vom_config_load(const char *filename, vom_config_t *out) {
    if (!filename || !out) return false;
    vom_config_defaults(out);

    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }
        char line[MAX_LINE_LEN];
    char current_section[MAX_NAME_LEN] = "";

    while (fgets(line, sizeof(line), file)) {
        strip_comment(line);
        char *s = trim_whitespace(line);
        if (*s == '\0') continue;

        if (s[0] == '[') {
            char *end = strchr(s, ']');
            if (end) {
                *end = '\0';
                char *sec = trim_whitespace(s + 1);
                strncpy(current_section, sec, MAX_NAME_LEN - 1);
                current_section[MAX_NAME_LEN - 1] = '\0';
            }
            continue;
        }

        char *eq = strchr(s, '=');
        if (!eq) continue;

        *eq = '\0';
        char *k = trim_whitespace(s);
        char *v = trim_whitespace(eq + 1);
        set_config_entry(out, current_section, k, v);
    }

    fclose(file);
    return true;
}
