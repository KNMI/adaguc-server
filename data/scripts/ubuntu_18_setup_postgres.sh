#/bin/bash

### Setup postgres database for user adaguc and password adaguc, databasename adaguc ###
sudo -u postgres createuser --superuser adaguc
sudo -u postgres psql postgres -c "ALTER USER adaguc PASSWORD 'adaguc';"
sudo -u postgres psql postgres -c "CREATE DATABASE adaguc;"
echo "\q" | psql "dbname=adaguc user=adaguc password=adaguc host=localhost"
if [ ${?} -eq 0 ];then
    echo "Postgres database setup correctly"
fi
