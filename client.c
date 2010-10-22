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

/* libevent */
#include <event.h>

/* local includes */
#include "hiredis.h"
#include "config.h"
#include "tcp_mapper.h"


/* pgsql */
#include <libpq-fe.h>
extern PGconn *pgsql;

/* mysql */
#include <mysql.h>
extern MYSQL *mysql;

/* config */
extern config cfg; 

/* redis */
extern redisPool redis_pool;

/**
 *  @struct client
 */
struct client {
	struct event ev_read;
};

/**
 * @name tcp_mapper_mysql_query
 * @description Issue a query to mysql
 * @param MYSQL *mysql
 * @param char *query
 * @return int
 */
int tcp_mapper_mysql_query(MYSQL *mysql, char *query, char *result);

/**
 * @name tcp_mapper_pgsql_query
 * @description Issue a query to pgsql
 * @param PGconn *pgsql
 * @param char *query
 * @return int
 */
int tcp_mapper_pgsql_query(PGconn *pgsql, char *query, char *result);


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

    char   cmd[16] = "";
    char   key[64] = "";
	char   buf[256];

    char   mysqlQueryString[255];
    char   pgsqlQueryString[255];
    char   response[255] = "";
    char   *result;
 
	struct client *client = (struct client *)arg;

    memset(&buf, 0, sizeof(buf));

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

    /* split the command and the key needed */
    sscanf(buf, "%s %s", cmd, key);

    syslog(LOG_INFO, "Postfix request: (%s %s)", cmd, key);

    /* initialize */
    memset(&response, 0, sizeof(response));
    memset(&result,   0, sizeof(result));

    /* Lookup the key into redis and if is not found (!= 0)
     * try to lookup into MySQL or PGSQL.
     *
     * If exists, set into redis.
     *
     */
    if(redis_lookup((char *) &response, &redis_pool, key) != 0) {
        syslog(LOG_INFO, "Missing key (%s) checking SQL", key);

        /* ping the mysql server, and reconnect */
        mysql_ping(mysql);

        /* check if pgsql is still alive */
        if(PQstatus(pgsql) != CONNECTION_OK)  {
            PQreset(pgsql);
        }

        /* lookup MySQL */
        snprintf(mysqlQueryString, strlen(cfg.missing_registry_mysql_query) + strlen(key)+ 1, 
                cfg.missing_registry_mysql_query, key);

        snprintf(pgsqlQueryString, strlen(cfg.missing_registry_pgsql_query) + strlen(key)+ 1, 
                cfg.missing_registry_pgsql_query, key);
     
        if(tcp_mapper_mysql_query(mysql, mysqlQueryString, (char *) &result) > 0) {
            redis_set(&redis_pool, key, (char *) &result);
            snprintf( (char *) &response, ( strlen(POSTFIX_RESPONSE_OK) +
                    strlen((char *) &result) ) +3,
                    "%s %s\n", POSTFIX_RESPONSE_OK, (char *) &result);

            syslog(LOG_INFO, "Key (%s) found on MySQL", key);
        } 
        else if(tcp_mapper_pgsql_query(pgsql, pgsqlQueryString,(char *) &result) > 0) {

            redis_set(&redis_pool, key, (char *) &result);

            snprintf( (char *) &response, ( strlen(POSTFIX_RESPONSE_OK) +
                    strlen((char *) &result) ) +3,
                    "%s %s\n", POSTFIX_RESPONSE_OK, (char *) &result);

            syslog(LOG_INFO, "Key (%s) found on PostgreSQL", key);
        }
        else {
            snprintf((char *)&response, strlen(POSTFIX_RESPONSE_ERROR) + 16,
                    "%s %s\n", POSTFIX_RESPONSE_ERROR, "unknown entry");

            syslog(LOG_INFO, "Key (%s) not found", key);
        }
    }
    send(fd, &response, (size_t)strlen((const char *)&response), MSG_NOSIGNAL);
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
	event_set(&client->ev_read, client_fd, EV_READ|EV_PERSIST, on_read, 
	    client);

	/* Setting up the event does not activate, add the event so it
	 * becomes active. */

	event_add(&client->ev_read, NULL);

	syslog(LOG_INFO, "Accepted connection from %s\n",
               inet_ntoa(client_addr.sin_addr));
}
