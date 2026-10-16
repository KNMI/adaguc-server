#include <cstdlib>
#include <CTString.h>
#include <cstddef>
#include <CServerError.h>
#include <CAutoResource.h>

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

      // The SLD parameter used to fetch a styling document from an arbitrary,
      // client-supplied URL. This has been removed: it let a client make the
      // server issue HTTP requests to any host it chose (SSRF). Reject it
      // explicitly rather than silently ignoring it.
      if (value0Cap == "SLD") {
        setExceptionType(ServiceExceptionType::UnprocessableEntity);
        return 1;
      }
    }
  }
  return 0;
}