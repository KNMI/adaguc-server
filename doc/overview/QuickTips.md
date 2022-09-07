# Quick tips:

[Back to readme](../../Readme.md)

## Scanning datasets

Scan with the adaguc-server container:
```
docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh <optional datasetname>
```

## Endpoints

* https://&lt;your hostname&gt;:&lt;port&gt;/adagucserver? Will be forwarded automaticall to /wms or /wcs depending on the service type
* https://&lt;your hostname&gt;:&lt;port&gt;/wms? For serving Web Map Services
* https://&lt;your hostname&gt;:&lt;port&gt;/wcs? For serving Web Coverage Services
* https://&lt;your hostname&gt;:&lt;port&gt;/adagucopendap/ For serving OpenDAP services
* https://&lt;your hostname&gt;:&lt;port&gt;/autowms? For getting the list of visualizable resources

Print your ./Docker/.env file for current docker-compose settinhs with `cat ./Docker/.env`
