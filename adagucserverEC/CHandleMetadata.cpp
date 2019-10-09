
#include "CHandleMetadata.h"
#include "CAutoResource.h"
#include "CDFObjectStore.h"
const char *CHandleMetadata::className="CHandleMetadata";


int CHandleMetadata::process(CServerParams *srvParam) {
    CT::string metadata = "{}";
    bool hasFoundDataSetOrAutoResource = false;

    if(CAutoResource::configure(srvParam,true)==0){
        hasFoundDataSetOrAutoResource = true;
    }
     
    if (!hasFoundDataSetOrAutoResource){
      CDBError("No dataset or autoresource found");
      return 1;
    }

    CT::string fileName = srvParam->internalAutoResourceLocation;

    CDFObject *cdfObject =  CDFObjectStore::getCDFObjectStore()->getCDFObjectHeaderPlain(NULL, srvParam, fileName.c_str());

    CT::string data = CDF::dumpAsJSON(cdfObject);
    CDBDebug("dump %s", data.c_str());
    metadata = data;

    if (srvParam->JSONP.length()==0) {
        printf("%s%c%c\n","Content-Type: application/json ",13,10);
        printf("%s", metadata.c_str());
    } else {
        printf("%s%c%c\n","Content-Type: application/javascript ",13,10);
        printf("%s(%s)",srvParam->JSONP.c_str(), metadata.c_str());
    }
    return 0;
}