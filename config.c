/**
 *  Postfix Redis tcp map is a daemon to implement Postfixs tcp lookup table.
 *  Copyright (C) 2011  Leandro Mendes <leandro at gmail dot com>
 *  ---------------------------------------------------------------------------
 *  This file is part of postfix-redis-tcp-map.
 *
 *  postfix-redis-tcp-map is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  postfix-redis-tcp-map is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with postfix-redis-tcp-map.  If not, see <https://www.gnu.org/licenses/>.
 */
 
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
    cfg.expire_seconds  = strtol(g_key_file_get_string(keyfile, "main", "expire_seconds", NULL), NULL, 10);

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
    cfg.mysql_enabled  = g_key_file_get_boolean(keyfile, "mysql", "enabled", NULL);
    cfg.missing_registry_mysql_query = g_key_file_get_string(keyfile, "mysql", "missing_registry_query", NULL);

    /* postgresql configuration */
    cfg.pgsql_address  = g_key_file_get_string(keyfile, "pgsql", "address", NULL);
    cfg.pgsql_port     = g_key_file_get_integer(keyfile, "pgsql", "port", NULL);
    cfg.pgsql_username = g_key_file_get_string(keyfile, "pgsql", "username", NULL);
    cfg.pgsql_password = g_key_file_get_string(keyfile, "pgsql", "password", NULL);
    cfg.pgsql_dbname   = g_key_file_get_string(keyfile, "pgsql", "dbname", NULL);
    cfg.pgsql_enabled  = g_key_file_get_boolean(keyfile, "pgsql", "enabled", NULL);
    cfg.missing_registry_pgsql_query = g_key_file_get_string(keyfile, "pgsql", "missing_registry_query", NULL);

    cfg.ldap_uri         = g_key_file_get_string(keyfile, "ldap", "uri", NULL);
    cfg.ldap_bind_dn     = g_key_file_get_string(keyfile, "ldap", "bind_dn", NULL);
    cfg.ldap_bind_pw     = g_key_file_get_string(keyfile, "ldap", "bind_pw", NULL);
    cfg.ldap_base        = g_key_file_get_string(keyfile, "ldap", "base", NULL);
    cfg.ldap_enabled     = g_key_file_get_boolean(keyfile, "ldap", "enabled", NULL);
    cfg.ldap_search_filter = g_key_file_get_string(keyfile, "ldap", "search_filter", NULL);
    cfg.ldap_result_attr = g_key_file_get_string(keyfile, "ldap", "result_attr", NULL);



    g_key_file_free(keyfile);

    return cfg;
}
