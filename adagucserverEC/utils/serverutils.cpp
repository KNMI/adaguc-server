
#include "serverutils.h"

bool checkIfFileMatchesLayer(CT::string layerPathToScan, CServerConfig::XMLE_Layer *layer) {
  // Get the directory of the file to scan:
  CT::string directoryOfFileToScan = layerPathToScan = CDirReader::makeCleanPath(layerPathToScan);
  directoryOfFileToScan.substringSelf(0, directoryOfFileToScan.length() - directoryOfFileToScan.basename().length());
  directoryOfFileToScan = CDirReader::makeCleanPath(directoryOfFileToScan) + "/";

  if (layer->attr.type.empty() || layer->attr.type.equals("database")) {
    if (layer->FilePath.size() > 0) {
      CT::string filePath = CDirReader::makeCleanPath(layer->FilePath[0]->value);
      // Directories need to end with a /
      CT::string filePathWithTrailingSlash = filePath + "/";
      CT::string filter = layer->FilePath[0]->attr.filter;
      // When the FilePath in the Layer configuration is exactly the same as the file to scan, give a Match
      if (layerPathToScan.equals(filePath)) {
        return true;
        // When the directory of the file to scan matches the FilePath and the filter matches, give a Match
      } else if (directoryOfFileToScan.startsWith(filePathWithTrailingSlash)) {
        if (CDirReader::testRegEx(layerPathToScan.basename(), filter.c_str()) == 1) {
          return true;
        }
      }
    }
  }
  return false;
}
