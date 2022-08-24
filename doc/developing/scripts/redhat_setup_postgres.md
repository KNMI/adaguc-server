# Instructions on how to setup postgres server for adaguc on redhat systems

[Back to Developing](../../Developing.md)

```
#/bin/bash

### Setup postgres database for user adaguc and password adaguc, databasename adaguc ###
mkdir /postgresql
touch /var/log/postgresql.log

chown postgres: /postgresql/
chown postgres: /var/log/postgresql.log

runuser -l postgres -c "initdb -D /postgresql"
runuser -l postgres -c "pg_ctl -D /postgresql -l /var/log/postgresql.log start"
sleep 2
runuser -l postgres -c "createuser --superuser adaguc"
runuser -l postgres -c "psql postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\""
runuser -l postgres -c "psql postgres -c \"CREATE DATABASE adaguc;\""

echo "\q" | psql "dbname=adaguc user=adaguc password=adaguc host=localhost"
if [ ${?} -eq 0 ];then
    echo "Postgres database setup correctly"
    echo "Use the following setting for ADAGUC:"
    echo "export ADAGUC_DB=\"dbname=adaguc user=adaguc password=adaguc host=localhost\""
else
    echo "Postgres database not fully functional, please check"
fi
```