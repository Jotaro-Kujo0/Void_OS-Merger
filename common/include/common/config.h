#ifndef VOM_COMMON_CONFIG_H
#define VOM_COMMON_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_STR_MAX 128

/* Authoritative flat configuration container profile */
typedef struct vom_config {
    /* [cluster] section */
    char cluster_name[CONFIG_STR_MAX];
    
    //master. section
    char master_endpoint[CONFIG_STR_MAX];
    
    //worker section
    char worker_id[CONFIG_STR_MAX];
    uint32_t heartbeat_interval_sec;
    char advertised_capabilities[CONFIG_STR_MAX];
    
    /* [logging] section */
    char log_level[CONFIG_STR_MAX];
} vom_config_t;

/* Global Configurations API Lifecycle Hooks */
void vom_config_defaults(vom_config_t *out);
bool vom_config_load(const char *path, vom_config_t *out);

#endif /* VOM_COMMON_CONFIG_H */
