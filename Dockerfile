FROM centos:7

RUN yum update -y && yum install -y \
    epel-release deltarpm
    
RUN yum update -y && yum clean all && yum groupinstall -y "Development tools"

RUN yum update -y && yum install -y \
    cairo-devel \
    curl-devel \
    gd-devel \
    gdal-devel \
    hdf5-devel \
    libxml2-devel \
    logrotate \
    make \
    netcdf \
    netcdf-devel \
    postgresql-devel \
    postgresql-server \
    proj \
    proj-devel \
    sqlite \
    sqlite-devel \
    tomcat \
    udunits2 \
    udunits2-devel 
    
RUN mkdir /adaguc

# Install adaguc-services (spring boot application for running adaguc-server)
RUN curl -L https://jitpack.io/com/github/KNMI/adaguc-services/1.0.2/adaguc-services-1.0.2.war > /usr/share/tomcat/webapps/adaguc-services.war

# Install adaguc-server from context
COPY . /adaguc/adaguc-server-master
WORKDIR /adaguc/adaguc-server-master
RUN bash compile.sh

# Run adaguc-server functional tests
RUN bash runtests.sh

# Setup directories
RUN mkdir -p /data/adaguc-autowms && \
    mkdir -p /data/adaguc-datasets && \
    mkdir -p /data/adaguc-data && \
    mkdir -p /adaguc/userworkspace && \
    mkdir -p /adaguc/basedir && \
    mkdir -p /var/log/adaguc && \
    mkdir -p /adaguc/adagucdb && \
    mkdir -p /data/adaguc-datasets-internal
   
# Configure
COPY ./Docker/adaguc-server-config.xml /adaguc/adaguc-server-config.xml
COPY ./Docker/adaguc-services-config.xml /adaguc/adaguc-services-config.xml
COPY ./Docker/start.sh /adaguc/
COPY ./Docker/adaguc-server-logrotate /etc/logrotate.d/adaguc
COPY ./Docker/adaguc-server-*.sh /adaguc/
COPY ./Docker/baselayers.xml /data/adaguc-datasets-internal/baselayers.xml
RUN  chmod +x /adaguc/adaguc-server-*.sh && chmod +x /adaguc/start.sh

# Set adaguc-services configuration file
ENV ADAGUC_SERVICES_CONFIG=/adaguc/adaguc-services-config.xml 
ENV ADAGUCDB=/adaguc/adagucdb

# These volumes are configured in /adaguc/adaguc-server-config.xml
# Place your netcdfs, HDF5 and GeoJSONS here, they will be visualized with the source=<file> KVP via the URI
VOLUME /data/adaguc-autowms   
# Place your dataset XML configuration here, they will be accessible with the dataset=<dataset basename> KVP via the URI
VOLUME /data/adaguc-datasets  
# Place your netcdfs, HDF5 and GeoJSONS here you don't want to have accessible via dataset configurations.
VOLUME /data/adaguc-data      
# Loggings are save here, including logrotate
VOLUME /var/log/adaguc/       
# You can make the postgresql database persistent by externally mounting it
VOLUME /adaguc/adagucdb       

EXPOSE 8080

ENTRYPOINT /adaguc/start.sh
