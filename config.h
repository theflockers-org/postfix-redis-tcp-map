#include <glib.h>

typedef struct config config;

struct config {

    /* tcp daemon */
    gchar *listen_address;
    gchar *registry_prefix;
    int   listen_port;

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
    int   pgsql_port;

    /* database (dbms)*/
    gchar *mysql_address;
    gchar *mysql_dbname;
    gchar *mysql_username;
    gchar *mysql_password;
    int   mysql_port;

    gchar *missing_registry_mysql_query;
    gchar *missing_registry_pgsql_query;
};
