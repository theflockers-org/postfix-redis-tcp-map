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

    int i;
    // char currentdb[2];
    redisReply *reply;

    pool->size = poolsize;
    // reply = redisConnect(&fd, hostname, cfg.redis_port);
    redisContext *c = redisConnect(hostname, cfg.redis_port);

    if (c == NULL || c->err) {
        if (c) {
            printf("Error: %s\n", c->errstr);
        } else {
            printf("Can't allocate redis context\n");
        }
    }

    for(i = 0; i < poolsize; i++) {
        if(pool->c[i])
           redisFree(pool->c[i]);

        pool->c[i] = redisConnect(hostname, cfg.redis_port);
        if (pool->c[i] == NULL || pool->c[i]->err) {
            if (pool->c[i]) {
                printf("Error: %s\n", pool->c[i]->errstr);
            } else {
                printf("Can't allocate redis context\n");
            }
        }
        reply = redisCommand(pool->c[i], "SELECT %s", cfg.redis_db_index);
        freeReplyObject(reply);
    }

    if(cfg.expire_seconds == 0) {
        reply = redisCommand(pool->c[0], "KEYS *");
        if(reply->type == REDIS_REPLY_ARRAY)  {
            if(reply->elements == 0 ) {
                printf("Empty database, i'm fuc**ng off\n");
                syslog(LOG_ERR, "Empty database, i'm fuc**ng off");
                freeReplyObject(reply);
                return -1;
            }
        }
        freeReplyObject(reply);
    }
    pool->current = 0;
    return 0;
}

redisContext * redisPoolGetCurrent(redisPool *pool) {

    if(pool->current == pool->size)
       pool->current = 0;
    int cur = pool->current;

    pool->prev = pool->current;
    pool->current++;
    pool->next = pool->current + 1;

    return pool->c[cur];
}

int redis_set(redisPool *pool, char *key, char *val) {

    redisReply *reply;
    redisContext *c;
    c = redisPoolGetCurrent(pool);

    // up to 10 years expire time
    char str_seconds[10] = "";

    reply = redisCommand(c, "SET %b:%b %b",
                    cfg.registry_prefix, strlen(cfg.registry_prefix), 
                    key, strlen(key), 
                    val, strlen(val));
    
    freeReplyObject(reply);
    if(cfg.expire_seconds > 0) {
        sprintf(str_seconds, "%d",  cfg.expire_seconds);
        reply = redisCommand(c, "EXPIRE %b:%b %b",
                    cfg.registry_prefix, strlen(cfg.registry_prefix),
                    key, strlen(key), 
                    str_seconds, strlen(str_seconds));
        freeReplyObject(reply);
    }
    return 0;
}

int redis_lookup(char *response, redisPool *pool, char *key) {

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
                + 23, "%s %s\n", POSTFIX_RESPONSE_TEMPFAIL, "reset database error");
        }
        /* reset the counter */
        time(&reset);
        init_time = (int) reset;
    }

    redisContext *c = redisPoolGetCurrent(pool);
    
    /* running the string cfg.registry_prefix to know what kinds of service i'll answer. */
    int i, scount = 0;
    for(i = 0; i<=strlen(cfg.registry_prefix); i++) {
        /* when i found a comma or a string ending, issue the command */
        if((cfg.registry_prefix[i] == ',') || cfg.registry_prefix[i] == '\0') {
            registry[scount] = '\0';
            reply = redisCommand(c, "GET %b:%b", registry, strlen(registry), 
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
                + strlen(reply->str) +3, "%s %s\n", POSTFIX_RESPONSE_TEMPFAIL, reply->str);
        }
        else {
            /* if reconnected, select current */
            c = redisPoolGetCurrent(&redis_pool);

            /* re-send the request */
            reply = redisCommand(c, "GET %b:%b", registry, strlen(registry), key, strlen(key));
            snprintf(replyStr, (size_t) strlen(POSTFIX_RESPONSE_OK) 
                + strlen(reply->str) +4, "%s %s\n", POSTFIX_RESPONSE_OK, reply->str);
        }
    }
    else {

        /* time the reply to client */
        if(reply->type != REDIS_REPLY_NIL) {
            /* if is a string (in this case, always) */
            if(reply->type == REDIS_REPLY_STRING) {
                snprintf(replyStr, (size_t) strlen(POSTFIX_RESPONSE_OK) 
                        + strlen(reply->str) +3, "%s %s", POSTFIX_RESPONSE_OK, reply->str);
            }
        }
        /* if the entry does not exists, reply 500 to client */
        /* update: return -1, to query a dbms */
        else {
            freeReplyObject(reply);
            return -1;
        }

    }
    syslog(LOG_INFO, "Reply sent to client: (%s)", replyStr);

    /* freeing response */
    freeReplyObject(reply);

    /* pasting into *response */
    snprintf(response, (size_t) strlen(replyStr) +2, "%s\n", replyStr);

    return 0;
}
