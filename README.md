# ADAGUC / adaguc-server
ADAGUC is a geographical information system to visualize netCDF files via the web. The software consists of a server side C++ application and a client side JavaScript application. The software provides several features to access and visualize data over the web, it uses OGC standards for data dissemination.

See http://dev.knmi.nl/projects/adagucserver/wiki for details

# Docker for adaguc-server:
```
docker pull openearth/adaguc-server

rm -rf /tmp/adaguc-database
rm -rf /tmp/adaguc-logs


mkdir -p $HOME/adaguc-server-docker/adaguc-data
mkdir -p $HOME/adaguc-server-docker/adaguc-datasets
mkdir -p $HOME/adaguc-server-docker/adaguc-autowms
mkdir -p /tmp/adaguc-database && chmod 760 /tmp/adaguc-database
mkdir -p /tmp/adaguc-logs && chmod 760 /tmp/adaguc-logs

docker run \
  -e EXTERNALADDRESS="http://127.0.0.1:8090/" \
  -p 8090:8080 \
  -v $HOME/adaguc-server-docker/adaguc-data:/data/adaguc-data \
  -v $HOME/adaguc-server-docker/adaguc-datasets:/data/adaguc-datasets \
  -v $HOME/adaguc-server-docker/adaguc-autowms:/data/adaguc-autowms \
  -v /tmp/adaguc-database:/adaguc/adagucdb \
  -v /tmp/adaguc-logs:/var/log/adaguc \
  -it adaguc-server 

```
# Test a NetCDF file
```
Put a NetCDF testfile into your autowms folder
curl -kL https://github.com/KNMI/adaguc-server/raw/master/data/datasets/testdata.nc > $HOME/adaguc-server-docker/adaguc-autowms/testdata.nc
```
This file is now accessible via http://localhost:8090/adaguc-services/adagucserver?source=testdata.nc
You can visualize this link in the adaguc-viewer via "Add data"

# Test your own configuration for styling, aggregations, etc ...

curl -kL https://raw.githubusercontent.com/KNMI/adaguc-server/master/data/config/datasets/dataset_a.xml > $HOME/adaguc-server-docker/adaguc-datasets/dataset_a.xml
This dataset is now accessible via http://localhost:8090/adaguc-services/adagucserver?service=wms&request=getcapabilities&dataset=dataset_a

# Docker compose with server and viewer:

The compose file is located here: [Docker/docker-compose.yml](Docker/docker-compose.yml)

Prebuilt images are available at https://hub.docker.com/ through openearth:
* https://hub.docker.com/r/openearth/adaguc-viewer/
* https://hub.docker.com/r/openearth/adaguc-server/

                     
To get a instance online with docker compose: 
```
cd ./adaguc-server/Docker
docker pull openearth/adaguc-viewer
docker pull openearth/adaguc-server
docker-compose up 
```
In working directory go to:
* viewer at http://localhost:8091/adaguc-viewer/ 
* server http://localhost:8090/adaguc-services/adagucserver? 

To stop:
```
docker-compose down
```

The following directories will be created if they do not exist:
* $HOME/data/adaguc-datasets 
* $HOME/data/adaguc-autowms 

# Use your own data
Copy your data files to $HOME/data/adaguc-autowms. Files are are accessible by linking them via the source= key value pair. Filenames must be URLEncoded. Supported files are NetCDF, HDF5 and GeoJSON.
The example file 'testdata.nc' is accessible via http://localhost:8090/adaguc-services/adagucserver?source=testdata.nc&service=WMS&request=GetCapabilities

Files can be visualized in the adaguc-viewer via:
* Go to http://localhost:8091/adaguc-viewer/
* Add service http://localhost:8090/adaguc-services/adagucserver?source=testdata.nc via "Add data"
* A direct link is: http://localhost:8091/adaguc-viewer/?service=http%3A%2F%2Flocalhost%3A8090%2Fadaguc-services%2Fadagucserver%3Fsource%3Dtestdata.nc

Testdata can be found here: http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/catalog.html. 

# Custom datasets
It is also possible to configure new datasets with custom styling and create aggregations over many files. Check https://dev.knmi.nl/projects/adagucserver/wiki/ for more information

* Copy your XML configurations to $HOME/data/adaguc-datasets
* Datasets are accessible via http://localhost:8090/adaguc-services/adagucserver?service=wms&request=getcapabilities&dataset=dataset_a

# Opendap services can be visualized

The following OpenDAP URL can be visualized:
```
http://opendap.knmi.nl/knmi/thredds/dodsC/omi/OMI___OPER_R___TYTRCNO_L3/TYTRCNO/OMI___OPER_R___TYTRCNO_3.nc
```
To provide it to the source=<file location> parameter it must be be URL encoded:
```
http%3A%2F%2Fopendap.knmi.nl%2Fknmi%2Fthredds%2FdodsC%2Fomi%2FOMI___OPER_R___TYTRCNO_L3%2FTYTRCNO%2FOMI___OPER_R___TYTRCNO_3.nc
```

The ADAGUC WMS URL becomes: 
```
http://localhost:8090/adaguc-services/adagucserver?source=http%3A%2F%2Fopendap.knmi.nl%2Fknmi%2Fthredds%2FdodsC%2Fomi%2FOMI___OPER_R___TYTRCNO_L3%2FTYTRCNO%2FOMI___OPER_R___TYTRCNO_3.nc&service=wms&request=getcapabilities
```

This WMS URL can be visualized in the viewer by using "Add data". (http://localhost:8091/adaguc-viewer/ if you use the compose)

# Allowing other hosts in the viewer

In the docker container for adaguc-viewer, at location /var/www/html/adaguc-viewer/config.php there is a list with all allowed hostnames. Add your own hostname if you want to allow data visualization from your own host. See https://dev.knmi.nl/projects/adagucviewer/wiki/Configuration for details.

The default file is located here: https://github.com/KNMI/adaguc-viewer/blob/master/Docker/config.php
