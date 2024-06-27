VERSION = 0.2
PACKAGE = postfix-redis-tcp-map
LIBS = -lhiredis -levent -lc `pkg-config --libs glib-2.0` $(LDFLAGS)
CPPFLAGS = -Wall -I/usr/include/hiredis -g -DUSE_LIBEVENT `pkg-config --cflags glib-2.0` $(CFLAGS)
PREFIX = /usr
SBINDIR = $(PREFIX)/sbin
SYSCONFDIR = /etc/postfix-redis/

CC = gcc
LD = gcc

all: package postfix-redis

postfix-redis: main.o client.o config.o redis.o mysql.o pgsql.o ldap.o
	$(LD) -o postfix-redis config.o main.o client.o redis.o mysql.o pgsql.o ldap.o $(LIBS) $(CPPFLAGS) 

main.o: main.c
	$(CC) -c main.c $(CPPFLAGS) $(LIBS)

config.o: config.c
	$(CC) -c config.c $(CPPFLAGS)

client.o: client.c
	$(CC) -c client.c -o client.o $(CPPFLAGS) $(LIBS)

redis.o: redis.c
	$(CC) -c redis.c $(CPPFLAGS) $(LIBS)

mysql.o: mysql.c
	$(CC) -c mysql.c $(CPPFLAGS) $(LIBS)

pgsql.o: pgsql.c
	$(CC) -c pgsql.c $(CPPFLAGS) $(LIBS)

ldap.o: ldap.c
	$(CC) -c ldap.c $(CPPFLAGS) $(LIBS)

clean:
	rm -f postfix-redis *.o
	rm -rf SOURCES tmp

install:
	install -m 0700 -s postfix-redis $(SBINDIR)
	install -b -D -m 0600 postfix-redis.cfg.example $(SYSCONFDIR)/postfix-redis.cfg

package:
	[ -d SOURCES ] || mkdir SOURCES
	/usr/bin/tar -czf SOURCES/${PACKAGE}-${VERSION}.tar.gz *.c *.h *.spec \
		*.sample *.service *.md *py COPYING.LESSER Makefile

deb:
	mkdir -p debian/DEBIAN
	install -D -m 0700 postfix-redis debian/usr/sbin/postfix-redis
	install -D -m 0755 postfix-redis.cfg.example debian/etc/postfix-redis.cfg
	#install -D -m 0755 hiredis/libhiredis.so debian/usr/lib/libhiredis.so
	sed "s/{ARCH}/`dpkg --print-architecture`/g" debian_control.in > debian/DEBIAN/control
	dpkg-deb --build debian
	mv debian.deb $(PACKAGE)_$(VERSION)_`dpkg --print-architecture`.deb

