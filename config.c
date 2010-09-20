#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "config.h"

config parseConfig(char *filename) {

    FILE *fp;
    char buf[4096];
    char opt[255], val[255];
    config cfg;

    if( (fp = fopen(filename, "r") )== NULL) {
        perror("Erro opening de config");
    }
    memset(&opt, 0, sizeof(opt));
    memset(&val, 0, sizeof(val));

    while(fgets(buf, sizeof(buf), fp) != 0) {
        sscanf(buf, "%s = %s", opt, val);

        if(strncmp("listen_address", opt, 14) == 0)
            snprintf(cfg.listen_address, (size_t) strlen(val)+1, "%s", val);

        if(strncmp("listen_port", opt, 12) == 0)
            cfg.listen_port = atoi(val);

        if(strncmp("timeout", opt, 7) == 0)
            cfg.timeout = atoi(val);

        if(strncmp("redis_address", opt, 13) == 0)
            snprintf(cfg.redis_address, (size_t) strlen(val)+1, "%s", val);

        if(strncmp("redis_port", opt, 10) == 0)
            cfg.redis_port = atoi(val);

        if(strncmp("redis_timeout", opt, 13) == 0)
            cfg.redis_timeout = atoi(val);
    }
    fclose(fp);
    return cfg;
}
