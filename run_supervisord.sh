#!/bin/bash

# Look up db user/pass/hostname/dbname from ADAGUC_DB string
# Use these values to setup pgbouncer when running in AWS
db_host=$(echo $ADAGUC_DB | cut -d' ' -f1 | cut -d'=' -f2)
db_port=$(echo $ADAGUC_DB | cut -d' ' -f2 | cut -d'=' -f2)
db_user=$(echo $ADAGUC_DB | cut -d' ' -f3 | cut -d'=' -f2)
db_password=$(echo $ADAGUC_DB | cut -d' ' -f4 | cut -d'=' -f2)
db_name=$(echo $ADAGUC_DB | cut -d' ' -f5 | cut -d'=' -f2)

echo '"'$db_user'"' '"'$db_password'"' > /adaguc/adaguc-server-master/userlist.txt

pgbouncer_cfg="adaguc = host=$db_host port=$db_port dbname=$db_name"
sed -i "s/^adaguc = .*/${pgbouncer_cfg}/g" /adaguc/adaguc-server-master/pgbouncer.ini

/usr/bin/supervisord -c /etc/supervisor/conf.d/supervisord.conf
