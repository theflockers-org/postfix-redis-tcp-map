#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "hiredis.h"

#define LISTEN_ADDRESS  "10.1.1.1"    
#define LISTEN_PORT     6666
#define BACKLOG 1024

int handle_request(int s, int fd, char *value);

int main(void) {

    int    sock, s;
    int    optval = 1;
    char   buffer[1024];
    struct protoent *proto;
    struct sockaddr_in listen_addr, peer_addr;
    
    pid_t  pid;

    proto = getprotobyname("tcp");
    sock = socket(AF_INET, SOCK_STREAM, proto->p_proto);
    
    memset(&listen_addr, 0, sizeof(struct sockaddr));

    listen_addr.sin_family      = AF_INET;
    listen_addr.sin_port        = htons(LISTEN_PORT);
    listen_addr.sin_addr.s_addr = inet_addr(LISTEN_ADDRESS);

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));
    if(bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) != 0) {
        perror("Error binding socket.");
        exit(1);
    }
    if(listen(sock, 1024) != 0) {
        perror("Could not listen socket");
    }
    
    int fd;
    redisConnect(&fd, "127.0.0.1", 6379);

    for(;;) {
        int len = sizeof(peer_addr);
        if( (s = accept(sock, (struct sockaddr *)&peer_addr, (socklen_t *) &len) ) > 0) {
            pid = fork();
            if(pid == 0) {
                fprintf(stdout, "Forking pid: %i\n", pid);
                fflush(stdout);
                while( recv(s, buffer, sizeof(buffer), 0) > 0) {
                    handle_request(s, fd, buffer);
                    fprintf(stdout, "Buffer: %s\n", buffer);
                    fflush(stdout);
                    memset(&buffer, 0, sizeof(buffer));
                }
            }
        }
    }

    
}
