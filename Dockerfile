FROM centos/devtoolset-7-toolchain-centos7:7
USER root

LABEL maintainer="adaguc@knmi.nl"

# Version should be same as in Definitions.h
LABEL version="2.4.2" 

######### First stage (build) ############

# production packages, same as stage two
RUN yum update -y && \
    yum install -y epel-release deltarpm && \
    yum install -y cairo \
    curl \
    python3 \
    gd \
    gdal \
    hdf5 \
    libxml2 \
    proj \
    udunits2 \
    openssl \
    netcdf \
    libwebp-devel \
    java-11-openjdk && \
    # building / development packages
    yum install -y centos-release-scl && \
    yum install -y devtoolset-7-gcc-c++ && \
    source /opt/rh/devtoolset-7/enable && \
    yum install -y cairo-devel \
    gd-devel \
    gdal-devel \
    hdf5-devel \
    libxml2-devel \
    make \
    netcdf-devel \
    openssl \
    postgresql-devel \
    proj-devel \
    sqlite-devel \
    udunits2-devel && \
    yum clean all && \
    rm -rf /var/cache/yum

# Install adaguc-server from context
COPY . /adaguc/adaguc-server-master

# Alternatively install adaguc from github
# WORKDIR /adaguc
# ADD https://github.com/KNMI/adaguc-server/archive/master.tar.gz /adaguc/adaguc-server-master.tar.gz
# RUN tar -xzvf adaguc-server-master.tar.gz

WORKDIR /adaguc/adaguc-server-master


# Force to use Python 3
RUN ln -sf /usr/bin/python3 /usr/bin/python

RUN bash compile.sh

######### Second stage (production) ############
FROM centos:7
USER root

# production packages, same as stage one
RUN yum update -y && \
    yum install -y epel-release deltarpm && \
    yum install -y cairo \
    curl \
    gd \
    gdal \
    hdf5 \
    libxml2 \
    proj \
    python3 \
    python3-lxml \
    postgresql \
    udunits2 \
    openssl \
    netcdf \
    libwebp \
    java-11-openjdk-headless \
    python36-numpy \
    python36-netcdf4 \
    python36-six \
    python36-requests \
    python36-pillow \
    python36-lxml && \
    yum clean all && \
    rm -rf /var/cache/yum

WORKDIR /adaguc/adaguc-server-master

# Install compiled adaguc binaries from stage one    
COPY --from=0 /adaguc/adaguc-server-master/bin /adaguc/adaguc-server-master/bin
COPY --from=0 /adaguc/adaguc-server-master/data /adaguc/adaguc-server-master/data
COPY --from=0 /adaguc/adaguc-server-master/tests /adaguc/adaguc-server-master/tests
COPY --from=0 /adaguc/adaguc-server-master/runtests.sh /adaguc/adaguc-server-master/runtests.sh

# Install adaguc-services (spring boot application for running adaguc-server)
RUN curl -L https://jitpack.io/com/github/KNMI/adaguc-services/1.2.11/adaguc-services-1.2.11.jar -o /adaguc/adaguc-services.jar

# Run adaguc-server functional and regression tests
RUN  bash runtests.sh 

    # Set same uid as vivid
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
    mkdir -p /servicehealth

# Configure
COPY ./Docker/adaguc-server-config.xml /adaguc/adaguc-server-config.xml
COPY ./Docker/adaguc-services-config.xml /adaguc/adaguc-services-config.xml
COPY ./Docker/start.sh /adaguc/
COPY ./Docker/adaguc-server-*.sh /adaguc/
COPY ./Docker/baselayers.xml /data/adaguc-datasets-internal/baselayers.xml
RUN  chmod +x /adaguc/adaguc-server-*.sh && \
    chmod +x /adaguc/start.sh && \
    chown -R adaguc:adaguc /data/adaguc* /adaguc /servicehealth

# Put in default java truststore
RUN cp /etc/pki/java/cacerts /adaguc/security/truststore.ts

ENV ADAGUC_SERVICES_CONFIG=/adaguc/adaguc-services-config.xml

USER adaguc

# For HTTP
EXPOSE 8080

ENTRYPOINT ["sh", "/adaguc/start.sh"]
