######### Second stage (production) ############
FROM centos:7
USER root

WORKDIR /adaguc/adaguc-server-master

RUN yum update -y --setopt=tsflags=nodocs && \
    yum install -y --setopt=tsflags=nodocs epel-release deltarpm && \
    yum install -y --setopt=tsflags=nodocs cairo \
    curl \
    gd \
    gdal \
    hdf5 \
    libxml2 \
    proj \
    postgresql \
    udunits2 \
    openssl \
    netcdf \
    libwebp \
    httpd \
    yum clean all && \
    rm -rf /var/cache/yum

# Copy adaguc-server
COPY --from=openearth/adaguc-server /adaguc/adaguc-server-master/bin /adaguc/adaguc-server-master/bin
COPY --from=openearth/adaguc-server /adaguc/adaguc-server-master/data /adaguc/adaguc-server-master/data
COPY --from=openearth/adaguc-server /adaguc/adaguc-server-config.xml /adaguc/adaguc-server-config.xml
COPY --from=openearth/adaguc-server /adaguc/adaguc-server-*.sh /adaguc/
COPY --from=openearth/adaguc-server /data/adaguc-datasets-internal/baselayers.xml /data/adaguc-datasets-internal/baselayers.xml
COPY --from=openearth/adaguc-server /adaguc/adaguc-server-master/bin/adagucserver /var/www/cgi-bin/adaguc-server

# Copy httpd scripts
COPY ./start.sh /adaguc/
COPY ./httpd.conf /etc/httpd/conf/httpd.conf

#Setup user, directories and permissions
RUN useradd -m adaguc -u 1000 && \
    # Setup directories
    mkdir -p /data/adaguc-autowms && \
    mkdir -p /data/adaguc-datasets && \
    mkdir -p /data/adaguc-data && \
    mkdir -p /adaguc/userworkspace && \
    mkdir -p /data/adaguc-services-home && \
    mkdir -p /adaguc/basedir && \
    mkdir -p /adaguc/security && \
    mkdir -p /data/adaguc-datasets-internal && \
    mkdir -p /servicehealth && \
    chmod +x /adaguc/adaguc-server-*.sh && \
    chmod +x /adaguc/start.sh && \
    chown -R adaguc:adaguc /data/adaguc* /adaguc /servicehealth


#Setup permissions
RUN rm -rf /run/httpd/* /tmp/httpd* && \
    chown adaguc: /run/httpd* && \
    touch /var/log/adaguc-server.log && chown adaguc: /var/log/adaguc-server.log 

USER adaguc

# For HTTP
EXPOSE 8080

VOLUME /etc
ENTRYPOINT ["sh", "/adaguc/start.sh"]

