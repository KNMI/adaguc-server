#!/bin/bash

if [[ "${PGBOUNCER_ENABLE,,}" == "true" ]]; then
    echo "PGBOUNCER_ENABLE env is true, using /etc/supervisor/conf.d/adaguc-pgbouncer.conf"

    # Look up host/port/user/password/name fields from ADAGUC_DB string
    # Order does not matter. Keys get prefixed with "db_" in this script.
    set $ADAGUC_DB
    for word in "$@"; do
        IFS='=' read -r key val <<< "$word"
        test -n "$val" && printf -v "adaguc_db_${key}" "${val}"
    done

    # The postgres instance in docker-compose does not have SSL support enabled.
    # This is configurable through the PGBOUNCER_DISABLE_SSL environment variable.
    # By default, SSL is used
    if [[ "${PGBOUNCER_DISABLE_SSL,,}" == "true" ]]; then
        sed -i "s/^server_tls_sslmode = .*/server_tls_sslmode = disable/g" /adaguc/pgbouncer/pgbouncer.ini
    fi

    # Set username/password, pgbouncer uses this to authenticate to RDS
    echo '"'$adaguc_db_user'"' '"'$adaguc_db_password'"' > /adaguc/pgbouncer/userlist.txt

    # Rewrite pgbouncer config. $db_host could be an RDS hostname or adaguc-db when running through docker-compose.
    pgbouncer_cfg="adaguc = host=$adaguc_db_host port=$adaguc_db_port dbname=$adaguc_db_dbname"
    sed -i "s/^adaguc = .*/${pgbouncer_cfg}/g" /adaguc/pgbouncer/pgbouncer.ini

    # Rewrite $ADAGUC_DB
    export ADAGUC_DB="host=localhost port=${adaguc_db_port} user=${adaguc_db_user} password=${adaguc_db_password} dbname=${adaguc_db_dbname}"

    # supervisord starts both pgbouncer and adaguc
    /usr/bin/supervisord -c /etc/supervisor/conf.d/adaguc-pgbouncer.conf
else
    echo "No PGBOUNCER_ENABLE env found, using /etc/supervisor/conf.d/adaguc.conf"

    # supervisord starts adaguc only
    /usr/bin/supervisord -c /etc/supervisor/conf.d/adaguc.conf
fi
