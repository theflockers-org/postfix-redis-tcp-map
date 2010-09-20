/* config.h */

typedef struct config config;

struct config {
    char listen_address[255];
    char redis_address[255];
    int  listen_port;
    int  redis_port;
    int  timeout;
    int  redis_timeout;
};
/*
# config file

listen_address = 10.1.1.1
listen_port = 6666
timeout = 5

redis_address = 10.1.1.1
redis_port = 6379
redis_timeout = 5
*/
