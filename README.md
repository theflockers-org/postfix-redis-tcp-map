# Postfix Redis-TCP Map

Fetches entries from your MySQL/PGSQL or LDAP and saves on Redis for caching
purposes

## Disclaimer
This software is distributed as is without any warranty

## Known bugs

PGSQL and LDAP are not functional at the moment.

## Requirements

### Fedora/CentOS/Rocky
- glib2-devel
- libevent-devel
- hiredis-devel
- mysql-devel
- libpq-devel
- openldap-devel

## Building
```
$ make
$ sudo ./postfix-redis -c /etc/postfix-redis/postfix-redis.cfg
```

## Database

On MySQL/PGSQL
You need a sample database like this (at least) but you can make the query on 
`postfix-redis.cfg` match what you need.

``` 
------------------
      users
------------------
user | domain
------------------
john | xpto.com
foo  | bar.co.uk
lala | popo.com.br
```
Sample `missing_registry_query`

```
SELECT 'OK' from domain where user = '%u' and domain = '%d'
```
The `OK` part is mandatory.

If you user table uses the full e-mail address, you can user `%s` instead of `%u`
following Postfix lookup tables documentation. 

- https://www.postfix.org/mysql_table.5.html
- https://www.postfix.org/pgsql_table.5.html
- https://www.postfix.org/ldap_table.5.html
