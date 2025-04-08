# Quick tips:

[Back to readme](../../Readme.md)

## Scanning datasets

List datasets: 
```
docker exec -i -t my-adaguc-server /adaguc/scan.sh -l
```

Scan a dataset:
```
docker exec -i -t my-adaguc-server /adaguc/scan.sh -d <datasetname>
```

Scan a file:
```
docker exec -i -t my-adaguc-server /adaguc/scan.sh -f <filename to scan>
```

Scan all datasets: 
```
docker exec -i -t my-adaguc-server /adaguc/scan.sh
```

Similar command, but in this case we go inside the container and call the script directly:

```
docker exec -it my-adaguc-server bash
cd /adaguc
. adaguc-server-chkconfig.sh
${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},<datasetname>
```


Now you can use all options as documented in [doc/info/Commandline.md](../../doc/info/Commandline.md)

## Listing files in your docker container

A tip to list the actual contents in the docker container is to do:



```
# List data files in your container
docker exec -i -t my-adaguc-server bash -c "ls -lrt /data/adaguc-data"
```

```
# List dataset configurations in your container
docker exec -i -t my-adaguc-server bash -c "ls -lrt /data/adaguc-datasets"
```


## Endpoints

* https://&lt;your hostname&gt;:&lt;port&gt;/adagucserver? Will be forwarded automaticall to /wms or /wcs depending on the service type
* https://&lt;your hostname&gt;:&lt;port&gt;/wms? For serving Web Map Services
* https://&lt;your hostname&gt;:&lt;port&gt;/wcs? For serving Web Coverage Services
* https://&lt;your hostname&gt;:&lt;port&gt;/adagucopendap/ For serving OpenDAP services
* https://&lt;your hostname&gt;:&lt;port&gt;/autowms? For getting the list of visualizable resources

Print your ./Docker/.env file for current docker-compose settings with `cat ./Docker/.env`

## Updating the layer metadata table


This refreshes the layermetadata table immediately. This is also done automatically every minute by the server itself, and is also done when you scan a dataset or file.
 
 ```
 docker exec -i -t my-adaguc-server /adaguc/scan.sh -m
 ```