#include <cstdlib>
#include <CTString.h>
#include <cstddef>
#include <CServerError.h>
#include <CAutoResource.h>

// Returns the value of the first parameter with this name (case insensitive) in the query string, or an empty string when there is none
static std::string getQueryStringParameter(const std::string &queryString, const std::string &name) {
  // Split before decoding, same as CRequest::process_querystring
  for (const auto &encodedParameter: CT::split(queryString, "&")) {
    std::string parameter = CT::decodeURL(encodedParameter);
    size_t equalPos = parameter.find('=');
    if (equalPos != std::string::npos && CT::equalsIgnoreCase(parameter.substr(0, equalPos), name)) return parameter.substr(equalPos + 1);
  }
  return "";
}

// Configures the dataset of the DATASET parameter in the query string, unless a dataset is already set
int getDatasetFromQueryString(CServerParams &srvParam) {
  const char *queryString = getenv("QUERY_STRING");
  if (queryString == nullptr || !srvParam.datasetLocation.empty()) return 0;

  srvParam.datasetLocation = getQueryStringParameter(queryString, "DATASET");
  if (srvParam.datasetLocation.empty()) return 0;

  int status = CAutoResource::configureDataset(&srvParam, false);
  if (status != 0) {
    CDBError("CAutoResource::configureDataset failed");
  }
  return status;
}
