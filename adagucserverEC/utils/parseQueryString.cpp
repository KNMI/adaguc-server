#include <cstdlib>
#include <CTString.h>
#include <cstddef>
#include <CSLD.h>
#include <CServerError.h>
#include <CAutoResource.h>

int getDatasetAndSldFromQueryString(CServerParams &srvParam) {
  const char *pszQueryString = getenv("QUERY_STRING");
  if (pszQueryString != nullptr) {
    CT::string queryString(pszQueryString);
    queryString.decodeURLSelf();
    auto parameters = queryString.split("&");
    for (size_t j = 0; j < parameters.size(); j++) {
      CT::string value0Cap;
      CT::string values[2];
      int equalPos = parameters[j].indexOf("="); // split("=");
      if (equalPos != -1) {
        values[0] = parameters[j].substring(0, equalPos);
        values[1] = parameters[j].c_str() + equalPos + 1;
      } else {
        values[0] = parameters[j].c_str();
        values[1] = "";
      }
      value0Cap.copy(&values[0]);
      value0Cap.toUpperCaseSelf();
      if (value0Cap.equals("DATASET")) {
        if (srvParam.datasetLocation.empty()) {

          srvParam.datasetLocation.copy(values[1].c_str());
          int status = CAutoResource::configureDataset(&srvParam, false);
          if (status != 0) {
            CDBError("CAutoResource::configureDataset failed");
            return status;
          }
        }
      }

      // Check if parameter name is a SLD parameter AND have file name
      CSLD csld;
      if (csld.parameterIsSld(values[0])) {
#ifdef CREQUEST_DEBUG
        CDBDebug("Found SLD parameter in query");
#endif

        // Set server params
        csld.setServerParams(&srvParam);

        // Process the SLD URL
        if (values[1].empty()) {
          setStatusCode(HTTP_STATUSCODE_404_NOT_FOUND);
          return 1;
        }
        int status = csld.processSLDUrl(values[1]);

        if (status != 0) {
          CDBError("Processing SLD failed");
          return status;
        }
      }
    }
  }
  return 0;
}