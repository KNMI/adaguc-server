#!/bin/sh
export > /etc/envvars
trap "sv stop crond; sv stop nginx; exit" SIGTERM

if [ ! -f /cert/dhparam.pem ]; then
    echo "No dhparam file found."
    openssl dhparam -out /cert/dhparam.pem 2048
fi

if [ ! -f /cert/privkey.pem ]; then
    echo "No SSL certificate found."
    echo "Generating self-signed certificate"
    ( set -x; openssl req -x509 -nodes -days 365 -newkey rsa:2048 -subj '/CN=*/O=C3S-MAGIC/C=EU' -keyout /cert/privkey.pem -out /cert/fullchain.pem )
fi

/sbin/runsvdir /etc/service &
sh ./certbot.sh
wait
