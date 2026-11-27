FROM debian:trixie-slim

RUN apt-get -q -y update \
    && DEBIAN_FRONTEND=noninteractive apt-get -q -y upgrade \
    && DEBIAN_FRONTEND=noninteractive apt-get -q -y install --no-install-recommends \
    nginx \
    certbot \
    runit \
    gettext-base \
    openssl \
    cron \
    netcat-openbsd \
    ca-certificates \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /etc/service/nginx /etc/service/crond /acme

COPY nginx.conf /nginx.conf
RUN ln -sf /dev/stdout /var/log/nginx/access.log
RUN ln -sf /dev/stderr /var/log/nginx/error.log

COPY certbot.sh /certbot.sh
RUN chmod a+x /certbot.sh

COPY ./crontab /tmp/nginx-crontab
# Keep our renewal job, which also copies certificates and reloads nginx.
RUN crontab /tmp/nginx-crontab \
    && rm /tmp/nginx-crontab /etc/cron.d/certbot

COPY nginx.sh /etc/service/nginx/run
RUN chmod a+x /etc/service/nginx/run
COPY crond.sh /etc/service/crond/run
RUN chmod a+x /etc/service/crond/run

COPY start.sh /start.sh
RUN chmod a+x /start.sh

ENTRYPOINT ["/start.sh"]
