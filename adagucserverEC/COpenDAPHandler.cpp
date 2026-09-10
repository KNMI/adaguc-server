#include "COpenDAPHandler.h"
#include "CRequest.h"
#include "CDBFactory.h"
#include "CAutoResource.h"
#include "utils/LayerUtils.h"

static const bool COPENDAPHANDLER_DEBUG = false;

class CDFTypeToOpenDAPType {
public:
  static std::string getvar(const int type) {
    std::string rtype = "unknown";
    if (type == CDF_NONE) rtype = "CDF_NONE";
    if (type == CDF_BYTE) rtype = "Byte";
    if (type == CDF_UBYTE) rtype = "Byte";
    if (type == CDF_CHAR) rtype = "Byte";
    if (type == CDF_SHORT) rtype = "Int16";
    if (type == CDF_USHORT) rtype = "UInt16";
    if (type == CDF_INT) rtype = "Int32";
    if (type == CDF_UINT) rtype = "Uint32";
    if (type == CDF_FLOAT) rtype = "Float32";
    if (type == CDF_DOUBLE) rtype = "Float64";
    if (type == CDF_STRING) rtype = "String";
    return rtype;
  }
  static std::string getatt(const int type) {
    std::string rtype = "unknown";
    if (type == CDF_NONE) rtype = "CDF_NONE";
    if (type == CDF_BYTE) rtype = "Byte";
    if (type == CDF_UBYTE) rtype = "Byte";
    if (type == CDF_CHAR) rtype = "String";
    if (type == CDF_SHORT) rtype = "Int16";
    if (type == CDF_USHORT) rtype = "UInt16";
    if (type == CDF_INT) rtype = "Int32";
    if (type == CDF_UINT) rtype = "Uint32";
    if (type == CDF_FLOAT) rtype = "Float32";
    if (type == CDF_DOUBLE) rtype = "Float64";
    if (type == CDF_STRING) rtype = "String";
    return rtype;
  }
};

int COpenDAPHandler::getDimSize(CDataSource *dataSource, const char *name) {
  if (COPENDAPHANDLER_DEBUG) {
    CDBDebug("getDimSize There are %zu dims for %s", dataSource->cfgLayer->Dimension.size(), name);
  }
  // First check wether dims are configured in the DataBase
  for (size_t d = 0; d < dataSource->cfgLayer->Dimension.size(); d++) {
    if (COPENDAPHANDLER_DEBUG) {
      CDBDebug("getDimSize Checking : %s", dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
    }
    if (dataSource->cfgLayer->Dimension[d]->attr.name == name) {
      if (COPENDAPHANDLER_DEBUG) {
        CDBDebug("getDimSize found : %s", dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
      }
      std::string tableName;
      std::string dim = dataSource->cfgLayer->Dimension[d]->attr.name;

      try {
        tableName = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)
                        ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->elementValue, dataSource->cfgLayer->FilePath[0]->attr.filter, dim.c_str(), dataSource);
      } catch (int e) {
        CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->elementValue.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dim.c_str());
        return -1;
      }
      if (COPENDAPHANDLER_DEBUG) {
        CDBDebug("getDimSize tableName = %s", tableName.c_str());
      }
      size_t dimSize = 0;
      CDBStore::Store *store = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getUniqueValuesOrderedByValue(dim.c_str(), 0, true, tableName.c_str());
      if (store != NULL) {
        if (store->records.size() != 0) {
          if (COPENDAPHANDLER_DEBUG) {
            CDBDebug("getDimSize %zu", store->records.size());
          }
          dimSize = store->records.size();
        }
      }
      delete store;
      if (COPENDAPHANDLER_DEBUG) {
        CDBDebug("getDimSize DimSize from DB for dim %s = %zu", dim.c_str(), dimSize);
      }
      return dimSize;
    }
  }

  // Check wether we can find the dim in the netcdf file
  try {
    if (COPENDAPHANDLER_DEBUG) {
      CDBDebug("getDimSize Trying to lookup in cdfObject");
    }
    CDF::Dimension *v = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeaderPlain(dataSource, dataSource->srvParams, dataSource->getFileName().c_str())->getDimensionThrows(name);
    if (COPENDAPHANDLER_DEBUG) {
      CDBDebug("Length = %zu", v->length);
    }
    return v->length;
  } catch (int e) {
  }
  CDBError("getDimSize failed");
  return -1;
}

std::string COpenDAPHandler::VarInfoToString(std::vector<VarInfo> selectedVariables) {
  std::string r;
  for (size_t j = 0; j < selectedVariables.size(); j++) {
    CT::printfconcat(r, "Variable Name: %s\n", selectedVariables[j].name.c_str());
    for (size_t i = 0; i < selectedVariables[j].dimInfo.size(); i++) {
      CT::printfconcat(r, "  Dim name %s :[%zu %zu %td]\n", selectedVariables[j].dimInfo[i].name.c_str(), selectedVariables[j].dimInfo[i].start, selectedVariables[j].dimInfo[i].count,
                       selectedVariables[j].dimInfo[i].stride);
    }
  }
  return r;
}

std::string COpenDAPHandler::createDDSHeader(std::string layerName, CDFObject *cdfObject, std::vector<VarInfo> selectedVariables) {
  /* Print DODS and DDS header */
  std::string output = "";
  if (jsonWriter)
    output += "{\n  \"dataset\": {\n";
  else
    output += "Dataset {\n";

  for (size_t i = 0; i < selectedVariables.size(); i++) {
    for (size_t j = 0; j < cdfObject->variables.size(); j++) {
      CDF::Variable *v = cdfObject->variables[j];
      CDFType type = (CDFType)v->getType();

      if (selectedVariables[i].name == v->name) {
        if (jsonWriter) {
          if (j > 0) {
            output += ",\n";
          }
          CT::printfconcat(output, "    \"%s\": {\n", v->name.c_str());
          CT::printfconcat(output, "      \"type\": \"%s\",\n", CDFTypeToOpenDAPType::getvar(type).c_str());
          CT::printfconcat(output, "      \"dimensions\": [\n");
          for (size_t j = 0; j < v->dimensionlinks.size(); j++) {
            int size = -1;
            if (selectedVariables[i].dimInfo.size() == v->dimensionlinks.size()) {
              size = selectedVariables[i].dimInfo[j].count;
            }
            if (j > 0) output += ",\n";
            CT::printfconcat(output, "        {\"%s\": %d }", v->dimensionlinks[j]->name.c_str(), size);
          }
          output += "\n      ]\n";
          output += "    }";

        } else {
          CT::printfconcat(output, "    %s ", CDFTypeToOpenDAPType::getvar(type).c_str());
          output += v->name;
          for (size_t j = 0; j < v->dimensionlinks.size(); j++) {
            int size = -1;
            if (selectedVariables[i].dimInfo.size() == v->dimensionlinks.size()) {
              size = selectedVariables[i].dimInfo[j].count;
            }
            CT::printfconcat(output, "[%s = %d]", v->dimensionlinks[j]->name.c_str(), size);
          }
          if (v->dimensionlinks.size() == 0) {
            CT::printfconcat(output, "[%d]", 1);
          }
          output += ";\n";
        }
      }
    }
  }
  if (jsonWriter)
    CT::printfconcat(output, "\n  }");
  else
    CT::printfconcat(output, "} %s;\n", layerName.c_str());
  return output;
}

int bytesWritten = 0;
void COpenDAPHandler::writeInt(int &v) {
  if (jsonWriter) {
    if (!jsonValuesWritten) {
      jsonValuesWritten = true;
      fprintf(opendapoutstream, "%d", v);
    } else {
      fprintf(opendapoutstream, ", %d", v);
    }
    return;
  }
  unsigned char c1 = ((unsigned char)v);
  unsigned char c2 = ((unsigned char)(v >> 8));
  unsigned char c3 = ((unsigned char)(v >> 16));
  unsigned char c4 = ((unsigned char)(v >> 24));
  ;
  fwrite(&c4, 1, 1, opendapoutstream);
  fwrite(&c3, 1, 1, opendapoutstream);
  fwrite(&c2, 1, 1, opendapoutstream);
  fwrite(&c1, 1, 1, opendapoutstream);
  bytesWritten += 4;
}

void COpenDAPHandler::writeDouble(double &v) {
  if (jsonWriter) {
    if (!jsonValuesWritten) {
      jsonValuesWritten = true;
      fprintf(opendapoutstream, "%f", v);
    } else {
      fprintf(opendapoutstream, ", %f", v);
    }
    return;
  }
  unsigned char const *p = reinterpret_cast<unsigned char const *>(&v);
  fwrite(&p[7], 1, 1, opendapoutstream);
  fwrite(&p[6], 1, 1, opendapoutstream);
  fwrite(&p[5], 1, 1, opendapoutstream);
  fwrite(&p[4], 1, 1, opendapoutstream);
  fwrite(&p[3], 1, 1, opendapoutstream);
  fwrite(&p[2], 1, 1, opendapoutstream);
  fwrite(&p[1], 1, 1, opendapoutstream);
  fwrite(&p[0], 1, 1, opendapoutstream);

  bytesWritten += 8;
}

int COpenDAPHandler::putVariableDataSize(CDF::Variable *v) {

  if (v->getType() != CDF_STRING) {
    int a = v->getSize();
    writeInt(a);
    writeInt(a);
  } else {

    int a = v->getSize();

    writeInt(a);
  }

  return 0;
}

int COpenDAPHandler::putVariableData(CDF::Variable *v, CDFType type) {
  int written = 0;
  size_t typeSize = CDF::getTypeSize(type);

  size_t varSize = v->getSize();

  if (jsonWriter) {
    {
      switch (type) {
      case CDF_BYTE:
        for (size_t d = 0; d < varSize; d++) {
          int a = (int)((char *)v->data)[d];
          writeInt(a);
        }
        break;
      case CDF_UBYTE:
        for (size_t d = 0; d < varSize; d++) {
          int a = (unsigned int)((unsigned char *)v->data)[d];
          writeInt(a);
        }
        break;
      case CDF_SHORT:
        for (size_t d = 0; d < varSize; d++) {
          int a = (int)((short *)v->data)[d];
          writeInt(a);
        }
        break;
      case CDF_USHORT:
        for (size_t d = 0; d < varSize; d++) {
          int a = (unsigned int)((unsigned short *)v->data)[d];
          writeInt(a);
        }
        break;
      case CDF_INT:
        for (size_t d = 0; d < varSize; d++) {
          int a = (int)((int *)v->data)[d];
          writeInt(a);
        }
        break;
      case CDF_UINT:
        for (size_t d = 0; d < varSize; d++) {
          int a = (unsigned int)((unsigned int *)v->data)[d];
          writeInt(a);
        }
        break;
      case CDF_FLOAT:
        for (size_t d = 0; d < varSize; d++) {
          double a = (double)((float *)v->data)[d];
          writeDouble(a);
        }
        break;
      case CDF_DOUBLE:
        for (size_t d = 0; d < varSize; d++) {
          double a = (double)((double *)v->data)[d];
          writeDouble(a);
        }
        break;
      case CDF_CHAR: {
        if (v->dimensionlinks.size() == 2) {
          /* Support strings, often they have two dimensionions indicating the number of strings and the string length */
          for (size_t d = 0; d < v->dimensionlinks[0]->getSize(); d++) {
            size_t stringLength = v->dimensionlinks[1]->getSize();
            if (d > 0) fprintf(opendapoutstream, ", ");
            fprintf(opendapoutstream, "\"%s\"", (std::string((const char *)v->data + d * stringLength, stringLength)).c_str());
          }
        } else {
          for (size_t d = 0; d < varSize; d++) {
            int a = (unsigned int)((char *)v->data)[d];
            writeInt(a);
          }
        }
      } break;
      case CDF_STRING:
        if (type == CDF_STRING)
          for (size_t d = 0; d < varSize; d++) {
            const char **data = (const char **)v->data;
            for (size_t d = 0; d < varSize; d++) {
              if (d > 0) fprintf(opendapoutstream, ", ");
              int l = int(strlen(data[d]));
              if (l < 0) {
                CDBError("String too large");
                return 1;
              }
              for (int e = 0; e < l; e++) {
                putc(data[d][e], opendapoutstream);
              }
            }
          }
        break;
      default:
        fprintf(opendapoutstream, " ? ");
        break;
      }
    }
    return 0;
  }
  if (type == CDF_BYTE || type == CDF_UBYTE || type == CDF_CHAR || type == CDF_INT || type == CDF_UINT || type == CDF_FLOAT || type == CDF_DOUBLE) {
    unsigned char *data = (unsigned char *)v->data;
    for (size_t d = 0; d < varSize; d++) {
      for (size_t e = 0; e < typeSize; e++) {
        putc(data[d * typeSize + (typeSize - 1) - e], opendapoutstream);
      }
      bytesWritten += typeSize;
      written += typeSize;
    }
  }

  if (type == CDF_SHORT || type == CDF_USHORT) {
    unsigned char *data = (unsigned char *)v->data;

    for (size_t d = 0; d < varSize; d++) {
      putc(0, opendapoutstream);
      putc(0, opendapoutstream);
      bytesWritten += 2;
      written += 2;
      for (size_t e = 0; e < typeSize; e++) {
        putc(data[d * typeSize + (typeSize - 1) - e], opendapoutstream);
      }
      bytesWritten += typeSize;
      written += typeSize;
    }
  }

  // Strings need to be '0' terminated.
  if (type == CDF_STRING) {
    const char **data = (const char **)v->data;
    for (size_t d = 0; d < varSize; d++) {
      int l = int(strlen(data[d]));
      if (l < 0) {
        CDBError("String too large");
        return 1;
      }
      writeInt(l);
      for (int e = 0; e < l; e++) {
        putc(data[d][e], opendapoutstream);
        written++;
        bytesWritten++;
      }

      // Padding bytes to sequences of four.
      while (int(written / 4) * 4 != written) {
        putc(0, opendapoutstream);
        written++;
        bytesWritten++;
      }
    }
  }

  // Bytes need to be padded to words of four bytes
  if ((type == CDF_BYTE || type == CDF_CHAR || type == CDF_UBYTE)) {
    // Padding bytes to sequences of four.
    while (int(written / 4) * 4 != written) {
      CDBDebug("Padding");
      putc(48, opendapoutstream);
      written++;
      bytesWritten++;
    }
  }
  return 0;
}
int COpenDAPHandler::handleOpenDAPRequest(const char *path, const char *_query, CServerParams *srvParam) {

  if (COPENDAPHANDLER_DEBUG) {
    CDBDebug("\n*****************************************************************************************");
  }

  jsonWriter = false;

  httpHeaderContentType = CT::fromCharPointer(getenv("CONTENT_TYPE"));
  if (httpHeaderContentType == "application/json") {
    jsonWriter = true;
  }

  CDBDebug("CONTENT_TYPE %s", httpHeaderContentType.c_str());

  std::string query;
  if (_query != NULL) {
    query = _query;
    if (query == "null") {
      query = "";
    }
  }
  CDBDebug("OpenDAP Received [%s] [%s]", path, query.c_str());
  std::string defaultPath = "opendap";
  if (srvParam->cfg->OpenDAP[0]->attr.path.empty() == false) {
    defaultPath = srvParam->cfg->OpenDAP[0]->attr.path;
  }

  std::string dapName = path + defaultPath.length() + 1;
  std::string layerName = "";
  std::string pathQuery = "";
  bool isDDSRequest = false;
  bool isDASRequest = false;
  bool isDODRequest = false;
  dapName = CT::decodeURL(dapName);

  int i = CT::lastIndexOf(dapName, ".dds");
  if (i != -1) {
    layerName = CT::substring(dapName, 0, i);
    pathQuery = CT::substring(dapName, i + 4, -1);
    isDDSRequest = true;
  } else {
    int i = CT::lastIndexOf(dapName, ".das");
    if (i != -1) {
      layerName = CT::substring(dapName, 0, i);
      pathQuery = CT::substring(dapName, i + 4, -1);
      isDASRequest = true;
    } else {
      int i = CT::lastIndexOf(dapName, ".dods");
      if (i != -1) {
        layerName = CT::substring(dapName, 0, i);
        pathQuery = CT::substring(dapName, i + 5, -1);
        isDODRequest = true;
      } else {
        int i = CT::lastIndexOf(dapName, ".dds");
        if (i != -1) {
          layerName = CT::substring(dapName, 0, i);
          pathQuery = CT::substring(dapName, i + 5, -1);
          isDDSRequest = true;
        }
      }
    }
  }

  if (isDDSRequest == false && isDASRequest == false && isDODRequest == false) {
    CDBError("Not a valid OpenDAP request received, e.g. use .dds, .das");
    return 1;
  }

  opendapoutstream = stdout;

  if (isDODRequest) {
    if (jsonWriter) {
      printf("%s%c%c\n", "Content-Type: application/json", 13, 10);
    } else {
      printf("%s%c%c", "XDAP: 2.0 ", 13, 10);
      printf("%s%c%c\n", "Content-Type: application/octet-stream", 13, 10);
    }
  } else {

    if (jsonWriter) {
      printf("%s%c%c\n", "Content-Type: application/json", 13, 10);
    } else {
      printf("%s%c%c", "XDAP: 2.0 ", 13, 10);
      if (isDDSRequest) {
        printf("%s%c%c", "Content-Description: dods-dds ", 13, 10);
      }
      printf("%s%c%c\n", "Content-Type: text/plain; charset=utf-8", 13, 10);
    }
  }

  if (COPENDAPHANDLER_DEBUG) {
    CDBDebug("layerName: %s", layerName.c_str());
  }
  // Check if a dataset/dataURL was given
  std::string dataURL = "";
  if (CT::endsWith(layerName, ".nc") || CT::endsWith(layerName, ".geojson") || CT::endsWith(layerName, ".hdf5") || CT::endsWith(layerName, ".h5")) {
    /* If the layerName ends with .nc extension, it is likely not a Layer but a filename */
    dataURL = layerName;
    layerName = "";
  } else {
    int lastSlash = CT::lastIndexOf(layerName, "/");
    if (lastSlash != -1) {
      dataURL = CT::substring(layerName, 0, lastSlash);
      layerName = CT::substring(layerName, lastSlash + 1, -1);
    }
  }
  if (COPENDAPHANDLER_DEBUG) {
    CDBDebug("dataURL: %s", dataURL.c_str());
    CDBDebug("layerName: %s", layerName.c_str());
  }

  if (dataURL.length() > 0) {
    bool hasFoundDataSetOrAutoResource = false;

    // Check if DATASET is enabled
    if (hasFoundDataSetOrAutoResource == false) {
      for (size_t j = 0; j < srvParam->cfg->Dataset.size(); j++) {
        if (srvParam->cfg->Dataset[j]->attr.enabled == "true" && srvParam->cfg->Dataset[j]->attr.location.empty() == false) {
          srvParam->datasetLocation = dataURL;
          CDBDebug("Checking %s", srvParam->cfg->Dataset[j]->attr.location.c_str());
          int status = CAutoResource::configureDataset(srvParam, true);
          if (status == 0) {
            CDBDebug("Found dataset %s", srvParam->datasetLocation.c_str());
            hasFoundDataSetOrAutoResource = true;
            break;
          }
        }
      }
    }

    // Check if AUTORESOURCE is enabled
    if (hasFoundDataSetOrAutoResource == false) {
      srvParam->datasetLocation = "";
      resetErrors();
      if (srvParam->isAutoResourceEnabled()) {
        srvParam->autoResourceLocation = dataURL;
        if (CAutoResource::configure(srvParam, true) == 0) {
          hasFoundDataSetOrAutoResource = true;
        }
      }
    }

    if (!hasFoundDataSetOrAutoResource) {
      CDBError("No dataset or autoresource found");
      return 1;
    }
  }

  if (srvParam->cfg->Layer.size() == 0) {
    CDBError("No layers found for this configuration");
    return 1;
  }

  if (COPENDAPHANDLER_DEBUG) {
    CDBDebug("Layername = %s", layerName.c_str());
    CDBDebug("pathQuery = %s", pathQuery.c_str());
    CDBDebug("autoResourceVariable = %s", srvParam->autoResourceVariable.c_str());
    CDBDebug("Num layers: %zu ", srvParam->cfg->Layer.size());
  }
  CDataSource *dataSource = new CDataSource();
  bool foundLayer = false;

  for (size_t layerNo = 0; layerNo < srvParam->cfg->Layer.size(); layerNo++) {
    if (srvParam->cfg->Layer[layerNo]->attr.type == "database") {
      std::string intLayerName = makeUniqueLayerName(srvParam->cfg->Layer[layerNo]);

      if (layerName.length() == 0) {
        layerName = intLayerName;
      }
      CT::replaceSelf(intLayerName, "/", "_");

      if (intLayerName == layerName) {
        if (dataSource->setCFGLayer(srvParam, srvParam->cfg->Layer[layerNo], 0) != 0) {
          CDBError("Error setCFGLayer");
          delete dataSource;
          return 1;
        }
        foundLayer = true;
        break;
      }
    }
  }

  if (foundLayer == false) {
    CDBError("Unable to find layer %s", layerName.c_str());
    delete dataSource;
    return 1;
  }

  if (COPENDAPHANDLER_DEBUG) {
    CDBDebug("Found layer %s", layerName.c_str());
  }
  if (dataSource->dLayerType == CConfigReaderLayerTypeDataBase) {
    // When this layer has no dimensions, we do not need to query
    //  When there are no dims, we can get the filename from the config
    if (dataSource->cfgLayer->Dimension.size() == 0) {

      if (CAutoConfigure::autoConfigureDimensions(dataSource) != 0) {
        CDBError("Unable to configure dimensions automatically");
        delete dataSource;
        return 1;
      }
    }
    ///

    std::vector<std::string> fileList;
    try {
      fileList = CDBFileScanner::searchFileNames(dataSource->cfgLayer->FilePath[0]->elementValue.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter, NULL);
    } catch (int linenr) {
      CDBError("Could not find any filename");
      delete dataSource;
      return 1;
    }

    if (dataSource->getFileName().empty()) {
      if (fileList.size() == 0) {
        CDBError("fileList.size()==0");
        delete dataSource;
        return 1;
      }
      dataSource->addStep(fileList[0]);
      dataSource->getCDFDims()->push_back({.name = "time", .value = "0", .index = 0});
    }
  }

  if (COPENDAPHANDLER_DEBUG) {
    CDBDebug("This layer has %zu dims.", dataSource->cfgLayer->Dimension.size());
    for (size_t d = 0; d < dataSource->cfgLayer->Dimension.size(); d++) {
      CDBDebug("%s %s", dataSource->cfgLayer->Dimension[d]->attr.name.c_str(), dataSource->cfgLayer->Dimension[d]->elementValue.c_str());
    }
  }

  // Read the NetCDF header!

  try {

    CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeaderPlain(dataSource, dataSource->srvParams, dataSource->getFileName().c_str());
    ; // dataSource->getDataObject(0)->cdfObject;

    for (size_t d = 0; d < dataSource->cfgLayer->Dimension.size(); d++) {
      // Check for the configured dimensions or scalar variables
      // 1 )Is this a scalar?
      CDF::Variable *dimVar = cdfObject->getVariableNE(dataSource->cfgLayer->Dimension[d]->attr.name);
      CDF::Dimension *dimDim = cdfObject->getDimensionNE(dataSource->cfgLayer->Dimension[d]->attr.name);

      if (dimVar != NULL && dimDim == NULL) {
        // Check for scalar variable
        if (dimVar->dimensionlinks.size() == 0) {
          if (COPENDAPHANDLER_DEBUG) {
            CDBDebug("Found scalar variable %s with no dimension. Creating dim", dimVar->name.c_str());
          }
          dimDim = new CDF::Dimension();
          dimDim->name = dimVar->name;
          dimDim->setSize(1);
          cdfObject->addDimension(dimDim);
          dimVar->dimensionlinks.push_back(dimDim);
        }
      }
    }
    if (COPENDAPHANDLER_DEBUG) {
      CDBDebug("dataSource->cfgLayer->Dimension.size() %zu", dataSource->cfgLayer->Dimension.size());
    }
    for (size_t d = 0; d < dataSource->cfgLayer->Dimension.size(); d++) {
      COGCDims ogcDim;
      ogcDim.name = dataSource->cfgLayer->Dimension[d]->attr.name;
      ogcDim.value = ogcDim.name;
      ogcDim.netCDFDimName = dataSource->cfgLayer->Dimension[d]->attr.name;
      dataSource->requiredDims.push_back(ogcDim);
      if (COPENDAPHANDLER_DEBUG) {
        CDBDebug("Push %s", dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
      }
    }

    if (isDDSRequest || isDODRequest) {

      std::vector<VarInfo> selectedVariables;
      // Parsing dim queries per variable (e.g. precip[0][0:3] == x,y)
      if (!query.empty()) {
        std::vector<std::string> items = CT::split(query, ",");
        for (size_t j = 0; j < items.size(); j++) {
          if (COPENDAPHANDLER_DEBUG) {
            CDBDebug("Selected variable = \"%s\"", items[j].c_str());
          }

          // Split on every [ token, gives sequences precip, 0] and 0:3]
          std::vector<std::string> varsettings = CT::split(items[j], "[");
          varsettings[0] = CT::decodeURL(varsettings[0]);

          // Push the variable
          if (COPENDAPHANDLER_DEBUG) {
            CDBDebug("Push varinfo %s", varsettings[0].c_str());
          }

          selectedVariables.push_back(VarInfo(varsettings[0].c_str()));
          // Retrieve other settings, like start,count,stride
          if (varsettings.size() > 1) {
            // Fill in start/count/stride from request
            if (COPENDAPHANDLER_DEBUG) {
              CDBDebug("Getting start/count/stride from request");
            }
            for (size_t d = 1; d < varsettings.size(); d++) {
              CT::replaceSelf(varsettings[d], "]", ""); // gives sequences precip, 0 and 0:3

              // Now split on :
              std::vector<std::string> startCountStrideItems = CT::split(varsettings[d], ":");
              size_t start = 0;
              size_t count = 1;
              size_t stride = 1;
              if (startCountStrideItems.size() == 1) {
                start = atoi(startCountStrideItems[0].c_str());
              }
              if (startCountStrideItems.size() == 2) {
                start = atoi(startCountStrideItems[0].c_str());
                count = atoi(startCountStrideItems[1].c_str()) - start;
                count++;
                if (count < 1) count = 1;
              }
              if (startCountStrideItems.size() == 3) { // TODO CHECK if [start:count:stride] is correct.
                start = atoi(startCountStrideItems[0].c_str());
                count = atoi(startCountStrideItems[1].c_str()) - start;
                count++;
                if (count < 1) count = 1;
                stride = atoi(startCountStrideItems[2].c_str());
              }
              if (COPENDAPHANDLER_DEBUG) {
                CDBDebug("DIMINFO: %zu,%s  %zu:%zu", start, varsettings[d].c_str(), d, j);
              }
              std::string dimname = cdfObject->getVariableThrows(selectedVariables.back().name)->dimensionlinks[d - 1]->name;
              if (COPENDAPHANDLER_DEBUG) {
                CDBDebug("Push dimInfo %s", dimname.c_str());
              }
              selectedVariables.back().dimInfo.push_back(VarInfo::Dim(dimname.c_str(), start, count, stride));
            }
          }
        }
      }

      // If no variables where selected, select them all.
      if (selectedVariables.size() == 0) {
        if (COPENDAPHANDLER_DEBUG) {
          CDBDebug("Selecting all variables");
        }
        for (size_t j = 0; j < cdfObject->variables.size(); j++) {
          if (COPENDAPHANDLER_DEBUG) {
            CDBDebug("Push varinfo %s", cdfObject->variables[j]->name.c_str());
          }
          selectedVariables.push_back(VarInfo(cdfObject->variables[j]->name.c_str()));
          //}
        }
      }

      if (COPENDAPHANDLER_DEBUG) {
        CDBDebug("Getting start/count/stride from database");
      }
      for (size_t i = 0; i < selectedVariables.size(); i++) {
        if (selectedVariables[i].dimInfo.size() == 0) {
          std::string varname = selectedVariables[i].name;

          try {
            CDF::Variable *v = cdfObject->getVariableThrows(varname);

            for (size_t j = 0; j < v->dimensionlinks.size(); j++) {
              int size = v->dimensionlinks[j]->getSize();
              int dimSize = getDimSize(dataSource, v->dimensionlinks[j]->name.c_str());
              if (COPENDAPHANDLER_DEBUG) {
                CDBDebug("Getting DimSize %d", dimSize);
              }
              if (dimSize > 0) {
                size = dimSize;
              }
              size_t count = size;
              size_t stride = 1;
              size_t start = 0;
              if (COPENDAPHANDLER_DEBUG) {
                CDBDebug("Push dimInfo varinfo %s", v->dimensionlinks[j]->name.c_str());
              }
              selectedVariables[i].dimInfo.push_back(VarInfo::Dim(v->dimensionlinks[j]->name.c_str(), start, count, stride));
            }
          } catch (int e) {
            CDBError("Exception %s", CDF::getErrorMessage(e).c_str());
          }
        }
      }

      //
      //         ){
      //         }
      //       }

      if (COPENDAPHANDLER_DEBUG) {
        std::string r = VarInfoToString(selectedVariables);

        CDBDebug("selectedVariables:[\n%s", r.c_str());
        CDBDebug("]");
      }

      std::string output = createDDSHeader(layerName, cdfObject, selectedVariables);
      if (jsonWriter) {
        if (!isDODRequest) {
          output += "\n}\n";
        }
        fprintf(opendapoutstream, "%s", output.c_str());

      } else {
        fprintf(opendapoutstream, "%s\n", output.c_str());
      }

      CDFObject *cdfObjectToRead = NULL;
      // Data request
      if (isDODRequest) {
        if (jsonWriter) {
          fprintf(opendapoutstream, ",\n  \"data\": {\n");
        } else {
          fprintf(opendapoutstream, "Data:\n");
        }
        bool varHasBeenWritten = false;
        for (size_t i = 0; i < selectedVariables.size(); i++) {
          for (size_t j = 0; j < cdfObject->variables.size(); j++) {
            CDF::Variable *v = cdfObject->variables[j];
            CDFType type = (CDFType)v->getType();

            if (selectedVariables[i].name == v->name) {
              if (jsonWriter && varHasBeenWritten) {
                fprintf(opendapoutstream, ",\n");
              }
              varHasBeenWritten = true;
              jsonValuesWritten = false;
              if (jsonWriter) {
                fprintf(opendapoutstream, "    \"%s\": [\n      ", v->name.c_str());
              }
              if (COPENDAPHANDLER_DEBUG) {
                CDBDebug("selectedVariables[i].dimInfo.size() = %zu", selectedVariables[i].dimInfo.size());
                CDBDebug("v->dimensionlinks.size()  = %zu", v->dimensionlinks.size());
              }
              bool hasAggregateDimension = false;
              // Check wether we need to iterate or not

              for (size_t k = 0; k < dataSource->requiredDims.size(); k++) {
                for (size_t l = 0; l < selectedVariables[i].dimInfo.size(); l++) {
                  if (dataSource->requiredDims[k].netCDFDimName == selectedVariables[i].dimInfo[l].name.c_str()) {
                    hasAggregateDimension = true;
                    break;
                    CDBDebug("Comparing [%s] ~ [%s]", dataSource->requiredDims[k].netCDFDimName.c_str(), selectedVariables[i].dimInfo[l].name.c_str());
                  }
                }
              }
              if (hasAggregateDimension) {
                std::vector<size_t> start(v->dimensionlinks.size());
                std::vector<size_t> count(v->dimensionlinks.size());
                std::vector<ptrdiff_t> stride(v->dimensionlinks.size());
                for (size_t k = 0; k < v->dimensionlinks.size(); k++) {
                  start[k] = selectedVariables[i].dimInfo[k].start;
                  count[k] = selectedVariables[i].dimInfo[k].count;
                  stride[k] = selectedVariables[i].dimInfo[k].stride;
                }
                // Convert start/count/stride to database request.

                if (COPENDAPHANDLER_DEBUG) {
                  CDBDebug("Starting reading partial data over aggregation dimension");
                }
                size_t varSize = 1;
                if (COPENDAPHANDLER_DEBUG) {
                  CDBDebug("Start retrieving files for variable %s", v->name.c_str());
                }
                for (size_t j = 0; j < v->dimensionlinks.size(); j++) {
                  if (COPENDAPHANDLER_DEBUG) {
                    CDBDebug(" start[%zu:%s] = %zu %zu %zu", j, v->dimensionlinks[j]->name.c_str(), start[j], count[j], stride[j]);
                  }
                  varSize *= count[j];
                }

                bool foundData = false;
                CDBStore::Store *store = NULL;
                try {
                  store = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getFilesForIndices(dataSource, start.data(), count.data(), stride.data(), 0);
                  if (store != NULL) {
                    if (COPENDAPHANDLER_DEBUG) {
                      CDBDebug("STORE SIZE %zu varSize = %zu", store->records.size(), varSize);
                    }
                    if (store->records.size() != 0) {
                      int intVarSize = varSize;
                      if (!jsonWriter) {
                        writeInt(intVarSize);
                        writeInt(intVarSize);
                      }
                      foundData = true;

                      std::string dimStandardName = "";
                      try {
                        dimStandardName = v->getAttributeThrows("standard_name")->toString();
                        ;
                      } catch (int e) {
                        dimStandardName = v->name;
                        ;
                      }
                      std::string dimUnits = "";
                      try {
                        dimUnits = v->getAttributeThrows("units")->toString();
                        ;
                      } catch (int e) {
                      }
                      bool readFromDB = false;
                      // If variable name equals dimension name, values are stored in the database.
                      CTime *time;
                      if (v->isDimension) {
                        if (type == CDF_DOUBLE) {
                          if (v->dimensionlinks.size() == 1) {
                            if (v->name == v->dimensionlinks[0]->name) {

                              if (dimStandardName == "time") {
                                if (dimUnits.length() > 2) {
                                  readFromDB = true;
                                  time = CTime::GetCTimeInstance(v);
                                  if (time == nullptr) {
                                    CDBDebug(CTIME_GETINSTANCE_ERROR_MESSAGE);
                                    return 1;
                                  }

                                } else {
                                  CDBDebug("%s name units are [%s]", v->name.c_str(), dimUnits.c_str());
                                }
                              } else {
                                CDBDebug("%s name is not time ", v->name.c_str());
                              }
                            } else {
                              CDBDebug("%s name not equal", v->name.c_str());
                            }
                          } else {
                            CDBDebug("%s size is not 1", v->name.c_str());
                          }
                        } else {
                          CDBDebug("%s is not of type DOUBLE", v->name.c_str());
                        }
                      } else {
                        if (COPENDAPHANDLER_DEBUG) {
                          CDBDebug("%s is not a dim", v->name.c_str());
                        }
                      }

                      for (auto &record: store->records) {

                        if (readFromDB) {
                          std::string dimValue = record.values.at(1);

                          if (COPENDAPHANDLER_DEBUG) {
                            CDBDebug("Dimension value from DB = [%s] units = [%s] standard_name = [%s]", dimValue.c_str(), dimUnits.c_str(), dimStandardName.c_str());
                          }
                          double value = time->dateToOffset(time->freeDateStringToDate(dimValue.c_str()));
                          writeDouble(value);
                        }

                        if (readFromDB == false) {
                          std::string fileName = record.values.at(0);
                          if (COPENDAPHANDLER_DEBUG) {
                            CDBDebug("Found file %s", fileName.c_str());
                          }
                          cdfObjectToRead = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeaderPlain(dataSource, dataSource->srvParams, fileName.c_str());
                          start[0] = std::stoi(record.values.at(2));
                          count[0] = 1;
                          if (COPENDAPHANDLER_DEBUG) {
                            CDBDebug("Start reading data for variable %s", v->name.c_str());
                            for (size_t j = 0; j < v->dimensionlinks.size(); j++) {
                              CDBDebug("  start[%zu] = %zu %zu %zu", j, start[j], count[j], stride[j]);
                            }
                          }
                          CDF::Variable *variableToRead = cdfObjectToRead->getVariableThrows(v->name);
                          variableToRead->readData(type, start.data(), count.data(), stride.data());
                          if (COPENDAPHANDLER_DEBUG) {
                            CDBDebug("Read %zu elements with type %s with element size %d", variableToRead->getSize(), CDF::getCDFDataTypeName(type).c_str(), CDF::getTypeSize(type));
                          }

                          putVariableData(variableToRead, type);
                        }
                      }
                    } else {

                      CDBDebug("Create missing data");
                      CDF::Variable *variableToRead = new CDF::Variable();
                      variableToRead->allocateData(varSize);
                      CDF::fill(variableToRead->data, type, 0, varSize);
                      putVariableData(variableToRead, type);
                      delete variableToRead;
                      foundData = true;
                    }
                  }
                  delete store;
                } catch (int e) {
                  delete store;
                  CDBError("Exception %s in getFilesForIndices", CDF::getErrorMessage(e).c_str());
                }

                if (foundData == false) {
                  if (COPENDAPHANDLER_DEBUG) {
                    CDBDebug("Read all data for %s", v->name.c_str());
                  }
                  int status = v->readData(type);
                  if (status != 0) {
                    CDBError("Unable to read data for %s", v->name.c_str());
                    return -1;
                  } else {
                    if (!jsonWriter) putVariableDataSize(v);
                    putVariableData(v, type);
                  }
                }

              } else {
                // No aggregate dimension
                if (COPENDAPHANDLER_DEBUG) {
                  CDBDebug("Read data for %s", v->name.c_str());
                }
                int status = 0;
                if (v->dimensionlinks.size() > 0) {
                  if (COPENDAPHANDLER_DEBUG) {
                    CDBDebug("READ PARTS");
                  }
                  std::vector<size_t> start(v->dimensionlinks.size());
                  std::vector<size_t> count(v->dimensionlinks.size());
                  std::vector<ptrdiff_t> stride(v->dimensionlinks.size());
                  for (size_t k = 0; k < v->dimensionlinks.size(); k++) {
                    start[k] = selectedVariables[i].dimInfo[k].start;
                    count[k] = selectedVariables[i].dimInfo[k].count;
                    stride[k] = selectedVariables[i].dimInfo[k].stride;
                  }
                  if (COPENDAPHANDLER_DEBUG) {
                    CDBDebug("Variable %s", v->name.c_str());
                    for (size_t j = 0; j < v->dimensionlinks.size(); j++) {
                      CDBDebug(" start[%zu] = %zu %zu %zu", j, start[j], count[j], stride[j]);
                    }
                  }
                  status = v->readData(type, start.data(), count.data(), stride.data());
                } else {
                  if (COPENDAPHANDLER_DEBUG) {
                    CDBDebug("READ ALL");
                  }

                  v->readData(type);
                }
                if (status != 0) {
                  CDBError("Unable to read data for %s", v->name.c_str());
                  delete dataSource;
                  return -1;
                } else {
                  if (!jsonWriter) putVariableDataSize(v);
                  putVariableData(v, type);
                }
              }
              if (jsonWriter) {
                fprintf(opendapoutstream, "\n    ]");
              }
            }
          }
        }
        if (jsonWriter) fprintf(opendapoutstream, "\n  }\n}\n");
      }
    }

    if (isDASRequest) {
      CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeaderPlain(dataSource, dataSource->srvParams, dataSource->getFileName().c_str());
      std::string output = "";
      if (jsonWriter)
        output += "{\n  \"attributes\": {\n";
      else
        output += "Attributes {\n";
      for (size_t i = 0; i < cdfObject->variables.size(); i++) {
        CDF::Variable *v = cdfObject->variables[i];
        {
          if (jsonWriter && i > 0) CT::printfconcat(output, ",\n");
          if (jsonWriter)
            CT::printfconcat(output, "    \"%s\": {\n", v->name.c_str());
          else
            CT::printfconcat(output, "    %s {\n", v->name.c_str());

          for (size_t j = 0; j < v->attributes.size(); j++) {
            {
              if (jsonWriter) {
                if (j > 0) {
                  CT::printfconcat(output, ",\n");
                }
              }
              std::string attrName = v->attributes[j]->name;
              CT::replaceSelf(attrName, " ", "_");
              CT::replaceSelf(attrName, "\"", "_");
              CT::replaceSelf(attrName, "[", "_");
              CT::replaceSelf(attrName, "]", "_");
              if (jsonWriter) {
                CT::printfconcat(output, "      \"%s\": ", attrName.c_str());
              } else {
                CT::printfconcat(output, "        %s %s ", CDFTypeToOpenDAPType::getatt(v->attributes[j]->type).c_str(), attrName.c_str());
              }
              if (v->attributes[j]->type == CDF_CHAR) {
                output += "\"";
                std::string s = v->attributes[j]->toString();

                CT::replaceSelf(s, "\"", "\\\"");

                output += s;
                if (v->attributes[j]->type == CDF_CHAR) output += "\"";
              } else {
                std::string s = v->attributes[j]->toString();
                CT::replaceSelf(s, " ", ",");
                output += s;
              }
              if (!jsonWriter) {
                CT::printfconcat(output, ";\n"); // TODO
              }
            }
          }
          if (jsonWriter)
            output += "\n    }";
          else
            output += "    }\n";
        }
      }
      if (jsonWriter)
        CT::printfconcat(output, "\n  }\n}");
      else
        CT::printfconcat(output, "}");
      fprintf(opendapoutstream, "%s\n", output.c_str());
    }

  } catch (int e) {
    CDBError("Exception with code %d found", e);
    return 1;
  }

  delete dataSource;
  if (COPENDAPHANDLER_DEBUG) {
    CDBDebug("**************************** OPENDAP END *******************************");
  }
  return 0;
}
