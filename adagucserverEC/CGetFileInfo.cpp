#include "CGetFileInfo.h"
#include "CDFObjectStore.h"

std::string CGetFileInfo::getLayersForFile(const char *filename) {
  CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObject(NULL, filename);

  std::string fileInfo = "";

  try {
    if (cdfObject == NULL) {
      CDBError("Unable to open file %s", filename);
      throw(__LINE__);
    }
    std::vector<std::string> variableList = CDFObjectStore::getListOfVisualizableVariables(cdfObject);

    fileInfo += "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    fileInfo += "<Configuration>\n";
    fileInfo += "\n";
    fileInfo += "  <!--header-->\n";
    fileInfo += "\n";

    for (size_t j = 0; j < variableList.size(); j++) {
      // printf("%s\n",variableList[j].c_str());

      CDF::Variable *var = cdfObject->getVariableThrows(variableList[j].c_str());

      std::string name = variableList[j];

      std::string title = variableList[j];

      // std::string filePath = makeCleanPath(filename);
      // filePath.setSize(filePath.lastIndexOf("/")+1);

      try {
        title = var->getAttributeThrows("long_name")->toString();
      } catch (int e) {
      }

      std::string abstract = title;

      try {
        abstract = var->getAttributeThrows("abstract")->toString();
      } catch (int e) {
        try {
          abstract = var->getAttributeThrows("description")->toString();
        } catch (int e) {
        }
      }

      std::string standardName = "";
      try {
        standardName = var->getAttributeThrows("standard_name")->toString();
      } catch (int e) {
      }

      /*if(standardName.length()>0){

       if(standardName.equals(variableList[j].c_str())==false){
          name.printconcat("_%s",standardName.c_str());
        }
      }*/

      fileInfo += "  <Layer type=\"database\">\n";
      CT::printfconcat(fileInfo, "    <FilePath filter=\".*\\.nc$\">%s</FilePath>\n", "[DATASETPATH]");
      CT::printfconcat(fileInfo, "    <Name>%s</Name>\n", CT::encodeXml(name).c_str());
      CT::printfconcat(fileInfo, "    <Title>%s</Title>\n", CT::encodeXml(title).c_str());
      CT::printfconcat(fileInfo, "    <Variable>%s</Variable>\n", CT::encodeXml(variableList[j]).c_str());
      // CT::printfconcat(fileInfo, "    <MetadataURL>[METADATAURL]</MetadataURL>\n");
      CT::printfconcat(fileInfo, "    <Abstract>%s</Abstract>\n", CT::encodeXml(abstract).c_str());
      fileInfo += "  </Layer>\n";
      fileInfo += "\n";
    }

    fileInfo += "  <!--footer-->\n";
    fileInfo += "\n";
    fileInfo += "</Configuration>\n";
  } catch (int e) {

    fileInfo = "";
  }

  CDFObjectStore::getCDFObjectStore()->clear();

  return fileInfo;
}
