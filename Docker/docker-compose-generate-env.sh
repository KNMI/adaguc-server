#/bin/bash


ADAGUC_PORT=443
ADAGUC_DATA_DIR=/data/adaguc-data
ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
ADAGUC_DATASET_DIR=/data/adaguc-datasets

usage() { echo "Usage: $0 -p <port number> -e <external adress> -a <autowmsdir> -d <dataset dir> -f <datadir>" 1>&2; exit 1; }


while getopts ":e:p:h:a:d:f:" o; do
    case "${o}" in
        e)
            EXTERNALADDRESS=${OPTARG}
            ;;
        p)
            ADAGUC_PORT=${OPTARG}
            ;;
        a)
            ADAGUC_AUTOWMS_DIR=${OPTARG}
            ;;
        d)
            ADAGUC_DATASET_DIR=${OPTARG}
            ;;
        f)
            ADAGUC_DATA_DIR=${OPTARG}
            ;;
        h)
            usage
            ;;
        *)
            usage
            ;;
    esac
done

shift $((OPTIND-1))

if [ -z "${EXTERNALADDRESS}" ]; then
  EXTERNALADDRESS="https://${HOSTNAME}/"
  if [ "${ADAGUC_PORT}" != "443" ]; then
    EXTERNALADDRESS="https://${HOSTNAME}:${ADAGUC_PORT}/"
  fi
fi


rm .env
echo "ADAGUC_HOME=$HOME" >> .env
echo "ADAGUC_DATA_DIR=${ADAGUC_DATA_DIR}" >> .env
echo "ADAGUC_AUTOWMS_DIR=${ADAGUC_AUTOWMS_DIR=}" >> .env
echo "ADAGUC_DATASET_DIR=${ADAGUC_DATASET_DIR}" >> .env
echo "ADAGUC_PORT=${ADAGUC_PORT}" >> .env
echo "EXTERNALADDRESS=${EXTERNALADDRESS}" >> .env
echo "############### env file ###############"
cat .env
echo "############### env file ###############"
echo "ADAGUC will be available on ${EXTERNALADDRESS}"
