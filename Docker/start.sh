#!/bin/bash

### Setup truststore and keystore ###
ADAGUC_SERVICES_SECURITY=/adaguc/security

# Get configured hostname from the EXTERNALADDRESS env
HOSTNAME=${EXTERNALADDRESS}
 
# Remove protocol part of url  #
HOSTNAME="${HOSTNAME#http://}" && HOSTNAME="${HOSTNAME#https://}" && HOSTNAME="${HOSTNAME#ftp://}" && HOSTNAME="${HOSTNAME#scp://}" && HOSTNAME="${HOSTNAME#scp://}" && HOSTNAME="${HOSTNAME#sftp://}"
 
# Remove username and/or username:password part of URL  #
HOSTNAME="${HOSTNAME#*:*@}" && HOSTNAME="${HOSTNAME#*@}"
 
# Remove rest of urls #
HOSTNAME=${HOSTNAME%%/*} && HOSTNAME=${HOSTNAME%%:*}
 
# Show domain name only #
echo "Derived hostname is $HOSTNAME"

#Setup keystore
export KEYSTOREUSERID=$(stat -c "%u" ${ADAGUC_SERVICES_SECURITY})
echo "Got ${KEYSTOREUSERID} from owner of dir ${ADAGUC_SERVICES_SECURITY}"
# Create security user
echo "Using KEYSTOREUSERID : ${KEYSTOREUSERID}"
useradd --shell /bin/bash -u ${KEYSTOREUSERID} -o -c "" -m useradaguc

# If needed create a self signed certificate in a keystore for serving over HTTPS
if [ ! -f ${ADAGUC_SERVICES_SECURITY}/keystore.jks ]; then
  echo "Generating self signed certificate for HTTPS with hostname ${HOSTNAME}"
  runuser -l useradaguc -c "keytool -genkey -noprompt -keypass password -alias tomcat \
  -keyalg RSA -storepass password -keystore ${ADAGUC_SERVICES_SECURITY}/keystore.jks -deststoretype pkcs12 \
  -dname CN=${HOSTNAME}"
else
  echo "Using provided certificate for HTTPS"
fi

# If needed create a truststore based on java truststore
if [ ! -f ${ADAGUC_SERVICES_SECURITY}/truststore.ts ]; then
  echo "Using default truststore from /etc/pki/java/cacerts"
  runuser -l useradaguc -c "cp /etc/pki/java/cacerts ${ADAGUC_SERVICES_SECURITY}/truststore.ts"
fi

### Make sure that this service trusts itself by adding its certificate to the trust store ###

# 1) Export certificate from a keystore to a file called adaguc-services-cert.pem
keytool -export -alias tomcat -rfc -file  ${ADAGUC_SERVICES_SECURITY}/adaguc-services-cert.pem -keystore ${ADAGUC_SERVICES_SECURITY}/keystore.jks -storepass password

# 2) Put this certificate from adaguc-services-cert.pem into the truststore
keytool -delete -alias adagucservicescert -keystore ${ADAGUC_SERVICES_SECURITY}/truststore.ts -storepass changeit -noprompt
keytool -import -v -trustcacerts -alias adagucservicescert -file  ${ADAGUC_SERVICES_SECURITY}/adaguc-services-cert.pem -keystore ${ADAGUC_SERVICES_SECURITY}/truststore.ts -storepass changeit -noprompt

# Create CA for tokenapi: file and key for  authority /O=KNMI/OU=RDWDT/CN=adaguc-services_ca_tokenapi"

if [ ! -f ${ADAGUC_SERVICES_SECURITY}/adaguc-services-ca.cert ]; then

openssl req \
    -new \
    -newkey rsa:4096 \
    -days 365 \
    -nodes \
    -x509 \
    -subj "/O=KNMI/OU=RDWDT/CN=adaguc-services_ca_tokenapi" \
    -keyout ${ADAGUC_SERVICES_SECURITY}/adaguc-services-ca.key \
    -out ${ADAGUC_SERVICES_SECURITY}/adaguc-services-ca.cert

# Put this CA in the truststore
keytool -delete -alias adaguc-services-ca -keystore ${ADAGUC_SERVICES_SECURITY}/truststore.ts -storepass changeit -noprompt
keytool -import -v -trustcacerts -alias adaguc-services-ca -file ${ADAGUC_SERVICES_SECURITY}/adaguc-services-ca.cert -keystore ${ADAGUC_SERVICES_SECURITY}/truststore.ts -storepass changeit -noprompt
else
  echo "Using CA file ${ADAGUC_SERVICES_SECURITY}/adaguc-services-ca.cert"
fi

### Configure postgres ###

# Detect postgres user id
PGUSERNAME=userpostgres
if [ -z ${PGUSERID+x} ] || [ -z ${PGUSERID} ]; then 
  echo "PGUSERID is unset, trying to get id from directory"; 
  export PGUSERID=$(stat -c "%u" ${ADAGUCDB})
  echo "Got ${PGUSERID} from owner of dir ${ADAGUCDB}"
  if [ ${PGUSERID} == 0 ]; then 
    echo "PGUSERID has root id, setting to postgres"; 
    PGUSERNAME=postgres
    export PGUSERID=`id -u postgres`
  fi
else 
  echo "PGUSERID is set to '$PGUSERID'"; 
fi

if [ ${KEYSTOREUSERID} -ne ${PGUSERID} ]; then
  # Create postgres user
  echo "Using PGUSERID : ${PGUSERID}"
  useradd --shell /bin/bash -u ${PGUSERID} -o -c "" -m $PGUSERNAME
  export HOME=/home/$PGUSERNAME
else 
  # Same user as security folder
  PGUSERNAME=useradaguc
fi

# Set postgres permissions
chmod 777 /var/run/postgresql/
runuser -l $PGUSERNAME -c "touch /var/log/adaguc/postgresql.log"
runuser -l $PGUSERNAME -c "chmod 777 /var/log/adaguc/postgresql.log"
touch /var/log/adaguc/postgresql.log
chmod 777 /var/log/adaguc/postgresql.log
chown $PGUSERNAME ${ADAGUCDB}
runuser -l $PGUSERNAME -c "chmod 700 ${ADAGUCDB}"

# Check if a db already exists for given path
dbexists=`runuser -l $PGUSERNAME -c "(ls ${ADAGUCDB}/postgresql.conf >> /dev/null 2>&1 && echo yes) || echo no"`
if [ ${dbexists} == "no" ]
then
  echo "Initializing new postgresql database"
  #mkdir -p ${ADAGUCDB} && chmod 777 ${ADAGUCDB} && chown postgres: ${ADAGUCDB} && #TODO NOT NEEDED ANYMORE?
  runuser -l $PGUSERNAME -c "pg_ctl initdb -U adaguc -w -D ${ADAGUCDB}" && \
  runuser -l $PGUSERNAME -c "pg_ctl -w -U adaguc -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start" && \
  echo "Configuring new postgresql database" && \
  runuser -l $PGUSERNAME -c "createuser --superuser adaguc" && \
  runuser -l $PGUSERNAME -c "psql -U adaguc postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\"" && \
  runuser -l $PGUSERNAME -c "psql -U adaguc postgres -c \"CREATE DATABASE adaguc;\""
  
  if [ $? -ne 0 ]
  then
  exit 1
  fi
else 
  echo "Re-using persistent postgresql database from ${ADAGUCDB}" && \
  runuser -l $PGUSERNAME -c "pg_ctl -w -U adaguc -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start"
  if [ $? -ne 0 ]
  then
  exit 1
  fi
fi

echo "Checking POSTGRESQL DB" &&  runuser -l $PGUSERNAME -c "psql -U adaguc postgres -c \"show data_directory;\""
if [ $? -ne 0 ]
then
  echo "Unable to connect to postgres database"
  exit 1
fi  

### Update baselayers and check if this succeeds ###
export ADAGUC_PATH=/adaguc/adaguc-server-master/ && \
export ADAGUC_TMP=/tmp && \
/adaguc/adaguc-server-master/bin/adagucserver --updatedb \
  --config /adaguc/adaguc-server-config.xml,baselayers.xml

if [ $? -ne 0 ]
then
  echo "Unable to update baselayers with adaguc-server --updatedb"
  exit 1
fi  

echo "Start serving on ${EXTERNALADDRESS}"
java -jar /adaguc/adaguc-services.jar
    
