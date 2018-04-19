# ADAGUC / adaguc-server
ADAGUC is a geographical information system to visualize netCDF files via the web. The software consists of a server side C++ application and a client side JavaScript application. The software provides several features to access and visualize data over the web, it uses OGC standards for data dissemination.

See https://dev.knmi.nl/projects/adagucserver/wiki for details

# Docker for adaguc-server:

A docker image for adaguc-server is available from dockerhub. This image enables you to quickstart with adaguc-server, everything is configured and pre-installed. You can mount your own directories and configuration files from your workstation inside the docker container. This allows you to use and configure your own data from your workstation. There is no need to go inside the docker container. Inside the docker container the paths for data and configuration files are always the same, this is useful for sharing dataset configuration files between different instances. This docker image can be used in production environments as well. See the docker-compose file below to quickstart with both adaguc-server and adaguc-viewer at the same time.

## Directories and data

The most important directories are:
* adaguc-data: Put your NetCDF, HDF5, GeoJSON or PNG files inside this directory, these are referenced by your dataset configurations.
* adaguc-datasets: These are your dataset configuration files, defining a service. These are small XML files allowing you to customize the styling and aggregation of datafiles. See satellite imagery example below. It is good practice to configure datasets to read data from the /data/adaguc-data folder, which can be mounted from your host machine. This ensures that dataset configurations will work on different environments where paths to the data can differ. Datasets are referenced in the WMS service by the dataset=<Your datasetname> keyword.
* adaguc-autowms: Put your files here you want to have visualised automatically without any scanning.
* adagucdb: Persistent place for the database files
* adaguc-logs: Logfiles are placed here, you can inspect these if something does not work as expected.

## Serve on another address and/or port

By default adaguc-server docker is set to run on port 8090 and to serve content locally on http://127.0.0.1:8090/, configured via settings given to the docker run command. If you need to run adaguc-server on a different address and/or port, you can configure this with the EXTERNALADDRESS and port settings. The EXTERNALADDRESS setting is used by the WMS to describe the OnlineResource element in the WMS GetCapabilities document. The viewer needs this information to point correctly to WMS getmap requests. It is good practice to set the EXTERNALADDRESS to your systems publicy accessible name and port. Inside the docker container adaguc runs on port 8080, that is why the port mapping in the docker run command is set to 8080.

## To start with the docker image, do:
```
docker pull openearth/adaguc-server # Or build latest docker from this repo yourself with "docker build -t adaguc-server ."

mkdir -p $HOME/adaguc-server-docker/adaguc-data
mkdir -p $HOME/adaguc-server-docker/adaguc-datasets
mkdir -p $HOME/adaguc-server-docker/adaguc-autowms
mkdir -p $HOME/adaguc-server-docker/adagucdb
mkdir -p $HOME/adaguc-server-docker/adaguc-logs && chmod 777 $HOME/adaguc-server-docker/adaguc-logs
mkdir -p $HOME/adaguc-server-docker/adaguc-security

docker run \
  -e EXTERNALADDRESS="http://`hostname`:8090/" \
  -p 8090:8080 \
  -v $HOME/adaguc-server-docker/adaguc-data:/data/adaguc-data \
  -v $HOME/adaguc-server-docker/adaguc-datasets:/data/adaguc-datasets \
  -v $HOME/adaguc-server-docker/adaguc-autowms:/data/adaguc-autowms \
  -v $HOME/adaguc-server-docker/adagucdb:/adaguc/adagucdb \
  -v $HOME/adaguc-server-docker/adaguc-logs:/var/log/adaguc \
  --name my-adaguc-server \
  -it openearth/adaguc-server 

```
If the container does not want to run because the container name is aready in use, please do:
```
docker rm my-adaguc-server
```
The container should now be accessible via http://localhost:8090/adaguc-services/adagucserver?

## Serving data using HTTPS

For some cases it is required that the server is running over a secure connection (https). This can be the case when a secure site tries to load your services over normal http. The browser blocks your service and warns you that you are trying to load mixed content; e.g. https and http in one site. The browser blocks this to garantee that the site remains safe.
To overcome this issue adaguc services can be served over https. During startup, the adaguc docker checks if you have provided a SSL certificate in the adaguc-security folder. If none available, it creates a self signed SSL certificate for you, the certificate is stored in a keystore in the adaguc-security folder. By default the self signed certificate is not automatically trusted by your browser. You have to make an exception in your browser in order to use the services. This can be done by visiting one of the URL's (https://localhost:8443/adaguc-services/adagucserver?) and confirm an exception. To overcome the security exception, you are free to add your own valid SSL certificate (from your certificate authority or letsencrypt) if you have one. The alias inside the keystore is currently 'tomcat' and the password is 'password'. 

```
docker run \
  -e EXTERNALADDRESS="https://`hostname`:8443/" \
  -p 8443:8443 \
  -v $HOME/adaguc-server-docker/adaguc-data:/data/adaguc-data \
  -v $HOME/adaguc-server-docker/adaguc-datasets:/data/adaguc-datasets \
  -v $HOME/adaguc-server-docker/adaguc-autowms:/data/adaguc-autowms \
  -v $HOME/adaguc-server-docker/adagucdb:/adaguc/adagucdb \
  -v $HOME/adaguc-server-docker/adaguc-logs:/var/log/adaguc \
  -v $HOME/adaguc-server-docker/adaguc-security:/adaguc/security \
  --name my-adaguc-server \
  -it openearth/adaguc-server 
```

The container is now accessible via :
https://localhost:8443/adaguc-services/adagucserver?
Remember: the first time you acces this link your browser will show a warning that there is a problem with the certificate. Make an exception for this service.

## Visualize a NetCDF file via autowms

```
# Put a NetCDF testfile into your autowms folder
curl -kL https://github.com/KNMI/adaguc-server/raw/master/data/datasets/testdata.nc > $HOME/adaguc-server-docker/adaguc-autowms/testdata.nc
```
AutoWMS files are referenced via the source= key value pair in the URL. Filenames must be URLEncoded. Supported files are NetCDF, HDF5 and GeoJSON.
This file is now accessible via http://localhost:8090/adaguc-services/adagucserver?source=testdata.nc

You can visualize this link in the adaguc-viewer via "Add data", for example in http://geoservices.knmi.nl/viewer2.0/

Other testdata can be found here: http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/catalog.html. 

## Test your own dataset configuration for styling, aggregations, etc ...

Get a dataset configurationfile:
```
curl -kL https://raw.githubusercontent.com/KNMI/adaguc-server/master/data/config/datasets/dataset_a.xml > $HOME/adaguc-server-docker/adaguc-datasets/dataset_a.xml
```
Now update the db:
```
docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh dataset_a
```
Dataset configurations are referenced via the dataset= key value pair in the URL.
This dataset is now accessible via 
http://localhost:8090/adaguc-services/adagucserver?service=wms&request=getcapabilities&dataset=dataset_a&

## Aggregation of hi-res satellite imagery

First download a sequence of satellite data from opendap.knmi.nl:
```
cd $HOME/adaguc-server-docker/adaguc-data/
wget -nc -r -l2 -A.h5   -I /knmi/thredds/fileServer/,/knmi/thredds/catalog/ 'http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/testsets/projectedgrids/meteosat/catalog.html'
ls opendap.knmi.nl/knmi/thredds/fileServer/ADAGUC/testsets/projectedgrids/meteosat/
```

Put a dataset configuration file named sat.xml inside $HOME/adaguc-server-docker/adaguc-datasets/ :
```xml
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
  <!-- Custom styles -->
  <Legend name="gray" type="colorRange">
    <palette index="0"   red="0"   green="0"   blue="0" alpha="0"/>
    <palette index="240" red="255" green="255"   blue="255"/>
  </Legend>
  <Style name="hrvis_0till30000">
    <Legend fixed="true">gray</Legend>
    <Min>0</Min>
    <Max>30000</Max>
    <RenderMethod>nearest</RenderMethod>
    <RenderSettings renderer="gd"/>
    <NameMapping name="nearest"        title="Albedo 0-30000" abstract="Albedo values from satellite imagery"/>
  </Style>
  <Style name="hrvis_0till30000_transparent">
    <Legend fixed="true">gray</Legend>
    <Min>0</Min>
    <Max>30000</Max>
    <RenderMethod>nearest</RenderMethod>
    <RenderSettings renderer="cairo"/>
    <NameMapping name="nearest"        title="Albedo 0-30000 transparent" abstract="Albedo values from satellite imagery with the lower values made transparent"/>
  </Style>
  <!-- Layers -->
  <Layer type="database">
    <Projection proj4="+proj=geos +lon_0=0.000000 +lat_0=0 +h=35807.414063 +a=6378.169000 +b=6356.583984"/>
    <Name>HRVIS</Name>
    <Title>HRVIS</Title>
    <Variable>image1.image_data</Variable>
    <FilePath
      filter="^METEOSAT_(8|9|10|11).*EUROPEHVIS.*\.h5">/data/adaguc-data/opendap.knmi.nl/knmi/thredds/fileServer/ADAGUC/testsets/projectedgrids/meteosat/</FilePath>
    <DataReader>HDF5</DataReader>
    <Dimension name="time" interval="PT15M">time</Dimension>
    <Styles>hrvis_0till30000,hrvis_0till30000_transparent</Styles>
  </Layer>
</Configuration>
```
Now update the db wit the sat dataset:
```
docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh sat
```
The following URL can be used in the viewer:
http://localhost:8090/adaguc-services/adagucserver?service=wms&request=getcapabilities&dataset=sat&

You can use this URL for example in http://geoservices.knmi.nl/viewer2.0/

## Opendap services can be visualized

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

## Docker compose with server and viewer:

The compose file is located here: [Docker/docker-compose.yml](Docker/docker-compose.yml)

Prebuilt images are available at https://hub.docker.com/ through openearth:
* https://hub.docker.com/r/openearth/adaguc-viewer/
* https://hub.docker.com/r/openearth/adaguc-server/

                     
To get an instance online with docker compose: 
```
cd ./adaguc-server
docker pull openearth/adaguc-viewer
docker pull openearth/adaguc-server

mkdir -p $HOME/adaguc-server-docker/adaguc-data
mkdir -p $HOME/adaguc-server-docker/adaguc-datasets
mkdir -p $HOME/adaguc-server-docker/adaguc-autowms
mkdir -p $HOME/adaguc-server-docker/adagucdb 
mkdir -p $HOME/adaguc-server-docker/adaguc-logs && chmod 777 $HOME/adaguc-server-docker/adaguc-logs
mkdir -p $HOME/adaguc-server-docker/adaguc-security

docker-compose -f ./Docker/docker-compose.yml up 
```
The following services are now available:
* viewer at http://localhost:8091/adaguc-viewer/ 
* server http://localhost:8090/adaguc-services/adagucserver? 

To stop:
```
## Press CTRL+C
docker-compose down
```

Use the following command to scan datasets:
```
docker exec -i -t adaguc-server /adaguc/adaguc-server-updatedatasets.sh <your dataset name>
```

## OpenDAP

Adaguc can serve data via OpenDAP. The format is 
```
http(s)://<yourhost>/adaguc-services/adagucopendap/<dataset_name>/<layer_name>
```

http://localhost:8090/adaguc-services/adagucopendap/dataset_a/testdata

Opendap endpoints can be checked by testing the following URL's:
* http://localhost:8090/adaguc-services/adagucopendap/dataset_a/testdata.das
* http://localhost:8090/adaguc-services/adagucopendap/dataset_a/testdata.dds

You can dump the header or visualize with:
* ncdump -h http://localhost:8090/adaguc-services/adagucopendap/dataset_a/testdata
* ncview http://localhost:8090/adaguc-services/adagucopendap/dataset_a/testdata

## Endpoints

* http://localhost:8090/adaguc-services/adagucserver? Will be forwarded automaticall to /wms or /wcs depending on the service type
* http://localhost:8090/adaguc-services/wms? For serving Web Map Services
* http://localhost:8090/adaguc-services/wcs? For serving Web Coverage Services
* http://localhost:8090/adaguc-services/adagucopendap/ For serving OpenDAP services
* http://localhost:8090/adaguc-services/autowms? For getting the list of visualizable resources

