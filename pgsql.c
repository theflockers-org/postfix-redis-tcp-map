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

/* postgresql */
#include <libpq-fe.h>

/* locais */
#include "config.h"
#include "tcp_mapper.h"

/* config */
extern config cfg;

/**
 * @name init_pgsql
 * @description initiates a pgsql connection
 * @return PGconn *pgsql
 */
PGconn * init_pgsql(void) {

    PGconn *pgsql;
    char connInfo[255];

    sprintf(connInfo, "host=%s port=%i dbname=%s user=%s password=%s", 
            cfg.pgsql_address, cfg.pgsql_port, cfg.pgsql_dbname, 
            cfg.pgsql_username, cfg.pgsql_password);

    pgsql = PQconnectdb(connInfo);
    
    if( (PQstatus(pgsql)) != CONNECTION_OK) {
        fprintf(stderr, "Connection error: %s", 
                PQerrorMessage(pgsql));

        PQfinish(pgsql);
        exit(-1);
    }
    return pgsql;
}

/**
 * @name tcp_mapper_pgsql_query
 * @description Issue a query to PostgreSQL server
 * @param PGconn *pgsql
 * @param char *query
 * @return int numrows
 */
int tcp_mapper_pgsql_query(PGconn *pgsql, char *query, char *result) {

    PGresult *res;
    int      numrows;

    res = PQexec(pgsql, query);
    numrows = PQntuples(res);

    /* get only the first row.. do not need more.. */
    if(numrows > 0) {
        sprintf(result, "%s", PQgetvalue(res, 0, 0));
    }

    PQclear(res);

    return numrows;
}
