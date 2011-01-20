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
