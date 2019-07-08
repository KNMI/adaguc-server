THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

sudo apt-get -y install openjdk-11-jdk maven

sudo mkdir -p /data/adaguc-autowms && sudo chown $USER: /data/adaguc-autowms
sudo mkdir -p /data/adaguc-data && sudo chown $USER: /data/adaguc-data
sudo mkdir -p /data/adaguc-datasets && sudo chown $USER: /data/adaguc-datasets

ADAGUC_PATH=${THISSCRIPTDIR}/../../


mkdir -p ${ADAGUC_PATH}/adaguc-services/config
mkdir -p ${ADAGUC_PATH}/adaguc-services/space


git clone http://github.com/KNMI/adaguc-services


mvn package -f ${ADAGUC_PATH}/adaguc-services

# Configure adaguc-services
printf "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<adaguc-services>\n\
  <external-home-url>http://{ENV.ADAGUC_HOSTNAME}:{ENV.ADAGUC_PORT}/</external-home-url>\n\
  <basedir>{ENV.ADAGUC_SERVICES_SPACE}</basedir>\n\
  <userworkspace>{ENV.ADAGUC_SERVICES_SPACE}</userworkspace>\n\
  <server>\n\
    <port>{ENV.ADAGUC_PORT}</port>\n\
    <contextpath>/</contextpath>
  </server>\n\
  <security>\n\
    <enablessl>false</enablessl>
  </security>\n\
  <adaguc-server>\n\
    <adagucexecutable>{ENV.ADAGUC_PATH}/bin/adagucserver</adagucexecutable>\n\
    <export>ADAGUC_PATH={ENV.ADAGUC_PATH}/</export>\n\
    <export>ADAGUC_DB={ENV.ADAGUC_DB}/</export>\n\
    <export>ADAGUC_CONFIG={ENV.ADAGUC_PATH}/data/config/adaguc.vm.xml</export>\n\
    <export>ADAGUC_FONT={ENV.ADAGUC_PATH}/data/fonts/FreeSans.ttf</export>\n    
  </adaguc-server>\n\
  <autowms>\n\
    <enabled>true</enabled>\n\
    <autowmspath>/data/adaguc-autowms/</autowmspath>\n\
    <datasetpath>/data/adaguc-datasets/</datasetpath>\n\
  </autowms>\n\
</adaguc-services>" > ${ADAGUC_PATH}/adaguc-services/config/adaguc-services-config.xml
