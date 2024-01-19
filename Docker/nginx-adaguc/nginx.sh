#!/bin/sh
source /etc/envvars
HOSTNAMEWITHSLASH=${EXTERNAL_HOSTNAME#https://};
HOSTNAME=${HOSTNAMEWITHSLASH%/}
envsubst '\$SSL_DOMAINS \$SSL_FORWARD \$HOSTNAME' < /nginx.conf > /etc/nginx/nginx.conf
exec /usr/sbin/nginx -g "daemon off;"
