VERSION = 0.1
PACKAGE = postfix-redis-tcp-map
LDFLAGS = -lhiredis -levent -lc -L/usr/lib64/mysql -lmysqlclient -lpq `pkg-config --libs glib-2.0` -lldap
CFLAGS = -Wall -I/usr/include/hiredis -g -DUSE_LIBEVENT -I/usr/include/mysql -I/usr/include/postgresql `pkg-config --cflags glib-2.0`
PREFIX = /usr
SBINDIR = $(PREFIX)/sbin
SYSCONFDIR = /etc/postfix-redis/

CC = gcc
LD = gcc

all: package postfix-redis

postfix-redis: main.o client.o config.o redis.o mysql.o pgsql.o ldap.o
	$(LD) -o postfix-redis config.o main.o client.o redis.o mysql.o pgsql.o ldap.o $(LDFLAGS) $(CFLAGS)

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

ldap.o: ldap.c
	$(CC) -c ldap.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f postfix-redis *.o

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

