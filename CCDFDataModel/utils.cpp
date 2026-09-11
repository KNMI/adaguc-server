#include "utils.h"

CDFReader *findReaderByFileName(std::string fileName) {
  if (CT::endsWith(fileName, ".nc")) {
    return new CDFNetCDFReader();
  }
  if (CT::endsWith(fileName, ".h5")) {
    return new CDFHDF5Reader();
  }
  if (CT::endsWith(fileName, ".geojson")) {
    return new CDFGeoJSONReader();
  }
  if (CT::endsWith(fileName, ".csv")) {
    return new CDFCSVReader();
  }
  if (CT::endsWith(fileName, ".png")) {
    return new CDFPNGReader();
  }
  return NULL;
}

std::string getStandardName(const CDF::Variable &variable) {
  const auto standardNameAttr = variable.getAttr("standard_name");
  return standardNameAttr != nullptr ? standardNameAttr->toString() : variable.name;
}