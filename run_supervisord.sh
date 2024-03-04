#!/bin/bash

# Look up db host/port/user/password/name from PGBOUNCE_DB string
# $PGBOUNCE_DB has the same format as $ADAGUC_DB
# Use these values to setup pgbouncer when running in AWS
read -r db_host db_port db_user db_password db_name <<< $(echo $PGBOUNCE_DB | awk -F'[= ]' '{ print $2" "$4" "$6" "$8" "$10 }')

# set user password
echo '"'$db_user'"' '"'$db_password'"' > /adaguc/adaguc-server-master/userlist.txt

# rewrite pgbouncer config
pgbouncer_cfg="adaguc = host=$db_host port=$db_port dbname=$db_name"
sed -i "s/^adaguc = .*/${pgbouncer_cfg}/g" /adaguc/adaguc-server-master/pgbouncer.ini

# supervisord starts both pgbouncer and adaguc
/usr/bin/supervisord -c /etc/supervisor/conf.d/supervisord.conf
