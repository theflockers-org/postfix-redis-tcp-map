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
 
#include "hiredis.h"

#define POSTFIX_RESPONSE_OK         "200"
#define POSTFIX_RESPONSE_TEMPFAIL   "400"
#define POSTFIX_RESPONSE_ERROR      "500"

#define TCP_MAPPER_DOMAIN_QUERY     "SELECT dominio, server FROM dominio WHERE dominio = \"%s\" LIMIT 1"


#define REDIS_POOL_SIZE             10

#ifdef USE_LIBEVENT
/* when accepting the connection */
void on_accept(int fd, short ev, void *arg);

/* when reading the file descriptor */
void on_read(int fd, short ev, void *arg);
#endif

int setnonblocking(int fd);

typedef struct redisPool redisPool;

struct redisPool {
    int size;
    redisContext *c[1024];
    int current;
    int next;
    int prev;
};

int redisPoolInit(redisPool *pool, char hostname[255], int poolsize);

int redis_lookup(char * response, redisPool *pool, char *key);
