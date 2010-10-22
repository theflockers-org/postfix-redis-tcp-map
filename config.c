/**
 * @file config.c
 * @brief Simple config file parser.
 * @author Leandro Mendes <leandro.mendes@locaweb.com.br>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <glib.h>
#include <errno.h>
#include <error.h>

#include "config.h"

/**
 * @name parseConfig
 * @description Open and parse a name filename 
 * @param char *filename
 * @return config cfg;
 */
config parseConfig(char *filename) {

    GKeyFile *keyfile;

    keyfile = g_key_file_new();

    if(! (g_key_file_load_from_file(keyfile, filename, 0, NULL))){
        printf("Error opening %s: %s\n", filename, strerror(errno));
        exit(-1); 
    }

    /* config struct */
    config cfg;

    memset(&cfg, 0, sizeof(config));

    /* tcp daemon main configuration */
    cfg.listen_address  = g_key_file_get_string(keyfile, "main",  "address", NULL);
    cfg.listen_port     = g_key_file_get_integer(keyfile, "main",  "port", NULL);
    cfg.registry_prefix = g_key_file_get_string(keyfile, "main",  "registry_prefix", NULL);

    /* redis configuration */
    cfg.redis_address     = g_key_file_get_string(keyfile, "redis", "address", NULL);
    cfg.redis_port        = g_key_file_get_integer(keyfile, "redis", "port", NULL);
    cfg.redis_db_index    = g_key_file_get_string(keyfile, "redis", "db_index", NULL);
    cfg.redis_reload_time = g_key_file_get_integer(keyfile, "redis", "reload_time", NULL);

    /* mysql configuration */
    cfg.mysql_address  = g_key_file_get_string(keyfile, "mysql", "address", NULL);
    cfg.mysql_port     = g_key_file_get_integer(keyfile, "mysql", "port", NULL);
    cfg.mysql_username = g_key_file_get_string(keyfile, "mysql", "username", NULL);
    cfg.mysql_password = g_key_file_get_string(keyfile, "mysql", "password", NULL);
    cfg.mysql_dbname   = g_key_file_get_string(keyfile, "mysql", "dbname", NULL);
    cfg.missing_registry_mysql_query = g_key_file_get_string(keyfile, "mysql", "missing_registry_query", NULL);

    /* postgresql configuration */
    cfg.pgsql_address  = g_key_file_get_string(keyfile, "pgsql", "address", NULL);
    cfg.pgsql_port     = g_key_file_get_integer(keyfile, "pgsql", "port", NULL);
    cfg.pgsql_username = g_key_file_get_string(keyfile, "pgsql", "username", NULL);
    cfg.pgsql_password = g_key_file_get_string(keyfile, "pgsql", "password", NULL);
    cfg.pgsql_dbname   = g_key_file_get_string(keyfile, "pgsql", "dbname", NULL);
    cfg.missing_registry_pgsql_query = g_key_file_get_string(keyfile, "pgsql", "missing_registry_query", NULL);

    g_key_file_free(keyfile);

    return cfg;
}
