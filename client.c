#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "hiredis.h"
#include "config.h"

#define RESPONSE_OK         "200"
#define RESPONSE_TEMPFAIL   "400"
#define RESPONSE_FATAL      "500"

extern config cfg;

int handle_request(int s, int fd) {

    redisReply *reply;
    char buffer[100] = "";
    char replyStr[256] = "";
    char cmd[4], key[255];
    int ret = 0;

    while(recv(s, buffer, sizeof(buffer), 0 ) < 0) {
        continue;
    }

    sscanf(buffer, "%s %s", cmd, key);
    reply = redisCommand(fd, "GET %b", key, strlen(key));
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
    if(send(s, replyStr, (size_t) strlen(replyStr), MSG_NOSIGNAL) == -1)
        ret = 0;

    freeReplyObject(reply); 

    return ret;
}
