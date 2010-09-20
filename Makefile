LDFLAGS = -L./hiredis -lhiredis
CFLAGS = -Wall -W -I./hiredis -g
CC = gcc
LD = gcc

all: postfix-redis

postfix-redis: main.o client.o config.o
	$(LD) -o postfix-redis config.o main.o client.o $(LDFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

config.o: config.c
	$(CC) -c config.c $(CFLAGS)

client.o: client.c
	$(CC) -c client.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f postfix-redis *.o
