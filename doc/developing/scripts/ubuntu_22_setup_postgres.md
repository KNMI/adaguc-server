# Instructions on how to setup postgres server for adaguc on Ubuntu 22

[Back to Developing](../../Developing.md)

```
#/bin/bash

### Setup postgres database for user adaguc and password adaguc, databasename adaguc ###
sudo service postgresql start
sudo -u postgres createuser --superuser adaguc
sudo -u postgres psql postgres -c "ALTER USER adaguc PASSWORD 'adaguc';"
sudo -u postgres psql postgres -c "CREATE DATABASE adaguc;"
echo "\q" | psql "dbname=adaguc user=adaguc password=adaguc host=localhost"
if [ ${?} -eq 0 ];then
    echo "Postgres database setup correctly"
    echo "Use the following setting for ADAGUC:"
    echo "export ADAGUC_DB=\"dbname=adaguc user=adaguc password=adaguc host=localhost\""
else
    echo "Postgres database not fully functional, please check"
fi
```
