# ADAGUC / adaguc-server
ADAGUC is a geographical information system to visualize netCDF files via the web. The software consists of a server side C++ application (ADAGUC-Server) and a client side JavaScript application (ADAGUC-Viewer). The software provides several features to access and visualize data over the web, it uses OGC standards for data dissemination.

# Useful links
 
* https://dev.knmi.nl/projects/adagucserver/wiki - For adaguc-server documentation 
* https://dev.knmi.nl/projects/adagucserver/wiki/Configuration - For configuration details.
* https://github.com/KNMI/adaguc-viewer - ADAGUC-viewer, a WMS client which can connect to ADAGUC-server
* https://knmi.github.io/adaguc-server/ - Gitbook documentation, currently information about supported data formats is written
* ```docker rm -f my-adaguc-server``` - For stopping and removing your docker container, useful when you want to start it with other arguments
* ```docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh``` - For updating adaguc-server datasets.
* ```docker exec --env ADAGUC_DATASET_MASK='^eprofile_L0_[0-9]{5}-[A-Z]\.xml$' -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh``` - For updating adaguc-server datasets where the datasetname matches the provided ADAGUC_DATASET_MASK.
* ```docker exec -i -t my-adaguc-server /adaguc/adaguc-server-checkfile.sh <file in autowms folder, e.g. testdata.nc>``` - For generating reports  about your files, provides feedback on what decisions adaguc-server makes based on your file metadata.

# Docker for adaguc-server:

A docker image for adaguc-server is available from [dockerhub](https://hub.docker.com/r/openearth/adaguc-server/). This image enables you to quickstart with adaguc-server, everything is configured and pre-installed. You can mount your own directories and configuration files from your workstation inside the docker container. This allows you to use and configure your own data from your workstation. 
Inside the docker container the paths for data and configuration files are always the same, this is useful for sharing dataset configuration files between different instances because configuration files can be the same across different environments. 
You can check the [docker-compose.yml](Docker/docker-compose.yml) file to quickstart with both adaguc-server and adaguc-viewer at the same time.

## Directories and data
The most important directories are:
* adaguc-data: Put your NetCDF, HDF5, GeoJSON or PNG files inside this directory, these are referenced by your dataset configurations.
* adaguc-datasets: These are your dataset configuration files, defining a service. These are small XML files allowing you to customize the styling and aggregation of datafiles. See satellite imagery example below. It is good practice to configure datasets to read data from the /data/adaguc-data folder, which can be mounted from your host machine. This ensures that dataset configurations will work on different environments where paths to the data can differ. Datasets are referenced in the WMS service by the dataset=<Your datasetname> keyword.
* adaguc-autowms: Put your files here you want to have visualised automatically without any scanning.
* adagucdb: Persistent place for the database files
* adaguc-logs: Logfiles are placed here, you can inspect these if something does not work as expected.

## Start the docker to serve your data using http *
__* Secure http is required if you want to use your services in GeoWeb, see below in the https section__
```
docker pull openearth/adaguc-server # Or build latest docker from this repo yourself with "docker build -t adaguc-server ."
export ADAGUCHOME=$HOME # You are free to set ADAGUCHOME to any directory you like
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-data
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-datasets
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-autowms
mkdir -p $ADAGUCHOME/adaguc-server-docker/adagucdb
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-logs && chmod 777 $ADAGUCHOME/adaguc-server-docker/adaguc-logs
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-security

export ADAGUCHOME=$HOME
docker run \
  -e EXTERNALADDRESS="http://`hostname`:8090/" \
  -p 8090:8080 \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-data:/data/adaguc-data \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-datasets:/data/adaguc-datasets \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-autowms:/data/adaguc-autowms \
  -v $ADAGUCHOME/adaguc-server-docker/adagucdb:/adaguc/adagucdb \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-logs:/var/log/adaguc \
  --tmpfs /tmp \
  --name my-adaguc-server \
  -d openearth/adaguc-server 

```
If the container does not want to run because the container name is aready in use, please do:
```
docker rm -f my-adaguc-server
```
The container should now be accessible via http://localhost:8090/adaguc-services/adagucserver?

## Serve on another address and/or port

By default adaguc-server docker is set to run on port 8090 and to serve content locally on http://127.0.0.1:8090/, configured via settings given to the docker run command. If you need to run adaguc-server on a different address and/or port, you can configure this with the EXTERNALADDRESS and port settings. The EXTERNALADDRESS setting is used by the WMS to describe the OnlineResource element in the WMS GetCapabilities document. The viewer needs this information to point correctly to WMS getmap requests. It is good practice to set the EXTERNALADDRESS to your systems publicy accessible name and port. Inside the docker container adaguc runs on port 8080, that is why the port mapping in the docker run command is set to 8080. It is also possible to run ADAGUC-server automatically via HTTPS, it will create a self signed certificate for you.

## Start the docker to serve your data securely using https*
__* Secure http is required if you want to show your ADAGUC services in GeoWeb__

For some cases it is required that your adagucserver is running over a secure connection (https). This can be the case when you want to open your services in a secure site. If you are trying to read http content on a https site, the browser blocks the http contents and warns you that you are trying to load insecure or mixed content; e.g. https and http in one site. The browser blocks this to garantee that the secure site remains safe.
To overcome this issue adaguc services can be served over https. During startup, the adaguc docker checks if you have provided a SSL certificate in the adaguc-security folder. If none available, the docker container will create a self signed SSL certificate for you; the certificate is stored in a keystore in the adaguc-security folder. By default the self signed certificate is not automatically trusted by your browser. You have to make an exception in your browser in order to use the services. This can be done by visiting one of the URL's (https://<your hostname>:8443/adaguc-services/adagucserver?) and confirm an exception. To overcome the security exception, you are free to add your own valid SSL certificate (from your certificate authority or letsencrypt) if you have one. The alias inside the keystore is currently 'tomcat' and the password is 'password'. Please note that you need to use your hostname in the URL instead of locahost when accessing your services, this is becauses services running over https will check the hostname for you. You also need to use https:// as prefix for your URL.

### Setup directories ###
```
docker pull openearth/adaguc-server # Or build latest docker from this repo yourself with "docker build -t adaguc-server ."
export ADAGUCHOME=$HOME # You are free to set ADAGUCHOME to any directory you like
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-data
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-datasets
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-autowms
mkdir -p $ADAGUCHOME/adaguc-server-docker/adagucdb
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-logs && chmod 777 $ADAGUCHOME/adaguc-server-docker/adaguc-logs
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-security
```
### Start adaguc-server ###
```
export ADAGUCHOME=$HOME && docker run \
  -e EXTERNALADDRESS="https://`hostname`:8443/" \
  -p 8443:8443 \
  -p 8090:8080 \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-data:/data/adaguc-data \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-datasets:/data/adaguc-datasets \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-autowms:/data/adaguc-autowms \
  -v $ADAGUCHOME/adaguc-server-docker/adagucdb:/adaguc/adagucdb \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-logs:/var/log/adaguc \
  -v $ADAGUCHOME/adaguc-server-docker/adaguc-security:/adaguc/security \
  --tmpfs /tmp \
  --name my-adaguc-server \
  -d openearth/adaguc-server 
```
If the container does not want to run because the container name is aready in use, please do:
```
docker rm -f my-adaguc-server
```

Find your hostname via the ```hostname``` command, you need your hostname to access your service via HTTPS.
```
hostname
> bhw485.knmi.nl
```

The container is now accessible via :
```https://<your hostname>:8443/adaguc-services/adagucserver?```

Note:
* _The first time you acces the service,  your browser will show a warning that there is a problem with the certificate. Make an exception for this service._
* _The following examples are made with the server running over HTTP on port 8090. Replace the prefix with the correct information if you are running over https._

## Visualize a NetCDF file via autowms

```
# Put a NetCDF testfile into your autowms folder
export ADAGUCHOME=$HOME
curl -kL https://github.com/KNMI/adaguc-server/raw/master/data/datasets/testdata.nc > $ADAGUCHOME/adaguc-server-docker/adaguc-autowms/testdata.nc
```
AutoWMS files are referenced via the source= key value pair in the URL. Filenames must be URLEncoded. Supported files are NetCDF, HDF5 and GeoJSON.
This file is now accessible via http://localhost:8090/adaguc-services/adagucserver?source=testdata.nc&service=WMS&request=GetCapabilities (An XML document about this NetCDF is shown)

A GetMap request looks like http://localhost:8090/adaguc-services/adagucserver?%26source%3Dtestdata%2Enc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=1249&HEIGHT=716&CRS=EPSG%3A4326&BBOX=34.29823486999199,-24.906440270168137,69.33472934198558,36.21169044983186&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&0.47843952121424993

You can visualize this link in the adaguc-viewer via "Add data", for example in http://geoservices.knmi.nl/viewer2.0/

Other testdata can be found here: http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/catalog.html. 

## Test your own dataset configuration for styling, aggregations, etc ...

Get a dataset configurationfile:
```
curl -kL https://raw.githubusercontent.com/KNMI/adaguc-server/master/data/config/datasets/dataset_a.xml > $ADAGUCHOME/adaguc-server-docker/adaguc-datasets/dataset_a.xml
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
export ADAGUCHOME=$HOME
cd $ADAGUCHOME/adaguc-server-docker/adaguc-data/
wget -nc -r -l2 -A.h5   -I /knmi/thredds/fileServer/,/knmi/thredds/catalog/ 'http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/testsets/projectedgrids/meteosat/catalog.html'
ls opendap.knmi.nl/knmi/thredds/fileServer/ADAGUC/testsets/projectedgrids/meteosat/
```

Create a dataset configuration file named $ADAGUCHOME/adaguc-server-docker/adaguc-datasets/sat.xml :
```xml
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
  <!-- Custom styles -->
  <Legend name="gray" type="colorRange">
    <palette index="0"   red="0"   green="0"   blue="0" alpha="255"/>
    <palette index="240" red="255" green="255"   blue="255"/>
  </Legend>
  <Legend name="gray-trans" type="colorRange">
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
    <Legend fixed="true">gray-trans</Legend>
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
  <Layer type="cascaded" hidden="false">
    <Name force="true">baselayer</Name>
    <Title>NPS - Natural Earth II</Title>
    <WMSLayer service="http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?" layer="naturalearth2"/>
    <LatLonBox minx="-180"  miny="-90" maxx="180" maxy="90"/>
  </Layer>
  <!-- Layer with name overlay from geoservices.knmi.nl -->
  <Layer type="cascaded" hidden="false">
    <Name force="true">overlay</Name>
    <Title>NPS - Natural Earth II</Title>
    <WMSLayer service="http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?" layer="world_line_thick"/>
    <LatLonBox minx="-180"  miny="-90" maxx="180" maxy="90"/>
    <WMSFormat name="image/png32"/>
  </Layer>
  <!-- Layer with name grid10 from geoservices.knmi.nl -->
  <Layer type="grid">
    <Name force="true">grid10</Name>
    <Title>grid 10 degrees</Title>
    <Grid resolution="10"/>
    <WMSFormat name="image/png32"/>
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

## Aggregation of ERA interim model data

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Configuration>
  <!-- Custom styles -->
  <Legend name="ColorPalette" type="colorRange">
    <palette index="0" red="0" green="0" blue="255" />
    <palette index="80" red="0" green="255" blue="255" />
    <palette index="120" red="0" green="255" blue="0" />
    <palette index="160" red="255" green="255" blue="0" />
    <palette index="240" red="255" green="0" blue="0" />
  </Legend>
  <Style name="era5_style">
    <Legend fixed="true">ColorPalette</Legend>
    <ContourLine width="3" linecolor="#ff0000" textcolor="#ff0000" textformatting="%2.0f" classes="300" />
    <ContourLine width="0.3" linecolor="#444444" textcolor="#444444" textformatting="%2.0f" interval="100" />
    <Min>0</Min>
    <Max>1500</Max>
    <ValueRange min="350" max="10000000" />
    <RenderMethod>nearest,contour,nearestcontour</RenderMethod>
    <NameMapping name="nearest" title="IVT 0-1500" abstract="IVT" />
  </Style>
  <!-- Layer with name baselayer from geoservices.knmi.nl -->
  <Layer type="cascaded" hidden="false">
    <Name force="true">baselayer</Name>
    <Title>NPS - Natural Earth II</Title>
    <WMSLayer service="http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?" layer="naturalearth2" />
    <LatLonBox minx="-180" miny="-90" maxx="180" maxy="90" />
  </Layer>
  <!-- Layer with name overlay from geoservices.knmi.nl -->
  <Layer type="cascaded" hidden="false">
    <Name force="true">overlay</Name>
    <Title>NPS - Natural Earth II</Title>
    <WMSLayer service="http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?" layer="world_line_thick" />
    <LatLonBox minx="-180" miny="-90" maxx="180" maxy="90" />
  </Layer>
  <!-- Layer with name grid10 from geoservices.knmi.nl -->
  <Layer type="grid">
    <Name force="true">grid10</Name>
    <Title>grid 10 degrees</Title>
    <Grid resolution="10" />
    <WMSFormat name="image/png32" />
  </Layer>
  <!-- Layer with name ivt (Integrated Water Vapor Transport) with ERA interim data -->
  <Layer type="database">
    <Variable>ivt</Variable>
    <FilePath filter="^IVT.ERAINT.2.*\.nc">/data/adaguc-data/TWEX/ERAInt/</FilePath>
    <Styles>era5_style,auto</Styles>
  </Layer>
</Configuration>

```

## Make a movie of the sat dataset

You can use the python script at [data/python/createmovie.py](data/python/createmovie.py)

Demo image:
http://adaguc.knmi.nl/data/msg_hrvis_demo.gif
![Loop of MSG HRVIS made with adaguc-server](http://adaguc.knmi.nl/data/msg_hrvis_demo.gif)

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

This WMS URL can be visualized in the viewer by using "Add data". 

## Docker compose with server and viewer:

The compose file is located here: [Docker/docker-compose.yml](Docker/docker-compose.yml)

Prebuilt images are available at https://hub.docker.com/ through openearth:
* https://hub.docker.com/r/openearth/adaguc-viewer/
* https://hub.docker.com/r/openearth/adaguc-server/

                     
To get an instance online with docker compose: 
```
git clone https://github.com/KNMI/adaguc-server/
export ADAGUCHOME=$HOME
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-data
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-datasets
mkdir -p $ADAGUCHOME/adaguc-server-docker/adaguc-autowms

cd adaguc-server/Docker

# Generate environment for adaguc:
bash docker-compose-generate-env.sh \
  -a $ADAGUCHOME/adaguc-server-docker/adaguc-autowms \
  -d $ADAGUCHOME/adaguc-server-docker/adaguc-datasets \
  -f $ADAGUCHOME/adaguc-server-docker/adaguc-data \
  -p 443
# You can view or edit the file ./.env

docker-compose pull
docker-compose build
docker-compose up -d && sleep 10

# Visit the url as configured in the .env file under EXTERNALADDRESS
# The server runs with a self signed certificate, this means you get a warning. Add an exception.

# Scan datasets:
docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh        

# To view logs:
docker exec -it my-adaguc-server tail -f  /var/log/adaguc/adaguc-server.log
```
The docker-compose-generate-env.sh tells you where you services can be accessed in the browser. Alternatively you can have a look at the ./adaguc-server/Docker/.env file
```
cat ./adaguc-server/Docker/.env

```

To stop:
```
## Press CTRL+C
docker-compose down
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
* http://localhost:8090/adaguc-services/servicehealth? For getting overview and status of available datasets


