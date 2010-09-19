LDFLAGS = -L./hiredis -lhiredis
CFLAGS = -Wall -W -I./hiredis
CC = gcc
LD = gcc

all: postfix-redis

postfix-redis: main.o client.o
	$(LD) -o postfix-redis main.o client.o $(LDFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

client.o: client.c
	$(CC) -c client.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f postfix-redir *.o
