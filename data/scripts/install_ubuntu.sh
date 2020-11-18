#/bin/bash
apt-get install cmake postgresql libcurl4-openssl-dev libcairo2-dev libxml2-dev libgd2-xpm-dev postgresql-server-dev-all postgresql-client libudunits2-dev udunits-bin g++ m4 netcdf-bin libnetcdf-dev libhdf5-dev libproj-dev



# 
# ### Setup postgres for adaguc: ###
# -u postgres createuser --superuser adaguc
# -u postgres psql postgres 
# \password adaguc 
# \q
# # note: type adaguc as password, when finished press \q to exit.
# 
# createdb adaguc
