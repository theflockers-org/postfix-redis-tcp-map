#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* LDAP */
#include <ldap.h>

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
LDAP * init_ldap(void) {

    LDAP    *ldap;
    int     err;
    int     v3 = LDAP_VERSION3;

    struct berval cred;
    struct berval *servcred;

    /* initialize */
    if((ldap_initialize(&ldap, cfg.ldap_uri) != LDAP_SUCCESS)) {
        perror("ldap_init error");
        free(ldap);
        exit(-1);
    }

    ldap_set_option(ldap, LDAP_OPT_PROTOCOL_VERSION, &v3);

    /* connect */
    if((err = ldap_sasl_bind_s(ldap, cfg.ldap_bind_dn, cfg.ldap_bind_pw, &cred, NULL, NULL, &servcred) != LDAP_SUCCESS)){
        printf("%s\n", ldap_err2string(err));

        /* freed memory */
        free(ldap);
        exit(-1);
    }
    return ldap;
}

/**
 * @name tcp_mapper_ldap_query
 * @description Issue a query to a LDAP instance
 * @param LDAP *ldap
 * @return int numrows
 */
int tcp_mapper_ldap_search(LDAP *ldap, char *search, char *result){

    LDAPMessage *ldap_result, *entry;
    int     numentries = 0;
    int     err;
    //char    **val;
    struct berval **vals;

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;


    if((err = ldap_search_ext_s(ldap, cfg.ldap_base, LDAP_SCOPE_SUBTREE, search,
                NULL, 0, NULL, NULL, &timeout, 10, &ldap_result) != LDAP_SUCCESS )) {

        printf("%s\n", ldap_err2string(err));
        return -1;
    }
    numentries = ldap_count_entries(ldap, ldap_result);

    if(numentries != 0) {
        /* just first entry. We don't need any other */
        entry = ldap_first_entry(ldap, ldap_result);
        vals   = ldap_get_values_len(ldap, entry, cfg.ldap_result_attr);

        if(vals == NULL) {
            return 0; 
        }
        snprintf(result, strlen(vals[0]->bv_val)+1, "%s", 
                 (char *) vals[0]->bv_val);
        ldap_value_free_len(vals);
    }

    return numentries;
}
