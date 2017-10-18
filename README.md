# adaguc-server
ADAGUC is a geographical information system to visualize netCDF files via the web. The software consists of a server side C++ application and a client side JavaScript application. The software provides several features to access and visualize data over the web, it uses OGC standards for data dissemination.

See http://dev.knmi.nl/projects/adagucserver/wiki for details

# Docker:
```
docker pull openearth/adaguc-server
mkdir -p $HOME/data/adaguc-autowms
mkdir -p $HOME/data/adaguc-datasets
docker run -e EXTERNALADDRESS="http://127.0.0.1:8080/" -p 8080:8080 -v $HOME/data:/data -it adaguc-server 

```
* Check http://localhost:8080/adaguc-services/wms.cgi?service=wms&request=getcapabilities
* You can copy NetCDF's / GeoJSONS to your hosts ~/data/adaguc-autowms directory. This will be served through adaguc-server, via the source=<filename> key value pair. testdata.nc is copied there by default. 
* Directories $HOME/data/adaguc-datasets and $HOME/data/adaguc-autowms will be created if they do not exist.
* Copy your NetCDF/GeoJSON/HDF5 to $HOME/data/adaguc-autowms
* Files are are accessible via http://localhost:8080/adaguc-services/wms.cgi?source=testdata.nc&&service=WMS&request=GetCapabilities
* Testdata can be found here: http://localhost:8080/adaguc-services/wms.cgi?source=http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/catalog.html
* Remote opendap can also be linked via http://opendap.knmi.nl/knmi/thredds/dodsC/ADAGUC/testsets/rgba_truecolor_images/butterfly_fromjpg_truecolor.nc
* Note that the value of source=<value> should officialy be URL encoded.
