FROM centos:7

RUN yum update -y && yum install -y \
    epel-release

RUN yum clean all && yum groupinstall -y "Development tools"

RUN yum update -y && yum install -y \
    hdf5-devel \
    netcdf \
    netcdf-devel \
    proj \
    proj-devel \
    sqlite \
    sqlite-devel \
    udunits2 \
    udunits2-devel \
    make \
    libxml2-devel \
    cairo-devel \
    gd-devel \
    postgresql-devel \
    postgresql-server \
    gdal-devel \
    tomcat \ 
    maven
         
RUN mkdir /src

WORKDIR /src

# Configure postgres
RUN mkdir /postgresql
RUN touch /var/log/postgresql.log
RUN chown postgres: /postgresql/
RUN chown postgres: /var/log/postgresql.log
RUN runuser -l postgres -c "initdb -D /postgresql"

# Install adaguc-server
WORKDIR /src
RUN curl -L  https://github.com/KNMI/adaguc-server/archive/master.tar.gz > adaguc-server.tar.gz
RUN tar xvf adaguc-server.tar.gz
WORKDIR /src/adaguc-server-master
RUN bash compile.sh

# Install adaguc-viewer
WORKDIR /usr/share/tomcat/webapps
RUN curl -L  https://github.com/KNMI/adaguc-viewer/archive/master.tar.gz > adaguc-viewer.tar.gz
RUN tar xvf adaguc-viewer.tar.gz
RUN mv adaguc-viewer-master adaguc-viewer

# Configure adaguc-viewer
RUN printf "\n\
xml2jsonrequestURL = '/adaguc-services/xml2json?'\n\
baseLayerConfiguration = [\n\
  {service:'http://172.18.0.2:8080/adaguc-services/adagucserver?',name:'baselayer',title:'Baselayer',enabled: true,format:'image/png'},\n\
  {service:'http://172.18.0.2:8080/adaguc-services/adagucserver?',name:'overlay',title:'Overlay',enabled: true,keepontop:true,format:'image/png'}\n\
];\n\
" >> adaguc-viewer/config.js

# Install adaguc-services
WORKDIR /src
RUN curl -L https://github.com/KNMI/adaguc-services/archive/master.tar.gz > adaguc-services.tar.gz
RUN tar xvf adaguc-services.tar.gz
WORKDIR /src/adaguc-services-master
RUN mvn package
RUN cp ./target/adaguc-services-1.0.0-SNAPSHOT.war /usr/share/tomcat/webapps/adaguc-services.war

RUN keytool -genkey -noprompt -keypass password -alias tomcat -keyalg RSA -storepass password -keystore /tmp/c4i_keystore.jks  -dname CN=compute-test.c3s-magic.eu/C=NL/O=C3SMAGIC/OU=KNMI 

# Configure adaguc-services
RUN printf "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<adaguc-services>\n\
  <external-home-url>http://172.18.0.2:8080</external-home-url>\n\
  <basedir>/root/adaguc-services-base</basedir>\n\
  <userworkspace>/root/adaguc-services-space</userworkspace>\n\
  <server>\n\
    <port>8080</port>\n\
  </server>\n\
  <security>\n\
    <keystore>/root/keystore.jks</keystore>\n\
    <keystorepassword>password</keystorepassword>\n\
    <keystoretype>JKS</keystoretype>\n\
    <keyalias>tomcat</keyalias>\n\
  </security>\n\
  <adaguc-server>\n\
    <adagucexecutable>/src/adaguc-server-master/bin/adagucserver</adagucexecutable>\n\
    <export>ADAGUC_PATH=/src/adaguc-server-master/</export>\n\
    <export>ADAGUC_CONFIG=/src/adaguc-server-master/data/config/adaguc.docker.xml</export>\n\
    <export>ADAGUC_DATARESTRICTION=FALSE</export>\n\
    <export>ADAGUC_LOGFILE=/tmp/adaguc.docker.log</export>\n\
    <export>ADAGUC_FONT=/src/adaguc-server-master/data/fonts/FreeSans.ttf</export>\n\
  </adaguc-server>\n\
</adaguc-services>" > /root/adaguc-services-config.xml

ENV ADAGUC_SERVICES_CONFIG=/root/adaguc-services-config.xml 

# Set up data dir, this is also configured in adaguc.docker.xml
RUN mkdir /data/

#Setup directory for automatic visualization of NetCDF's
RUN mkdir /data/adaguc-autowms

#Setup directory for visualization of ADAGUC datasets
RUN mkdir /data/adaguc-datasets

EXPOSE 8080

CMD echo "Starting POSTGRESQL DB" && \
    runuser -l postgres -c "pg_ctl -D /postgresql -l /var/log/postgresql.log start" && \
    sleep 1 && \ 
    mkdir -p /data/adaguc-autowms/ && \ 
    mkdir -p /data/adaguc-datasets/ && \ 
    cp /src/adaguc-server-master/data/datasets/testdata.nc /data/adaguc-autowms/ && \
    cp /src/adaguc-server-master/data/config/datasets/dataset_a.xml /data/adaguc-datasets/ && \
    echo "Configuring POSTGRESQL DB" && \
    runuser -l postgres -c "createuser --superuser adaguc" && \
    runuser -l postgres -c "psql postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\"" && \
    runuser -l postgres -c "psql postgres -c \"CREATE DATABASE adaguc;\"" && \
    echo "Starting TOMCAT Server" && \
    `nohup /usr/libexec/tomcat/server start &> /var/log/tomcat.log &` && \
    /bin/bash
 
# Build with docker build  -t adaguc-server ./data/docker/
# This docker container needs to be runned with custom configuration settings:  
# docker network create --subnet=172.18.0.0/16 adagucnet
# docker run -i -t --net adagucnet --ip 172.18.0.2 -v $HOME/data:/data adaguc-server
# Then visit http://172.18.0.2:8080/adaguc-viewer/?service=http%3A%2F%2F172.18.0.2%3A8080%2Fadaguc-services%2Fadagucserver%3Fservice%3Dwms%26request%3Dgetcapabilities%26source%3Dtestdata.nc
# You can copy NetCDF's / GeoJSONS to your hosts ~/data directory. This will be served through adaguc-server, via the source=<filename> key value pair. testdata.nc is copied there by default. See example URL above.