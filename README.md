# adaguc-server
ADAGUC is a geographical information system to visualize netCDF files via the web. The software consists of a server side C++ application and a client side JavaScript application. The software provides several features to access and visualize data over the web, it uses OGC standards for data dissemination.

See http://dev.knmi.nl/projects/adagucserver/wiki for details

# Docker:
```
docker pull openearth/adaguc-server
mkdir ~/data/
docker network create --subnet=172.18.0.0/16 adagucnet
docker run -i -t --net adagucnet --ip 172.18.0.2 -v $HOME/data:/data openearth/adaguc-server
```
Visit URL http://172.18.0.2:8080/adaguc-viewer/?service=http%3A%2F%2F172.18.0.2%3A8080%2Fadaguc-services%2Fadagucserver%3Fservice%3Dwms%26request%3Dgetcapabilities%26source%3Dtestdata.nc

You can copy NetCDF's / GeoJSONS to your hosts ~/data directory. This will be served through adaguc-server, via the source=<filename> key value pair. testdata.nc is copied there by default. See example URL above.
