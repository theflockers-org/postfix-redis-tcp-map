VERSION = 0.1-1
PACKAGE = postfix-tcp-redis-map
LDFLAGS = -L./hiredis -lhiredis -levent -lc -lmysqlclient -lpq `pkg-config --libs glib-2.0`
CFLAGS = -Wall -I./hiredis -g -DUSE_LIBEVENT -I/usr/include/mysql -I/usr/include/postgresql `pkg-config --cflags glib-2.0`
PREFIX = /usr
SBINDIR = $(PREFIX)/sbin
SYSCONFDIR = /etc/locaweb/postfix-redis/

CC = gcc
LD = gcc

all: postfix-redis package

postfix-redis: main.o client.o config.o redis.o mysql.o pgsql.o
	$(MAKE) -C hiredis/
	$(LD) -o postfix-redis config.o main.o client.o redis.o mysql.o pgsql.o $(LDFLAGS) $(CFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS) $(LDFLAGS)

config.o: config.c
	$(CC) -c config.c $(CFLAGS)

client.o: client.c
	$(CC) -c client.c -o client.o $(CFLAGS) $(LDFLAGS)

redis.o: redis.c
	$(CC) -c redis.c $(CFLAGS) $(LDFLAGS)

mysql.o: mysql.c
	$(CC) -c mysql.c $(CFLAGS) $(LDFLAGS)

pgsql.o: pgsql.c
	$(CC) -c pgsql.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f postfix-redis *.o
	cd hiredis; $(MAKE) clean
	rm -rf debian
	rm *.deb

install:
	install -m 0755 hiredis/libhiredis.so /usr/lib/
	install -m 0700 -s postfix-redis $(SBINDIR)
	install -b -D -m 0600 postfix-redis.cfg.example $(SYSCONFDIR)/postfix-redis.cfg

package:
	mkdir -p debian/DEBIAN
	install -D -m 0700 postfix-redis debian/usr/sbin/postfix-redis
	install -D -m 0755 postfix-redis.cfg.example debian/etc/postfix-redis.cfg
	sed "s/{ARCH}/`dpkg --print-architecture`/g" debian_control.in > debian/DEBIAN/control
	dpkg-deb --build debian
	mv debian.deb $(PACKAGE)_$(VERSION)_`dpkg --print-architecture`.deb

