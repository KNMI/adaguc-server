# TODO display in adaguc-services startup where adaguc is hosted

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
ADAGUC_PATH=${THISSCRIPTDIR}/../../

export ADAGUC_DB='dbname=adaguc user=adaguc password=adaguc host=localhost'
export ADAGUC_PORT=8080
export ADAGUC_HOSTNAME=`hostname`
export ADAGUC_SERVICES_SPACE=${ADAGUC_PATH}/adaguc-services/space
export ADAGUC_SERVICES_CONFIG=${ADAGUC_PATH}/adaguc-services/config/adaguc-services-config.xml

# Copy a test dataset
cp ${ADAGUC_PATH}/data/datasets/testdata.nc /data/adaguc-autowms/

nohup java -jar ${ADAGUC_PATH}/adaguc-services/target/adaguc-services-*.jar > adaguc-services.log 2> adaguc-services.log < /dev/null &
 
