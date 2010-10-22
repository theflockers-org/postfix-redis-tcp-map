#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>

/* local definitions */
#include "hiredis.h"
#include "config.h"

#include "tcp_mapper.h"

extern config cfg;

extern redisPool redis_pool;

extern int init_time;

int redisPoolInit(redisPool *pool, char hostname[255], int poolsize)  {

    int i, fd;
    char currentdb[2];
    redisReply *reply;

    pool->size = poolsize;
    reply = redisConnect(&fd, hostname, cfg.redis_port);

    if(reply != NULL) {
        return -1;
    }

    reply = redisCommand(fd, "GET CURRENTDB");

    if(reply->type == REDIS_REPLY_STRING) {
        snprintf(currentdb, strlen(reply->reply) +1, "%s", reply->reply);
    }

    if(atoi(currentdb) == 0) {
        printf("Current db not configured\n");
        syslog(LOG_ERR, "Current db not configured");
        return -1;
    }
        
    syslog(LOG_INFO, "Current database selected: %s", reply->reply);

    freeReplyObject(reply);
    close(fd);

    for(i = 0; i < poolsize; i++) {
        if(pool->fd[i])
           close(pool->fd[i]);

        reply = redisConnect(&pool->fd[i], hostname, cfg.redis_port);
        if(reply != NULL) {
            freeReplyObject(reply);
            return -1;
        }
    
        reply = redisCommand(pool->fd[i], "SELECT %s", currentdb);
        freeReplyObject(reply);
    }

    reply = redisCommand(pool->fd[0], "KEYS *");
    if(reply->type == REDIS_REPLY_ARRAY)  {
        if(reply->elements == 0 ) {
            printf("Empty database, i'm fuc**ng off\n");
            syslog(LOG_ERR, "Empty database, i'm fuc**ng off");
            freeReplyObject(reply);
            return -1;
        }
    }
    freeReplyObject(reply);
    pool->current = 0;
    return 0;
}

int redisPoolGetCurrent(redisPool *pool) {

    if(pool->current == pool->size)
       pool->current = 0;
    int cur = pool->current;

    pool->prev = pool->current;
    pool->current++;
    pool->next = pool->current + 1;

    return pool->fd[cur];
}

int redis_set(redisPool *pool, char *key, char *val) {

    redisReply *reply;
    int fd;

    fd = redisPoolGetCurrent(pool);

    reply = redisCommand(fd, "SET %b:%b %b",
                    cfg.registry_prefix, strlen(cfg.registry_prefix), 
                    key, strlen(key), 
                    val, strlen(val));

    freeReplyObject(reply);

    return 0;
}

int redis_lookup(char *response, redisPool *pool, char *key) {

    int        fd;
    redisReply *reply;
    char       replyStr[64] = "";
    char       registry[64] = "";
    time_t     client_time;
    time_t     reset;
    int        rest;

    time(&client_time);

    /* if the event time is greater then the init_time */
    rest = ((int) client_time - init_time);

    if(rest > cfg.redis_reload_time) {
        syslog(LOG_INFO, "%i passed. Realoading database\n", cfg.redis_reload_time);
        if(redisPoolInit(&redis_pool, cfg.redis_address, REDIS_POOL_SIZE) == -1) {
            snprintf(replyStr, (size_t) strlen(POSTFIX_RESPONSE_TEMPFAIL) 
                + 20, "%s %s\n", POSTFIX_RESPONSE_TEMPFAIL, "reset database error");
        }
        /* reset the counter */
        time(&reset);
        init_time = (int) reset;
    }

    fd = redisPoolGetCurrent(pool);
    
    /* running the string cfg.registry_prefix to know what kinds of service i'll answer. */
    int i, scount = 0;
    for(i = 0; i<=strlen(cfg.registry_prefix); i++) {
        /* when i found a comma or a string ending, issue the command */
        if((cfg.registry_prefix[i] == ',') || cfg.registry_prefix[i] == '\0') {
            registry[scount] = '\0';
            reply = redisCommand(fd, "GET %b:%b", registry, strlen(registry), 
                    key, strlen(key));

            /* if the reply is not nil, break the loop. 
             * Case is nil, continue until the end */
            if(reply->type != REDIS_REPLY_NIL)
                break;

            scount = 0;
            registry[0] = '\0';

            continue;
        }
        registry[scount] = cfg.registry_prefix[i];
        scount++;
    }

    if(reply->type == REDIS_REPLY_ERROR) {
        /* if disconected, try reconnect */
        if(redisPoolInit(&redis_pool, cfg.redis_address, REDIS_POOL_SIZE) == -1) {
            snprintf(replyStr, (size_t) strlen(POSTFIX_RESPONSE_TEMPFAIL) 
                + strlen(reply->reply) +3, "%s %s\n", POSTFIX_RESPONSE_TEMPFAIL, reply->reply);
        }
        else {
            /* if reconnected, select current */
            fd = redisPoolGetCurrent(&redis_pool);

            /* re-send the request */
            reply = redisCommand(fd, "GET %b:%b", registry, strlen(registry), key, strlen(key));
            snprintf(replyStr, (size_t) strlen(POSTFIX_RESPONSE_OK) 
                + strlen(reply->reply) +3, "%s %s\n", POSTFIX_RESPONSE_OK, reply->reply);
        }
    }
    else {

        /* time the reply to client */
        if(reply->type != REDIS_REPLY_NIL) {
            /* if is a string (in this case, always) */
            if(reply->type == REDIS_REPLY_STRING) {
                snprintf(replyStr, (size_t) strlen(POSTFIX_RESPONSE_OK) 
                        + strlen(reply->reply) +3, "%s %s", POSTFIX_RESPONSE_OK, reply->reply);
            }
        }
        /* if the entry does not exists, reply 500 to client */
        /* update: return -1, to query a dbms */
        else {
            freeReplyObject(reply);
            return -1;
            /*snprintf(replyStr, (size_t) strlen(POSTFIX_RESPONSE_ERROR) 
                + 16, "%s %s", POSTFIX_RESPONSE_ERROR, "unknown entry");*/
        }

    }
    syslog(LOG_INFO, "Reply sent to client: (%s)", replyStr);

    /* freeing response */
    freeReplyObject(reply);

    /* pasting into *response */
    sprintf(response, "%s\n", replyStr);

    return 0;
}
