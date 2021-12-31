#include "utils.h"

CDFReader *findReaderByFileName(CT::string fileName) {
  if (fileName.endsWith(".nc")) {
    return new CDFNetCDFReader();
  }
  if (fileName.endsWith(".h5")) {
    return new CDFHDF5Reader();
  }
  if (fileName.endsWith(".geojson")) {
    return new CDFGeoJSONReader();
  }
  if (fileName.endsWith(".csv")) {
    return new CDFCSVReader();
  }
  if (fileName.endsWith(".png")) {
    return new CDFPNGReader();
  }
  return NULL;
}