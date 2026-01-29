#include <stdio.h>
#include "CTString.h"
#include "../adagucserverEC/CInspire.h"

int main() {
  DEF_ERRORMAIN();
  const char *cswService = "http://www.nationaalgeoregister.nl/geonetwork/srv/eng/csw?Service=CSW&Request=GetRecordById&Version=2.0.2&id=1bd3867c-3010-4cab-80a8-3b9ebef1dea1&outputSchema=http://"
                           "www.isotc211.org/2005/gmd&elementSetName=full";

  try {
    CInspire::InspireMetadataFromCSW inspireMetadata = CInspire::readInspireMetadataFromCSW(cswService);
    printf("%s\n", inspireMetadata.toString().c_str());
  } catch (int a) {
    CDBError("Unable to read from catalog service: %s", CInspire::getErrorMessage(a).c_str());
  }

  return 0;
}