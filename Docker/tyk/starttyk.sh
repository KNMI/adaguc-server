#!/bin/bash

# Replace in app/*json files the environment variables starting with ADAGUCTYK_
environments=`env | grep ADAGUCTYK_`
filestochange=`ls /opt/tyk-gateway/apps/*.json`
for filetochange in $filestochange;do
    for environment in $environments;do 
        echo $environment
        arrIN=(${environment//=/ })
        
        KEY=${arrIN[0]}
        VALUE=${!KEY}
        # echo "s#${KEY}#${VALUE}#g"
        # /usr/bin/sed -i "s|${arrIN[0]}|${arrIN[1]}|g" $filetochange
        /usr/bin/sed -i "s#${KEY}#${VALUE}#g" $filetochange
    done
    cat $filetochange
done

# Start tyk.
/opt/tyk-gateway/tyk