# TODO display in adaguc-services startup where adaguc is hosted

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
export ADAGUC_PATH=`realpath -sm ${THISSCRIPTDIR}/../../`

echo "Using ADAGUC_PATH "${ADAGUC_PATH}
export ADAGUC_DB='dbname=adaguc user=adaguc password=adaguc host=localhost'
export ADAGUC_PORT=8080
export ADAGUC_HOSTNAME=`hostname`
export ADAGUC_SERVICES_SPACE=${ADAGUC_PATH}/adaguc-services/space
export ADAGUC_SERVICES_CONFIG=${ADAGUC_PATH}/adaguc-services/config/adaguc-services-config.xml

# Copy a test dataset
cp ${ADAGUC_PATH}/data/datasets/testdata.nc /data/adaguc-autowms/
rm ${ADAGUC_PATH}/adaguc-services.log
nohup java -jar ${ADAGUC_PATH}/adaguc-services/target/adaguc-services-*.jar > ${ADAGUC_PATH}/adaguc-services.log 2> ${ADAGUC_PATH}/adaguc-services.log < /dev/null &
sleep 3
cat ${ADAGUC_PATH}/adaguc-services.log
echo "---"
echo "Done" 
