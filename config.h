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
 
#include <glib.h>

typedef struct config config;

struct config {

    /* tcp daemon */
    gchar *listen_address;
    gchar *registry_prefix;
    int   listen_port;
    int   expire_seconds;

    /* redis */
    gchar *redis_address;
    gchar *redis_db_index;
    int   redis_port;
    int   redis_reload_time;

    /* database (dbms)*/
    gchar *pgsql_address;
    gchar *pgsql_dbname;
    gchar *pgsql_username;
    gchar *pgsql_password;
    gboolean  pgsql_enabled;
    int   pgsql_port;

    /* database (dbms)*/
    gchar *mysql_address;
    gchar *mysql_dbname;
    gchar *mysql_username;
    gchar *mysql_password;
    gboolean  mysql_enabled;
    int   mysql_port;

    /* ldap */
    gchar *ldap_uri;
    gchar *ldap_bind_dn;
    gchar *ldap_bind_pw;
    gchar *ldap_base;
    gchar *ldap_search_filter;
    gchar *ldap_result_attr;
    gboolean  ldap_enabled;


    gchar *missing_registry_mysql_query;
    gchar *missing_registry_pgsql_query;
};
