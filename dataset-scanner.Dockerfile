FROM centos/devtoolset-7-toolchain-centos7:7
USER root

MAINTAINER Adaguc Team at KNMI <adaguc@knmi.nl>

######### First stage (build) ############

# production packages, same as stage two
RUN yum update -y && \
  yum install -y epel-release deltarpm && \
  yum install -y cairo \
  curl \
  gd \
  gdal \
  hdf5 \
  libxml2 \
  libwebp-devel \
  postgresql-server \
  proj \
  udunits2 \
  openssl \
  netcdf && \
  # building / development packages
  yum install -y centos-release-scl && \
  yum install -y devtoolset-7-gcc-c++ && \
  source /opt/rh/devtoolset-7/enable && \
  yum install -y cairo-devel \
  curl-devel \
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
RUN bash compile.sh

######### Second stage (production) ############
FROM centos/devtoolset-7-toolchain-centos7:7
USER root
# production packages, same as stage one
RUN yum update -y && \
  yum install -y epel-release deltarpm && \
  # building / development packages
  yum install -y centos-release-scl && \
  yum install -y devtoolset-7-gcc-c++ && \
  source /opt/rh/devtoolset-7/enable && \
  yum install -y cairo \
  curl \
  gd \
  gdal \
  hdf5 \
  libxml2 \
  libwebp-devel \
  postgresql \
  proj \
  udunits2 \
  openssl \
  netcdf \
  python-devel && \
  yum clean all && \
  rm -rf /var/cache/yum

WORKDIR /adaguc/adaguc-server-master

# Install compiled adaguc binaries from stage one    
COPY --from=0 /adaguc/adaguc-server-master/bin /adaguc/adaguc-server-master/bin
COPY --from=0 /adaguc/adaguc-server-master/data /adaguc/adaguc-server-master/data
COPY --from=0 /adaguc/adaguc-server-master/tests /adaguc/adaguc-server-master/tests
COPY --from=0 /adaguc/adaguc-server-master/runtests.sh /adaguc/adaguc-server-master/runtests.sh

# Install adaguc-services (spring boot application for running adaguc-server)
RUN curl -L https://jitpack.io/com/github/KNMI/adaguc-services/1.2.0/adaguc-services-1.2.0.jar > /adaguc/adaguc-services.jar && \
  # Install newer numpy
  curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py && \
  python get-pip.py && \
  pip install numpy netcdf4 six python3-lxml requests && \
  # Run adaguc-server functional and regression tests
  bash runtests.sh && \
  # Set same uid as vivid
  useradd -m adaguc -u 1000 && \
  # Setup directories
  mkdir -p /data/adaguc-autowms && \
  mkdir -p /data/adaguc-datasets && \
  mkdir -p /data/adaguc-data && \
  mkdir -p /adaguc/userworkspace && \
  mkdir -p /data/adaguc-services-home && \
  mkdir -p /adaguc/basedir && \
  mkdir -p /var/log/adaguc && \
  mkdir -p /adaguc/adagucdb && \
  mkdir -p /adaguc/security && \
  mkdir -p /data/adaguc-datasets-internal && \
  mkdir -p /servicehealth
    
# Configure
COPY ./Docker/adaguc-server-config.xml /adaguc/adaguc-server-config.xml
COPY ./Docker/adaguc-services-config.xml /adaguc/adaguc-services-config.xml
COPY ./Docker/start.sh /adaguc/
COPY ./Docker/adaguc-server-*.sh /adaguc/
COPY ./Docker/baselayers.xml /data/adaguc-datasets-internal/baselayers.xml
RUN  chmod +x /adaguc/adaguc-server-*.sh && chmod +x /adaguc/start.sh \
  && chown -R adaguc:adaguc /data/adaguc* /adaguc /var/log/adaguc /servicehealth

USER adaguc

# Set adaguc-services configuration file
ENV ADAGUC_SERVICES_CONFIG=/adaguc/adaguc-services-config.xml
# Location where postgresql writes its files:
ENV ADAGUCDB=/adaguc/adagucdb
# Configuration settings for postgresql database connection
ENV ADAGUC_DB="host=localhost port=5432 user=adaguc password=adaguc dbname=adaguc"

ENTRYPOINT ["sh", "/adaguc/adaguc-server-updatedatasets.sh"]
# Override this at runtime with the target dataset
CMD [""]
