#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>

#include "hiredis.h"
#include "config.h"

#define RESPONSE_OK         "200"
#define RESPONSE_TEMPFAIL   "400"
#define RESPONSE_FATAL      "500"

extern config cfg;

int handle_request(int s, int fd, char *value) {

    redisReply *reply;
    char replyStr[256] = "";
    int i, ret = 0;

    for(i=0; i<4; i++)
        value++;

    value[strlen(value)-1] = '\0';

    reply = redisCommand(fd, "GET %b", value, strlen(value)-1);
    if(reply->type != REDIS_REPLY_NIL) {
        if(reply->type == REDIS_REPLY_STRING) {
            snprintf(replyStr, (size_t) strlen(RESPONSE_OK) + strlen(reply->reply) +3, "%s %s\n", RESPONSE_OK, reply->reply);
        } 
        else {
            fprintf(stdout, "Could not reconnect. Check your redis server");
            snprintf(replyStr, (size_t) strlen(RESPONSE_TEMPFAIL) + strlen(reply->reply) +3, "%s %s\n", RESPONSE_TEMPFAIL, reply->reply);
            ret = 1;
        }
        fprintf(stdout, "Answer to client: %s", replyStr);
    }

    else if(reply->type == REDIS_REPLY_ERROR) {
        fprintf(stdout, "trying reconnect..");
        reply = redisConnect(&fd, cfg.redis_address, cfg.redis_port);
        if(reply->type == REDIS_REPLY_ERROR) {
            fprintf(stdout, "Could not reconnect. Check your redis server");
        }
    }
    else {
        snprintf(replyStr, (size_t) strlen(RESPONSE_FATAL) + 16, "%s %s\n", RESPONSE_FATAL, "unknown entry");
    }
    fflush(stdout);
    send(s, replyStr, (size_t) strlen(replyStr) +1, 0);
    freeReplyObject(reply); 

    return ret;
}
