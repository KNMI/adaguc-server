# Quick tips:

[Back to readme](../../Readme.md)

## Scanning datasets

Scan with the adaguc-server container:
```
docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh <optional datasetname>
```

Similar command, but in this case we go inside the container and call the script directly:

```
docker exec -it my-adaguc-server bash
cd /adaguc
. ./adaguc-server-chkconfig.sh
${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},<optional datasetname>
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
