# Installation

## Docker

```
git clone https://github.com/KNMI/adaguc-server/
export ADAGUCHOME=$HOME
mkdir -p $ADAGUCHOME/adaguc-data
mkdir -p $ADAGUCHOME/adaguc-datasets
mkdir -p $ADAGUCHOME/adaguc-autowms

cd adaguc-server/Docker

# Generate environment for adaguc:
bash docker-compose-generate-env.sh \
  -a $ADAGUCHOME/adaguc-autowms \
  -d $ADAGUCHOME/adaguc-datasets \
  -f $ADAGUCHOME/adaguc-data \
  -p 443
# You can view or edit the file ./.env

docker-compose pull
docker-compose build
docker-compose up -d --build && sleep 10

# Visit the url as configured in the .env file under EXTERNALADDRESS
# The server runs with a self signed certificate, this means you get a warning. Add an exception.
```

## For developing

explanation of folder structure
venv, cmake, vscode 