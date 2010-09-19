#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>

#include "hiredis.h"

#define RESPONSE_OK         "200"
#define RESPONSE_TEMPFAIL   "400"
#define RESPONSE_FATAL      "500"


int handle_request(int s, int fd, char *value) {

    redisReply *reply;
    char replyStr[256] = "";
    int i;

    for(i=0; i<4; i++)
        value++;

    value[strlen(value)-1] = '\0';

    reply = redisCommand(fd, "GET %b", value, strlen(value)-1);
    if(reply->type != REDIS_REPLY_NIL) {
        if(reply->type == REDIS_REPLY_STRING)
            snprintf(replyStr, (size_t) strlen(RESPONSE_OK) + strlen(reply->reply) +3, "%s %s\n", RESPONSE_OK, reply->reply);
        else
            snprintf(replyStr, (size_t) strlen(RESPONSE_TEMPFAIL) + strlen(reply->reply) +3, "%s %s\n", RESPONSE_TEMPFAIL, reply->reply);

        fprintf(stdout, "Answering to client: %s", replyStr);
        fflush(stdout);
    }
    else {
        snprintf(replyStr, (size_t) strlen(RESPONSE_FATAL) + 16, "%s %s\n", RESPONSE_FATAL, "unknown entry");
    }

    send(s, replyStr, (size_t) strlen(replyStr) +1, 0);
    freeReplyObject(reply); 

    return 0;
}
