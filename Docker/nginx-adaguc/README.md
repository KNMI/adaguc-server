# docker-term-letsencrypt
docker / nginx / Let's encrypt SSL termination/proxy.

[![Docker Build](https://img.shields.io/docker/build/rsdnlesc/docker-term-letsencrypt.svg)](https://hub.docker.com/r/rsdnlesc/docker-term-letsencrypt/)

This Docker container can be used as a reverse proxy between the internet and the rest of your application.
It takes care of encryption and acquisition & automatic renewal of HTTPS certificates using Let's encrypt / Certbot. It should give you an `A` score at [SSL Labs](https://www.ssllabs.com/).

## How to use
Configuration is done through environment variables, (see the `.env` file):
```
# The domains to request the certificate for, seperated by a comma
SSL_DOMAINS=mydomain.org,www.mydomain.org
# The proxy to forward requests to (probably another container in your Docker network)
SSL_FORWARD=http://localhost:80
# The admin email (this is required when requesting a certificate)
SSL_ADMIN_EMAIL=admin@mydomain.org
```
Let's encrypt has a [rate limit](https://letsencrypt.org/docs/rate-limits/) of a few generated certificates per domain per month, so you cannot just generate new certificates every time the container is restarted.
So you should keep the generated certificates and config by mounting persistent volumes at:
- `/etc/letsencrypt`
- `/cert`

Make sure to bind ports `80` and `443` of course.

E.g. to run standalone:
- `docker pull rsdnlesc/docker-term-letsencrypt`
- or `docker build . -t rsdnlesc/docker-term-letsencrypt`

```
docker run -it \
  --env-file .env \
  -p80:80 \
  -p443:443 \
  -v`pwd`/cert:/cert \
  -v`pwd`/letsencrypt:/etc/letsencrypt \
  rsdnlesc/docker-term-letsencrypt
```

On first run a self-signed certificate is generated, which is overwritten when Certbot succeeds in getting a certificate from Let's Encrypt. This means that even if it fails everything will still function - but with warnings in the browser because of the self-signed certificate.

By default Certbot will try to renew your certificate every night at `3 am` (but this can be changed by editing the `crontab`).
