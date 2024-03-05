#!/bin/bash

if [[ "$PGBOUNCER_ENABLE" == "true" || "$PGBOUNCER_ENABLE" == "TRUE" ]]; then
    echo "PGBOUNCER_ENABLE is true, using /etc/supervisor/conf.d/adaguc-pgbouncer.conf"

    # Look up db host/port/user/password/name from ADAGUC_DB string
    read -r db_host db_port db_user db_password db_name <<< $(echo $ADAGUC_DB | awk -F'[= ]' '{ print $2" "$4" "$6" "$8" "$10 }')

    # Disable ssl for pgbouncer when running through docker-compose
    # if host=adaguc-db, then we're using docker-compose.
    if [[ "$db_host" == "adaguc-db" ]]; then
        sed -i "s/^server_tls_sslmode = .*/server_tls_sslmode = disable/g" /adaguc/pgbouncer/pgbouncer.ini
    fi

    # set user password
    echo '"'$db_user'"' '"'$db_password'"' > /adaguc/pgbouncer/userlist.txt

    # rewrite pgbouncer config
    pgbouncer_cfg="adaguc = host=$db_host port=$db_port dbname=$db_name"
    sed -i "s/^adaguc = .*/${pgbouncer_cfg}/g" /adaguc/pgbouncer/pgbouncer.ini
    export ADAGUC_DB="host=localhost port=${db_port} user=${db_user} password=${db_password} dbname=${db_name}"

    # supervisord starts both pgbouncer and adaguc
    /usr/bin/supervisord -c /etc/supervisor/conf.d/adaguc-pgbouncer.conf
else
    echo "No PGBOUNCER_ENABLE set to true, using /etc/supervisor/conf.d/adaguc.conf"
    /usr/bin/supervisord -c /etc/supervisor/conf.d/adaguc.conf
fi

# if [[ "$PGBOUNCE_DB" = "" ]]; then
#     echo "No PGBOUNCE_DB set, running adaguc only"
#     /usr/bin/supervisord -c /etc/supervisor/conf.d/adaguc.conf
# else
#     echo "PGBOUNCE_DB is set, running adaguc with pgbouncer"
#     # Look up db host/port/user/password/name from PGBOUNCE_DB string
#     # $PGBOUNCE_DB has the same format as $ADAGUC_DB
#     # Use these values to setup pgbouncer when running in AWS
#     read -r db_host db_port db_user db_password db_name <<< $(echo $PGBOUNCE_DB | awk -F'[= ]' '{ print $2" "$4" "$6" "$8" "$10 }')

#     # Disable ssl for pgbouncer when running through docker-compose
#     # if host=adaguc-db, then we're using docker-compose.
#     if [[ "$db_host" == "adaguc-db" ]]; then
#         sed -i "s/^server_tls_sslmode = .*/server_tls_sslmode = disable/g" /adaguc/pgbouncer/pgbouncer.ini
#     fi

#     # set user password
#     echo '"'$db_user'"' '"'$db_password'"' > /adaguc/pgbouncer/userlist.txt

#     # rewrite pgbouncer config
#     pgbouncer_cfg="adaguc = host=$db_host port=$db_port dbname=$db_name"
#     sed -i "s/^adaguc = .*/${pgbouncer_cfg}/g" /adaguc/pgbouncer/pgbouncer.ini

#     # supervisord starts both pgbouncer and adaguc
#     /usr/bin/supervisord -c /etc/supervisor/conf.d/adaguc-pgbouncer.conf
# fi
