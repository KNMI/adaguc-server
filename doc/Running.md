# Running adaguc-server with docker

[Back to readme](../Readme.md)

The easiest way to make use of the adaguc-server is via docker containers. Docker images for the adaguc system are hosted at [dockerhub](https://hub.docker.com/r/openearth/adaguc-server/). The required docker containers can be started with docker-compose. The docker compose file can be found in the Docker folder. Detailed instructions can be found below.


## Start adaguc via Docker (compose)

Adaguc comprises the following components run in separate containers:

1. adaguc-server
2. adaguc-viewer
3. nginx webserver
4. postgresql database

![Running adaguc-server via docker](./overview/adaguc-server-docker-compose.png)

## Directories and data
The most important directories are:
* adaguc-data: Put your NetCDF, HDF5, GeoJSON or PNG files inside this directory, these are referenced by your dataset configurations.
* adaguc-datasets: These are your dataset configuration files, defining a service. These are small XML files allowing you to customize the styling and aggregation of datafiles.  Datasets are referenced in the WMS service by the dataset=<Your datasetname> keyword.
* adaguc-autowms: Put your files here you want to have visualised automatically without any scanning. Go to the /autowms endpoint to see its contents.

We have to create the required directories. Easiest way is to follow the instructions below.

```
# Create directories
export ADAGUCHOME=$HOME
mkdir -p $ADAGUCHOME/adaguc-data
mkdir -p $ADAGUCHOME/adaguc-datasets
mkdir -p $ADAGUCHOME/adaguc-autowms
```

Now clone the source code and cd into the Docker folder

```
git clone https://github.com/KNMI/adaguc-server/
cd adaguc-server/Docker
```

A script is available to generate the required .env file needed for docker-compose:

```
# Generate environment for adaguc:
bash docker-compose-generate-env.sh \
  -a $ADAGUCHOME/adaguc-autowms \
  -d $ADAGUCHOME/adaguc-datasets \
  -f $ADAGUCHOME/adaguc-data \
  -p 443
# You can view or edit the file ./.env file
```

Now pull and build the containers:
```
docker-compose pull
docker-compose build
```

Once the steps above have been done, it is time to start:
```
docker-compose up -d --build && sleep 10
```

Visit the url as configured in the .env file under EXTERNALADDRESS. 

Note: The server runs with a self signed certificate, this means you get a warning. Accept this warning and add the exception.

If you now see the viewer, you have succesfully started the adaguc-server.

## Scanning datasets

Scan with the adaguc-server container:
```
docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh <optional datasetname>
```

## Visit the webservice:

The docker-compose-generate-env.sh tells you where you services can be accessed in the browser. Alternatively you can have a look at the ./adaguc-server/Docker/.env file
```
cat ./adaguc-server/Docker/.env

```

The webservices should now be accessible via :
```
https://<your hostname>/
```
or, if you specified a port other than 443
```
https://&lt;your hostname&gt;:&lt;port&gt;/
```

Note:
* _The first time you acces the service,  your browser will show a warning that there is a problem with the certificate. Make an exception for this service._

## To view logs:
```
docker logs -f my-adaguc-server
```

## To stop:
```
docker-compose down
```


