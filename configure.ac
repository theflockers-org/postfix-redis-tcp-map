AC_INIT([postfix-redis-tcp-map],[1.0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CHECK_HEADER([hiredis.h], [],[echo "error: hiredis.h not found"; exit -1])
PKG_CHECK_MODULES([glib], [glib-2.0 >= 2])
PKG_CHECK_MODULES([libevent], [libevent >= 2])
AC_SEARCH_LIBS(mysql_init, mysqlclient, [], [ echo "error: -lmysqlclient not found"; exit -1])
AC_SEARCH_LIBS(PQconnectdb, pq, [], [ echo "error: -lpq not found"; exit -1])
AC_SEARCH_LIBS(ldap_initialize, ldap, [], [ echo "error: -lldap not found"; exit -1])
AC_SEARCH_LIBS(redisConnect, hiredis, [], [echo "error: -lhiredis not found"; exit -1])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
