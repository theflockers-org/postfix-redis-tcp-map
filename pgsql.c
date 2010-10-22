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
