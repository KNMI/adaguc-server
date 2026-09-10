#include <cstdlib>
#include <CTString.h>
#include <cstddef>
#include <CSLD.h>
#include <CServerError.h>
#include <CAutoResource.h>

static const bool CREQUEST_DEBUG = false;

int getDatasetAndSldFromQueryString(CServerParams &srvParam) {
  const char *pszQueryString = getenv("QUERY_STRING");
  if (pszQueryString != nullptr) {
    std::string queryString(pszQueryString);
    queryString = CT::decodeURL(queryString);
    auto parameters = CT::split(queryString, "&");
    for (size_t j = 0; j < parameters.size(); j++) {
      std::string value0Cap;
      std::string values[2];
      int equalPos = CT::indexOf(parameters[j], "="); // split("=");
      if (equalPos != -1) {
        values[0] = CT::substring(parameters[j], 0, equalPos);
        values[1] = parameters[j].c_str() + equalPos + 1;
      } else {
        values[0] = parameters[j].c_str();
        values[1] = "";
      }
      value0Cap = CT::toUpperCase(values[0]);
      if (value0Cap == "DATASET") {
        if (srvParam.datasetLocation.empty()) {

          srvParam.datasetLocation = (values[1].c_str());
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
        if (CREQUEST_DEBUG) {
          CDBDebug("Found SLD parameter in query");
        }

        // Set server params
        csld.setServerParams(&srvParam);

        // Process the SLD URL
        if (values[1].empty()) {
          setExceptionType(ServiceExceptionType::UnprocessableEntity);
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