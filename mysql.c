/**
 *  Postfix Redis tcp map is a daemon to implement Postfixs tcp lookup table.
 *  Copyright (C) 2011  Leandro Mendes <leandro at gmail dot com>
 *  ---------------------------------------------------------------------------
 *  This file is part of postfix-redis-tcp-map.
 *
 *  postfix-redis-tcp-map is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  postfix-redis-tcp-map is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with postfix-redis-tcp-map.  If not, see <https://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* MySQL */
#include <mysql.h>

/* me */
#include "tcp_mapper.h"

/* config*/
#include "config.h"

extern config cfg;

/**
 * @name init_mysql
 * @description Starts and returns a connected MySQL instance
 * return MYSQL *mysql
 */
MYSQL * init_mysql(void) {

    MYSQL     *mysql;
    bool   reconnect = 1;

    /* initialize */
    if((mysql = mysql_init(NULL) ) == NULL) {
        perror("mysql_init error");
        /* freed memory */
        free(mysql);
        exit(-1);
    }

    /* reconnect if lost connection */
    mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);

    /* connect */
    if(!mysql_real_connect(mysql, cfg.mysql_address, cfg.mysql_username, 
                           cfg.mysql_password, cfg.mysql_dbname, 0, NULL, 0)){
        printf("Error connecting: %s\n", mysql_error(mysql));

        /* freed memory */
        free(mysql);
        exit(-1);
    }
    return mysql;
}

/**
 * @name tcp_mapper_mysql_query
 * @description Issue a query to a MySQL instance
 * @param MYSQL *mysql
 * @param char *query
 * @return int numrows
 */
int tcp_mapper_mysql_query(MYSQL *mysql, char *query, char *result){

    MYSQL_RES *res;
    MYSQL_ROW row;
    int       numrows;

    // printf("Query: %s\n", query);
    if(mysql_query(mysql, query) != 0) {
        printf("%s\n", mysql_error(mysql));
        return -1;
    }
    res = mysql_store_result(mysql);

    /* return the number of rows */
    numrows = mysql_num_rows(res);
    if(numrows > 0) {
        row = mysql_fetch_row(res);
        sprintf(result, "%s", (char *) row[0]);
    }
    mysql_free_result(res);
    // printf("Result: %s\n", result);

    return numrows;
}
