#!/bin/sh
source /etc/envvars
envsubst '\$SSL_DOMAINS \$SSL_FORWARD' < /nginx.conf > /etc/nginx/nginx.conf
exec /usr/sbin/nginx -g "daemon off;"
