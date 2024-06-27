/**
 *  Postfix Redis tcp map is a daemon to implement Postfixs tcp lookup table.
 *  Copyright (C) 2011  Leandro Mendes <theflockers at gmail dot com>
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
 
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <signal.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> /* inet_toa */

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

/* hiredis */
#include <hiredis/hiredis.h>

/* libevent */
#include <event.h>

/* local includes */
#include "config.h"
#include "tcp_mapper.h"

#ifdef HAS_PGSQL
/* pgsql */
#include <libpq-fe.h>
extern PGconn *pgsql;

/**
 * @name tcp_mapper_pgsql_query
 * @description Issue a query to pgsql
 * @param PGconn *pgsql
 * @param char *query
 * @return int
 */
int tcp_mapper_pgsql_query(PGconn *pgsql, char *query, char *result);
#endif

#ifdef HAS_MYSQL
/* mysql */
#include <mysql.h>
extern MYSQL *mysql;

/**
 * @name tcp_mapper_mysql_query
 * @description Issue a query to mysql
 * @param MYSQL *mysql
 * @param char *query
 * @return int
 */
int tcp_mapper_mysql_query(MYSQL *mysql, char *query, char *result);
#endif

#ifdef HAS_LDAP
/* ldap */
#include <ldap.h>
extern LDAP *ldap;

/**
 * @name tcp_mapper_ldap_query
 * @description Issue a search on ldap
 * @param LDAP *ldap
 * @param char *filter
 * @return int
 */
int tcp_mapper_ldap_search(LDAP *ldap, char *filter, char *result);
#endif

/* config */
extern config cfg; 

/**
 *  @struct client
 */
struct client {
    struct event ev_read;
};

/* redis */
extern redisPool redis_pool;

/**
 * @name redis_set
 * @description Inputs data onto redis
 * @param redisPool *pool
 * @param char *key
 * @param char *value
 * @return int
 */
int redis_set(redisPool *pool, char *key, char *val);

/**
 * @name on_read
 * @description Reads a fd and handle the request
 * @params int fd
 * @params short ev
 * @params void *arg
 * @return void
 */
void on_read(int fd, short ev, void *arg) {

    int    len;
    int    random_id = rand() % 999999;
    char   reqid[8] = "";

    char   cmd[4]   = "";
    char   key[64]  = "";
    char   buf[255] = "";

    char   mysqlQueryString[512] = "";
    char   pgsqlQueryString[512] = "";
    char   ldapSearchString[512] = "";

    char   *response = NULL;
    char   *result   = NULL;

    response = malloc(sizeof *response);
       result   = malloc(sizeof *result);
 
    struct client *client = (struct client *)arg;

    snprintf(reqid, 8, "%d", random_id);

    len = read(fd, buf, sizeof(buf));

    if (len == 0) {
        syslog(LOG_INFO, "Client disconnected");
        close(fd);
        event_del(&client->ev_read);
        free(client);
        return;
    } else if (len < 0) {
        syslog(LOG_INFO, "Socket failure, disconnecting client: %s",
            strerror(errno));

        close(fd);
        event_del(&client->ev_read);
        free(client);
        return;
    }

    typedef struct email {
        char user[255];
        char domain[255];
    } email;

    email parts;

    /* split the command and the key needed */
    sscanf(buf, "%s %s", cmd, key);

    int pos;
    char *buff = NULL;

    buff = malloc(sizeof *buff);
    memset(&parts, 0, sizeof(email));

    if((buff = strstr(key, "@") ) != NULL ) {
        pos = buff - key; 
        buff++;
        strncpy(parts.user, key, pos);
        snprintf(parts.domain, strlen(buff) +1, "%s", buff);
    } else {
        parts.domain[0] = '\0';
        parts.user[0] = '\0';
    }

    syslog(LOG_INFO, "request_id=%s msg=postfix request: (%s %s)", reqid, cmd, key);

    /* Lookup the key into redis and if is not found (!= 0)
     * try to lookup into MySQL or PGSQL.
     *
     * If exists, set into redis.
     *
     */
    char *replace_email_parts(char *key, char *buff, char *orig) {

        int i, c = 0;
        for(i = 0; i < strlen(orig); i++) {
            if(orig[i] == '%') {
                i++;
                switch(orig[i]) {
                    case 'd':
                        i++;
                        if(strlen(parts.domain) != 0) {
                            snprintf(&buff[c], (size_t) strlen(parts.domain) +1, "%s", parts.domain);
                        }
                        c = (strlen(parts.domain)) +c;
                    break;
                    case 'u':
                        i++;
                        if(strlen(parts.user) != 0) {
                            snprintf(&buff[c], (size_t) strlen(parts.user) +1, "%s", parts.user);
                        }
                        c = (strlen(parts.user)) +c;
                    break;
                    case 's':
                        i++;
                        snprintf(&buff[c], (size_t) strlen(key), "%s", key);
                        c = (strlen(key)) +c;
                    break;

                }
            }
            buff[c] = orig[i];
            c++;
        }
        buff[c] = '\0';

        return buff;
    }

    if(!cfg.mysql_enabled == 0) {
        char *mysql_missing_registry_query_buff = NULL;
        mysql_missing_registry_query_buff = (char *) malloc(1024);
        replace_email_parts(key, mysql_missing_registry_query_buff, cfg.missing_registry_mysql_query);
        snprintf(mysqlQueryString, (size_t) strlen(mysql_missing_registry_query_buff) +1, "%s", mysql_missing_registry_query_buff);
        free(mysql_missing_registry_query_buff);
    }

    if(!cfg.pgsql_enabled == 0) {
        char *pgsql_missing_registry_query_buff = NULL;
        pgsql_missing_registry_query_buff = (char *) malloc(1024);
        replace_email_parts(key, pgsql_missing_registry_query_buff, cfg.missing_registry_pgsql_query);
        snprintf(pgsqlQueryString, (size_t) strlen(pgsql_missing_registry_query_buff) +1, "%s", pgsql_missing_registry_query_buff);
        free(pgsql_missing_registry_query_buff);
    }

    if(!cfg.ldap_enabled == 0) {
        char *ldap_missing_registry_search_filter_buff = NULL;
        ldap_missing_registry_search_filter_buff = (char *) malloc(1024);
        replace_email_parts(key, ldap_missing_registry_search_filter_buff, cfg.ldap_search_filter);
        snprintf(ldapSearchString, (size_t) strlen(ldap_missing_registry_search_filter_buff) +1, "%s", ldap_missing_registry_search_filter_buff);
        free(ldap_missing_registry_search_filter_buff);
    }

    if(redis_lookup((char *) reqid, (char *) &response, &redis_pool, key) != 0) {

        char logline[255] = "";

        char *result          = NULL;
        char *backendResponse = NULL;

        result          = malloc(sizeof(result));
        backendResponse = malloc(sizeof(backendResponse));

        /* lookup MySQL, if enabled */
        if(!cfg.mysql_enabled == 0) {    
#ifdef HAS_MYSQL

            /* ping the mysql server, and reconnect */
            mysql_ping(mysql);

            if(tcp_mapper_mysql_query(mysql, mysqlQueryString, (char *) &result) > 0) {
                redis_set(&redis_pool, key, (char *) &result);

                snprintf((char *) &backendResponse, (strlen(POSTFIX_RESPONSE_OK) +
                            strlen((char *) &result)) +2,
                        "%s %s", POSTFIX_RESPONSE_OK, (char *) &result);

                /* logging success */
                snprintf(logline,
                    (size_t) strlen(TCP_MAPPER_LOG_LINE) +  strlen(reqid) + strlen(key) + strlen(POSTFIX_RESPONSE_OK) + 4,
                    TCP_MAPPER_LOG_LINE,
                    (char *) reqid, key, MYSQL_BACKEND_NAME, POSTFIX_RESPONSE_OK);
            } 
            else {
                snprintf((char *) &backendResponse, strlen(POSTFIX_RESPONSE_ERROR) + 15,
                    "%s %s", POSTFIX_RESPONSE_ERROR, "unknown entry");

                /* logging error */
                snprintf(logline,
                    (size_t) strlen(TCP_MAPPER_LOG_LINE) +  strlen(reqid) + strlen(key) + strlen(POSTFIX_RESPONSE_ERROR) + 4,
                    TCP_MAPPER_LOG_LINE,
                    (char *) reqid, key, MYSQL_BACKEND_NAME, POSTFIX_RESPONSE_ERROR);

            }
            syslog(LOG_INFO, logline);
#endif
        }

        /* lookup LDAP if enabled */
        else if(!cfg.ldap_enabled == 0) {
#ifdef HAS_LDAP

            if(tcp_mapper_ldap_search(ldap, ldapSearchString, (char *) &result) > 0) {
                redis_set(&redis_pool, key, (char *) &result);

                snprintf((char *) &backendResponse, (size_t) strlen(POSTFIX_RESPONSE_OK) +4, "%s OK", POSTFIX_RESPONSE_OK);

                /* logging success */
                snprintf(logline,
                    (size_t) strlen(TCP_MAPPER_LOG_LINE) + strlen(key) + strlen(POSTFIX_RESPONSE_OK) + 4,
                    TCP_MAPPER_LOG_LINE,
                    (char *) reqid, key, LDAP_BACKEND_NAME, POSTFIX_RESPONSE_OK);
            }
            else {
                snprintf((char *) &backendResponse, strlen(POSTFIX_RESPONSE_ERROR) + 15,
                        "%s %s", POSTFIX_RESPONSE_ERROR, "unknown entry");

                /* logging error */
                snprintf(logline,
                    (size_t) strlen(TCP_MAPPER_LOG_LINE) + strlen(reqid) + strlen(key) + strlen(POSTFIX_RESPONSE_ERROR) + 4,
                    TCP_MAPPER_LOG_LINE,
                    (char *) reqid, key, LDAP_BACKEND_NAME, POSTFIX_RESPONSE_ERROR);
            }
            syslog(LOG_INFO, logline);
#endif
        }

        /* lookup PgSQL, if enabled */
        else if(!cfg.pgsql_enabled == 0){
#ifdef HAS_PGSQL

            /* check if pgsql is still alive */
            if(PQstatus(pgsql) != CONNECTION_OK)  {
                PQreset(pgsql);
            }

            if(tcp_mapper_pgsql_query(pgsql, pgsqlQueryString,(char *) &result) > 0) {

                redis_set(&redis_pool, key, (char *) &result);

                snprintf((char *) &backendResponse, (strlen(POSTFIX_RESPONSE_OK) +
                            strlen((char *) &result) ) +3,
                        "%s %s", POSTFIX_RESPONSE_OK, (char *) &result);

                /* logging success */
                snprintf(logline,
                    (size_t) strlen(TCP_MAPPER_LOG_LINE) +  strlen(reqid) + strlen(key) + strlen(POSTFIX_RESPONSE_OK) + 4,
                    TCP_MAPPER_LOG_LINE,
                    (char *) reqid, key, PGSQL_BACKEND_NAME, POSTFIX_RESPONSE_OK);
            }
            else {
                snprintf((char *) &backendResponse, strlen(POSTFIX_RESPONSE_ERROR) + 16,
                        "%s %s", POSTFIX_RESPONSE_ERROR, "unknown entry");

                /* logging error */
                snprintf(logline,
                    (size_t) strlen(TCP_MAPPER_LOG_LINE) +  strlen(reqid) + strlen(key) + strlen(POSTFIX_RESPONSE_ERROR) + 4,
                    TCP_MAPPER_LOG_LINE,
                    (char *) reqid, key, PGSQL_BACKEND_NAME, POSTFIX_RESPONSE_ERROR);

            }
            syslog(LOG_INFO, logline);
#endif
        }
        else  {
            snprintf((char *) &backendResponse, strlen(POSTFIX_RESPONSE_ERROR) + 16,
                        "%s %s", POSTFIX_RESPONSE_ERROR, "unknown entry");

            /* logging error */
            snprintf(logline,
                (size_t) strlen(TCP_MAPPER_LOG_LINE) + strlen(reqid) + strlen(key) + strlen(POSTFIX_RESPONSE_ERROR) + 7,
                TCP_MAPPER_LOG_LINE,
                (char *)reqid, key, "none",  POSTFIX_RESPONSE_ERROR);
        }

        snprintf((char *)&response, (size_t) strlen((char *)&backendResponse) +3, "%s\n", (char *) &backendResponse);
        syslog(LOG_INFO, "reply sent to client: (%s)", (char *) &backendResponse);
    }

    send(fd, &response, (size_t) strlen((const char *) &response), MSG_NOSIGNAL);
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 */
void
on_accept(int fd, short ev, void *arg)
{
    int client_fd;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct client *client;

    /* Accept the new connection. */
    client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        warn("accept failed");
        return;
    }

    /* Set the client socket to non-blocking mode. */
    if (setnonblocking(client_fd) < 0)
        syslog(LOG_INFO, "failed to set client socket non-blocking");

    /* We've accepted a new client, allocate a client object to
     * maintain the state of this client. */
    client = calloc(1, sizeof(*client));
    if (client == NULL)
        err(1, "malloc failed");

    /* Setup the read event, libevent will call on_read() whenever
     * the clients socket becomes read ready.  We also make the
     * read event persistent so we don't have to re-add after each
     * read. */
    event_set(&client->ev_read, client_fd, EV_READ|EV_PERSIST, on_read, client);

    /* Setting up the event does not activate, add the event so it
     * becomes active. */

    event_add(&client->ev_read, NULL);

    syslog(LOG_INFO, "Accepted connection from %s\n",
               inet_ntoa(client_addr.sin_addr));
}
