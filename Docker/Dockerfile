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
         
RUN mkdir /adaguc

WORKDIR /adaguc

# Configure postgres
RUN mkdir /postgresql
RUN touch /var/log/postgresql.log
RUN chown postgres: /postgresql/
RUN chown postgres: /var/log/postgresql.log
RUN runuser -l postgres -c "initdb -D /postgresql"

# Install adaguc-server
WORKDIR /adaguc
RUN curl -L  https://github.com/KNMI/adaguc-server/archive/master.tar.gz > adaguc-server.tar.gz
RUN tar xvf adaguc-server.tar.gz
WORKDIR /adaguc/adaguc-server-master
RUN bash compile.sh

# Install adaguc-services
WORKDIR /adaguc
RUN curl -L https://github.com/maartenplieger/adaguc-services/archive/master.tar.gz > adaguc-services.tar.gz
RUN tar xvf adaguc-services.tar.gz
WORKDIR /adaguc/adaguc-services-master
RUN mvn package
RUN cp ./target/adaguc-services-*.war /usr/share/tomcat/webapps/adaguc-services.war

COPY adaguc-server-config.xml /adaguc/adaguc-server-config.xml

COPY adaguc-services-config.xml /adaguc/adaguc-services-config.xml

ENV ADAGUC_SERVICES_CONFIG=/adaguc/adaguc-services-config.xml 

# Set up data dir, this is also configured in adaguc-server-config.xml
RUN mkdir -p /data/adaguc-autowms
RUN mkdir -p /data/adaguc-datasets

EXPOSE 8080

COPY start.sh /adaguc/

RUN chmod +x /adaguc/start.sh

RUN mkdir -p /adaguc/userworkspace/
RUN mkdir -p /adaguc/basedir/-

ENTRYPOINT /adaguc/start.sh
 
# docker build -t adaguc-server . 
# docker run -e EXTERNALADDRESS="http://127.0.0.1:8080/" -p 8080:8080 -v $HOME/data:/data -it adaguc-server 
# Check http://localhost:8080/adaguc-services/wms.cgi?service=wms&request=getcapabilities
# You can copy NetCDF's / GeoJSONS to your hosts ~/data/adaguc-autowms directory. This will be served through adaguc-server, via the source=<filename> key value pair. testdata.nc is copied there by default. 
# Directories $HOME/data/adaguc-datasets and $HOME/data/adaguc-autowms will be created if they do not exist.
# Copy your NetCDF/GeoJSON/HDF5 to $HOME/data/adaguc-autowms
# Files are are accessible via http://localhost:8080/adaguc-services/wms.cgi?source=testdata.nc&&service=WMS&request=GetCapabilities
# Testdata can be found here: http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/catalog.html
