#!/bin/sh
source /etc/envvars

while ! nc -z -w1 localhost 80 ; do
    sleep 1
done

DIR_NAME=`ls /etc/letsencrypt/live | head -n 1`

install_cert()
{
	DIR_NAME=`ls /etc/letsencrypt/live | head -n 1`
	cp -L /etc/letsencrypt/live/${DIR_NAME}/*.pem /cert
	sv hup nginx
}

if [ -z "$DIR_NAME" ]; then
	# There is no existing certbot configuration
	DOMAIN_PARAMS="-d $(echo $EXTERNAL_HOSTNAME | sed 's/,/ -d /g')"
	( set -x; certbot certonly --webroot -w /acme -n --agree-tos --email ${SSL_ADMIN_EMAIL} ${DOMAIN_PARAMS} --expand )
	if [ $? -ne 0 ]; then
		echo "WARNING: Certbot failed to create certificate"
	else
		install_cert
	fi
else
	( set -x; certbot renew )
	if [ $? -ne 0 ]; then
        	echo "WARNING: Certbot failed to renew certificate"
	else
		install_cert
	fi
	
fi

