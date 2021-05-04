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
