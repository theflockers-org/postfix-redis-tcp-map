#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sysexits.h>

#include "hiredis.h"
#include "config.h"

#define BACKLOG     1024
#define MAX_EVENTS  50

int handle_request(int s, int fd, char *value);
config parseConfig(char *filename);

config cfg;

int setnonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return 0;
}


int main(void) {

    int    sock, s, fd, nfds, efd;
    int    n, len;
    int    optval = 1;
    char   buffer[1024];
    struct protoent *proto;
    struct epoll_event ev, events[MAX_EVENTS];
    struct sockaddr_in listen_addr, peer_addr;
    redisReply *reply;
    
    cfg = parseConfig("postfix-redis.cfg");

    proto = getprotobyname("tcp");
    sock = socket(AF_INET, SOCK_STREAM, proto->p_proto);
    
    memset(&listen_addr, 0, sizeof(struct sockaddr));

    listen_addr.sin_family      = AF_INET;
    listen_addr.sin_port        = htons(cfg.listen_port);
    listen_addr.sin_addr.s_addr = inet_addr(cfg.listen_address);

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));
    if(bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) != 0) {
        perror("Error binding socket.");
        exit(1);
    }
    if(listen(sock, 1024) != 0) {
        perror("Could not listen socket");
    }
    
    reply = redisConnect(&fd, cfg.redis_address, cfg.redis_port);
    if(reply != NULL) {
        perror("redis_error");
        exit(EXIT_FAILURE);
    }

    efd = epoll_create(MAX_EVENTS);
    if(efd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    ev.events  = EPOLLIN;
    ev.data.fd = sock;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ev) == -1) {
        perror("epoll_ctl: sock");
        exit(EXIT_FAILURE);
    }
    // socklen_t
    len = sizeof(peer_addr);

    for(;;) {
        nfds = epoll_wait(efd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_pwait");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; ++n) {
            if (events[n].data.fd == sock) {
                s = accept(sock, (struct sockaddr *)&peer_addr, (socklen_t *) &len);
                if (s == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                setnonblocking(s);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = s;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, s, &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else {
                while( recv(s, buffer, sizeof(buffer), 0) > 0) {
                    if(handle_request(events[n].data.fd, fd, buffer)) {
                        reply = redisConnect(&fd, cfg.redis_address, cfg.redis_port);
                        if(reply != NULL)
                            perror("redis_error");
                    }
                    fprintf(stdout, "Buffer: %s\n", buffer);
                    fflush(stdout);
                    memset(&buffer, 0, sizeof(buffer));
//                    usleep(100);
                }
            }
        }
    }
}
