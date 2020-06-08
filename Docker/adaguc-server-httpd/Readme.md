# Build

docker build --rm -t openearth/adaguc-server-httpd . && \
docker rm -f adaguc-server-httpd ; \
docker run -it \
  --name adaguc-server-httpd \
  --network=host \
  -e ADAGUC_DB="host=localhost port=5432 user=adaguc password=adaguc dbname=adaguc" \
  -e EXTERNALADDRESS="http://localhost:8080" \
  -v /data/adaguc-testsets/adaguc-data:/data/adaguc-data \
  -v /data/adaguc-testsets/adaguc-datasets/:/data/adaguc-datasets \
  -v /dev/null:/var/log/adaguc-server.log \
   openearth/adaguc-server-httpd

# Now visit
http://localhost:8080/adaguc-server?