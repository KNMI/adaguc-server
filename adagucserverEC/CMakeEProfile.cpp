#include <vector>
#include <algorithm>
#include "CMakeEProfile.h"
#include "CImageDataWriter.h"
#include "CUniqueRequests/CURTypes.h"

static const bool CMakeEProfile_DEBUG = false;

#define CMakeEProfile_MAX_DIMS 255

#define DEFAULT_VALIDITY_LENGTH_OF_OBSERVATION_IN_SECONDS 12.0f

std::string encodeJSON(std::string input) {
  std::string str = input;
  CT::replaceSelf(str, "\"", "\\");
  CT::replaceSelf(str, "\n", "");
  CT::replaceSelf(str, "\"", "\\\"");
  return str;
}

class EProfileUniqueRequests {
public:
  bool readDataAsCDFDouble;

  class AggregatedDimension {
  public:
    std::string name;
    int start;
    std::vector<std::string> values;
  };

  struct DimInfo {
    ~DimInfo() {
      for (auto val: aggregatedValues) {
        delete val;
      }
      aggregatedValues.clear();
    }
    std::map<int, std::string> dimValuesMap;             // All values, many starts with 1 count, result of set()
    std::vector<AggregatedDimension *> aggregatedValues; // Aggregated values (start/count series etc), result of  addDimSet()
  };

  typedef std::map<std::string, DimInfo *>::iterator it_type_diminfo;

  class Request {
  public:
    int numDims;
    AggregatedDimension *dimensions[CMakeEProfile_MAX_DIMS];
  };

  int drawEprofile(CDrawImage *drawImage, CDF::Variable *variable, size_t *start, size_t *count, EProfileUniqueRequests::Request *, CDataSource *dataSource, std::string &eProfileJSON);
  int plotHeightRetrieval(CDrawImage *drawImage, CDFObject *cdfObject, const char *varName, CColor c, size_t NrOfDates, double startGraphTime, double startGraphRange, double graphWidth,
                          double graphHeight, int timeWidth);

  class FileInfo {
  public:
    std::vector<Request *> requests;
    std::map<std::string, DimInfo *> dimInfoMap; // AggregatedDimension name is key
    ~FileInfo() {
      for (it_type_diminfo diminfomapiterator = dimInfoMap.begin(); diminfomapiterator != dimInfoMap.end(); diminfomapiterator++) {
        delete diminfomapiterator->second;
      }
      for (size_t j = 0; j < requests.size(); j++) {
        delete requests[j];
      }
    }
  };

  std::map<std::string, FileInfo *> fileInfoMap; // File name is key

  typedef std::map<std::string, FileInfo *>::iterator it_type_file;

  int dimOrdering[CMakeEProfile_MAX_DIMS];

  int *getDimOrder() { return dimOrdering; }

  EProfileUniqueRequests() { readDataAsCDFDouble = false; }
  ~EProfileUniqueRequests() {
    typedef std::map<std::string, FileInfo *>::iterator it_type_file;
    for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {

      delete filemapiterator->second;
    }
    //     }
  }

  void set(const char *filename, const char *dimName, size_t dimIndex, std::string dimValue) {

    /* Find the right file based on filename */
    FileInfo *fileInfo = NULL;
    std::map<std::string, FileInfo *>::iterator itf = fileInfoMap.find(filename);
    if (itf != fileInfoMap.end()) {
      fileInfo = (*itf).second;
    } else {
      fileInfo = new FileInfo();
      fileInfoMap.insert(std::pair<std::string, FileInfo *>(filename, fileInfo));
    }

    /* Find the right diminfo based on dimension name */
    DimInfo *dimInfo = NULL;
    std::map<std::string, DimInfo *>::iterator itd = fileInfo->dimInfoMap.find(dimName);
    if (itd != fileInfo->dimInfoMap.end()) {
      dimInfo = (*itd).second;
    } else {
      dimInfo = new DimInfo();
      fileInfo->dimInfoMap.insert(std::pair<std::string, DimInfo *>(dimName, dimInfo));
    }

    dimInfo->dimValuesMap[dimIndex] = dimValue.c_str();
  }

  void addDimSet(DimInfo *dimInfo, int start, std::vector<std::string> valueList) {
    if (CMakeEProfile_DEBUG) {
      CDBDebug("Adding %d with %zu values", start, valueList.size());
    }
    AggregatedDimension *aggregatedValue = new AggregatedDimension();
    aggregatedValue->start = start;
    aggregatedValue->values = valueList;
    dimInfo->aggregatedValues.push_back(aggregatedValue);
  }

  AggregatedDimension *dimensions[CMakeEProfile_MAX_DIMS];

  void nestRequest(it_type_diminfo diminfomapiterator, FileInfo *fileInfo, int depth) {
    if (diminfomapiterator != fileInfo->dimInfoMap.end()) {
      it_type_diminfo currentIt = diminfomapiterator;
      int currentDepth = depth;
      diminfomapiterator++;
      depth++;
      for (size_t j = 0; j < (currentIt->second)->aggregatedValues.size(); j++) {
        AggregatedDimension *aggregatedValue = (currentIt->second)->aggregatedValues[j];
        aggregatedValue->name = (currentIt->first).c_str();
        dimensions[currentDepth] = aggregatedValue;
        nestRequest(diminfomapiterator, fileInfo, depth);
      }
      return;
    } else {
      if (CMakeEProfile_DEBUG) {
        CDBDebug("Add request with following:");
      }
      Request *request = new Request();
      for (int j = 0; j < depth; j++) {
        request->dimensions[j] = dimensions[j];
      }
      request->numDims = depth;
      fileInfo->requests.push_back(request);
      return;
    }
  }

  void sortAndAggregate() {
    for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {

      for (it_type_diminfo diminfomapiterator = (filemapiterator->second)->dimInfoMap.begin(); diminfomapiterator != (filemapiterator->second)->dimInfoMap.end(); diminfomapiterator++) {
        if (CMakeEProfile_DEBUG) {
          CDBDebug("%s/%s", (filemapiterator->first).c_str(), (diminfomapiterator->first).c_str());
        }
        auto *dimValuesMap = &diminfomapiterator->second->dimValuesMap;
        int currentDimIndex = -1;
        int dimindex = 0;

        int startDimIndex = 0;
        std::vector<std::string> dimValues;
        for (auto dimvalindexmapiterator = dimValuesMap->begin(); dimvalindexmapiterator != dimValuesMap->end(); dimvalindexmapiterator++) {
          dimindex = dimvalindexmapiterator->first;
          const char *dimvalue = dimvalindexmapiterator->second.c_str();

          if (currentDimIndex != -1) {
            if (currentDimIndex == dimindex - 1) {
              currentDimIndex = dimindex;
            } else {

              //*** GO ***
              if (CMakeEProfile_DEBUG) {
                CDBDebug("Print stop at %d", currentDimIndex);
              }
              currentDimIndex = -1;
              addDimSet(diminfomapiterator->second, startDimIndex, dimValues);
            }
          }

          if (currentDimIndex == -1) {
            if (CMakeEProfile_DEBUG) {
              CDBDebug("Print start at %d", dimindex);
            }
            currentDimIndex = dimindex;
            startDimIndex = dimindex;
            dimValues.clear();
          }

          if (currentDimIndex != -1) {
            dimValues.push_back(dimvalue);
          }
        }
        if (currentDimIndex != -1) {
          //*** GO ***
          if (CMakeEProfile_DEBUG) {
            CDBDebug("Print stop at %d", dimindex);
          }
          currentDimIndex = -1;
          addDimSet(diminfomapiterator->second, startDimIndex, dimValues);
        }
      }
    }

    // Generate EProfileUniqueRequests
    for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
      nestRequest((filemapiterator->second)->dimInfoMap.begin(), filemapiterator->second, 0);
    }
  }

  void makeRequests(CDrawImage *drawImage, CImageWarper *, CDataSource *dataSource, int, int, std::string &eProfileJson) {
    if (CMakeEProfile_DEBUG) {
      CDBDebug("\\makeRequests");
    }
    CDataReader reader;

    reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);

    int status = 0;
    status = drawImage->createImage(dataSource->srvParams->geoParams);

    if (status != 0) {
      CDBError("Unable to create image ");
      return;
    }

    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if (styleConfiguration->legendIndex != -1) {
      status = drawImage->createPalette(dataSource->srvParams->cfg->Legend[styleConfiguration->legendIndex]);
      if (status != 0) {
        CDBError("Unknown palette type for %s", dataSource->srvParams->cfg->Legend[styleConfiguration->legendIndex]->attr.name.c_str());
        return;
      }
    }
    if (CMakeEProfile_DEBUG) {
      CDBDebug("dataSource->dataObjects.size() = [%zu]", dataSource->dataObjects.size());
    }
    for (size_t dataObjectNr = 0; dataObjectNr < dataSource->dataObjects.size(); dataObjectNr++) {
      DataObject *dataObject = dataSource->getDataObject(dataObjectNr);
      std::string variableName = dataObject->cdfVariable->name;
      variableName += "_backup";
      // Show all requests

      for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
        if (CMakeEProfile_DEBUG) {
          CDBDebug("filemapiterator");
        }

        {

          CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource, dataSource->srvParams, (filemapiterator->first).c_str());
          CDF::Variable *variable = cdfObject->getVariableNE(variableName.c_str());
          dataObject->cdfVariable = variable;
          if (variable == NULL) {
            CDBError("Variable %s not found", variableName.c_str());
            throw(__LINE__);
          }

          for (size_t j = 0; j < (filemapiterator->second)->requests.size(); j++) {

            Request *request = (filemapiterator->second)->requests[j];
            if (CMakeEProfile_DEBUG) {
              CDBDebug("%s", (filemapiterator->first).c_str());
            }

            variable->freeData();

            std::vector<size_t> start(variable->dimensionlinks.size()), count(variable->dimensionlinks.size());
            std::vector<ptrdiff_t> stride(variable->dimensionlinks.size());

            for (size_t j = 0; j < variable->dimensionlinks.size(); j++) {
              start[j] = 0;
              count[j] = variable->dimensionlinks[j]->getSize();
              stride[j] = 1;
            }
            /*       start[dataSource->dimXIndex] = projCacheInfo.imx;
                   start[dataSource->dimYIndex] = projCacheInfo.imy;
               */

            if (CMakeEProfile_DEBUG) {
              for (size_t i = 0; i < variable->dimensionlinks.size(); i++) {
                CDBDebug("  %zu [%zu:%zu]", i, start[i], count[i]);
              }
            }

            variable->setType(CDF_FLOAT);
            int status = variable->readData(variable->currentType, start.data(), count.data(), stride.data(), true);

            if (status != 0) {
              CDBError("Unable to read variable %s", variable->name.c_str());
              throw(__LINE__);
            }

            if (status == 0) {
              /**
               * DataPostProc: Here our datapostprocessor comes into action!
               */
              for (size_t dpi = 0; dpi < dataSource->cfgLayer->DataPostProc.size(); dpi++) {
                CServerConfig::XMLE_DataPostProc *proc = dataSource->cfgLayer->DataPostProc[dpi];
                // Algorithm ax+b:
                if (proc->attr.algorithm == ("ax+b")) {
                  double dfadd_offset = 0;
                  double dfscale_factor = 1;

                  std::string offsetStr = proc->attr.b.c_str();
                  dfadd_offset = CT::toDouble(offsetStr);
                  std::string scaleStr = proc->attr.a.c_str();
                  dfscale_factor = CT::toDouble(scaleStr);
                  double *_data = (double *)variable->data;
                  for (size_t j = 0; j < variable->getSize(); j++) {
                    _data[j] = _data[j] * dfscale_factor + dfadd_offset;
                  }
                  // Convert the nodata type
                  dataSource->getDataObject(dataObjectNr)->dfNodataValue = dataSource->getDataObject(dataObjectNr)->dfNodataValue * dfscale_factor + dfadd_offset;
                }
                // Apply units:
                if (proc->attr.units.empty() == false) {
                  dataSource->getDataObject(dataObjectNr)->overruledUnits = proc->attr.units.c_str();
                }
              }
              /* End of data postproc */
              if (CMakeEProfile_DEBUG) {
                CDBDebug("Read %zu elements", variable->getSize());
              }

              drawEprofile(drawImage, variable, start.data(), count.data(), request, dataSource, eProfileJson);

              //               try{
              //               }
            }
          }
        }
      }

      /*
          try{
            createStructure(dataObject ,drawImage,imageWarper,dataSource,dX,dY,gfiStructure);
          }catch(int e){
            CDBError("Error in createStructure at line %d",e);
            throw(__LINE__);
          }*/
    }
    reader.close();
    if (CMakeEProfile_DEBUG) {
      CDBDebug("/makeRequests");
    }
  }

  size_t size() { return fileInfoMap.size(); }

  FileInfo *get(size_t index) {
    typedef std::map<std::string, FileInfo *>::iterator it_type_file;
    size_t s = 0;
    for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
      if (s == index) return filemapiterator->second;
      s++;
    }
    return NULL;
  }
};

int CMakeEProfile::MakeEProfile(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, std::string &eProfileJson) {
  EProfileUniqueRequests uniqueRequest;
  /**
   * DataPostProc: Here our datapostprocessor comes into action!
   */
  //     //Algorithm ax+b:
  //     }
  //   }

  int numberOfDims = dataSource->requiredDims.size();
  int numberOfSteps = dataSource->getNumTimeSteps();

  if (CMakeEProfile_DEBUG) {
    CDBDebug("1) /*Find all individual files*/");
  }

  for (int step = 0; step < numberOfSteps; step++) {
    dataSource->setTimeStep(step);
    for (int dimnr = 0; dimnr < numberOfDims; dimnr++) {
      COGCDims &ogcDim = dataSource->requiredDims[dimnr];
      uniqueRequest.set(dataSource->getFileName().c_str(), ogcDim.netCDFDimName.c_str(), dataSource->getDimensionIndex(dimnr), dataSource->getDimensionValue(dimnr));
    }
  }

  // Sort
  try {
    uniqueRequest.sortAndAggregate();
  } catch (int e) {
    CDBError("Error in sortAndAggregate at line %d", e);
    throw(__LINE__);
  }

  // Make requests
  try {
    uniqueRequest.makeRequests(drawImage, imageWarper, dataSource, dX, dY, eProfileJson);
  } catch (int e) {
    CDBError("Error in makeRequests at line %d", e);
    throw(__LINE__);
  }

  return 0;
};

int EProfileUniqueRequests::plotHeightRetrieval(CDrawImage *drawImage, CDFObject *cdfObject, const char *varName, CColor c, size_t NrOfDates, double startGraphTime, double startGraphRange,
                                                double graphWidth, double graphHeight, int timeWidth) {

  std::string newVarName;
  newVarName = CT::printf("%s", varName);
  CDF::Variable *plotHeightRetrievalVariable = cdfObject->getVariableNE(newVarName.c_str());
  if (plotHeightRetrievalVariable != NULL) {
    int status = plotHeightRetrievalVariable->readData(CDF_SHORT);
    if (status != 0) {
      CDBError("Unable to read %s data", varName);
      return 1;
    }
  }
  if (plotHeightRetrievalVariable != NULL) {
    double imageWidth = drawImage->geoParams.width;
    double imageHeight = drawImage->geoParams.height;
    CDF::Variable *varTime = cdfObject->getVariableNE("time_obs");
    if (plotHeightRetrievalVariable->data != NULL) {
      size_t numLayers = 1;

      if (plotHeightRetrievalVariable->dimensionlinks.size() == 2) {
        numLayers = plotHeightRetrievalVariable->dimensionlinks[1]->getSize();
      }

      size_t numLoaded = plotHeightRetrievalVariable->dimensionlinks[0]->getSize();
      if (NrOfDates > numLoaded - 1) NrOfDates = numLoaded - 1;

      for (size_t time = 0; time < NrOfDates; time++) {
        for (size_t layer = 0; layer < numLayers; layer++) {
          int x = int(((((double *)varTime->data)[time] - startGraphTime) / graphWidth) * imageWidth);
          float h = ((short *)plotHeightRetrievalVariable->data)[time * numLayers + layer];
          if (h > 0) {
            int y = imageHeight - int(((h - startGraphRange) / graphHeight) * imageHeight);
            drawImage->line(x - 1 + timeWidth / 2, y - 1, x + 1 + timeWidth / 2, y + 1, c);
            drawImage->line(x - 1 + timeWidth / 2, y + 1, x + 1 + timeWidth / 2, y - 1, c);
          }
        }
      }
    }
  }
  return 0;
}

int EProfileUniqueRequests::drawEprofile(CDrawImage *drawImage, CDF::Variable *variable, size_t *, size_t *count, EProfileUniqueRequests::Request *, CDataSource *dataSource,
                                         std::string &eProfileJson) {

  CTime *adagucTime = CTime::GetCTimeInstance(((CDFObject *)variable->getParentCDFObject())->getVariableNE("time_obs"));
  if (adagucTime == nullptr) {
    CDBDebug(CTIME_GETINSTANCE_ERROR_MESSAGE);
    return 1;
  }

  COGCDims &ogcDim = dataSource->requiredDims[0];

  if (CMakeEProfile_DEBUG) {
    CDBDebug("count %zu", count[0]);
    ;
    CDBDebug("total %zu", ogcDim.uniqueValues.size());
    CDBDebug("ogcDim.uniqueValues[0].c_str()) = %s", ogcDim.uniqueValues[0].c_str());
  }

  std::string rangeVarName = variable->dimensionlinks[1]->name.c_str();
  if (CMakeEProfile_DEBUG) {
    CDBDebug("Reading range var with name %s", rangeVarName.c_str());
  }
  CDF::Variable *varRange = ((CDFObject *)variable->getParentCDFObject())->getVariableNE(rangeVarName.c_str());
  if (varRange == NULL) {
    CDBError("%s not found", rangeVarName.c_str());
    return -1;
  }
  varRange->readData(CDF_FLOAT);

  CDF::Variable *varTime = ((CDFObject *)variable->getParentCDFObject())->getVariableNE("time_obs");
  if (varTime == NULL) {
    CDBError("%s not found", rangeVarName.c_str());
    return -1;
  }
  varTime->readData(CDF_DOUBLE);
  if (varTime->getSize() != count[0]) {
    CDBError("varTime->getSize()!=count[0] : %lu!=%lu", varTime->getSize(), count[0]);
    return 1;
  }

  double startGraphTime = adagucTime->dateToOffset(adagucTime->freeDateStringToDate(ogcDim.uniqueValues[0].c_str()));
  double stopGraphTime = adagucTime->dateToOffset(adagucTime->freeDateStringToDate(ogcDim.uniqueValues[ogcDim.uniqueValues.size() - 1].c_str()));

  double startGraphRange = ((float *)varRange->data)[0];
  double stopGraphRange = ((float *)varRange->data)[variable->dimensionlinks[1]->getSize() - 1];

  if (variable->dimensionlinks[1]->getSize() != count[1]) {
    CDBError("Range not equal");
    return 1;
  }

  int foundTimeDim = -1;
  for (size_t k = 0; k < dataSource->srvParams->requestDims.size(); k++) {
    if (dataSource->srvParams->requestDims[k].name == "time") {
      foundTimeDim = k;
      break;
    }
  }

  if (foundTimeDim != -1) {
    auto timeEntries = CT::split(dataSource->srvParams->requestDims[foundTimeDim].value, "/");
    if (timeEntries.size() == 2) {
      if (CMakeEProfile_DEBUG) {
        CDBDebug("time=%s", dataSource->srvParams->requestDims[foundTimeDim].value.c_str());
      }
      startGraphTime = adagucTime->dateToOffset(adagucTime->freeDateStringToDate(timeEntries[0].c_str()));
      stopGraphTime = adagucTime->dateToOffset(adagucTime->freeDateStringToDate(timeEntries[1].c_str()));
    }
  }

  int foundElevationDim = -1;
  for (size_t k = 0; k < dataSource->srvParams->requestDims.size(); k++) {
    if (dataSource->srvParams->requestDims[k].name == "elevation") {
      foundElevationDim = k;
      break;
    }
  }

  if (foundElevationDim != -1) {
    auto elevationEntries = CT::split(dataSource->srvParams->requestDims[foundElevationDim].value, "/");
    if (elevationEntries.size() == 2) {
      if (CMakeEProfile_DEBUG) {
        CDBDebug("elevation=%s", dataSource->srvParams->requestDims[foundElevationDim].value.c_str());
      }
      startGraphRange = std::stod(elevationEntries[0]);
      stopGraphRange = std::stod(elevationEntries[1]);
    }
  }

  if (dataSource->srvParams->InfoFormat == "application/json") {
    float *data = (float *)varRange->data;
    eProfileJson += "{";
    std::string units = dObjgetUnits(*dataSource->getDataObject(0));
    if (!units.empty()) {
      CT::printfconcat(eProfileJson, "\"units\":\"%s\",", encodeJSON(units).c_str());
    } else {
      CT::printfconcat(eProfileJson, "\"units\":null,");
    }
    CDF::Attribute *unitsY = varRange->getAttributeNE("units");
    CDF::Attribute *standardName = variable->getAttributeNE("standard_name");
    CDF::Attribute *longName = variable->getAttributeNE("long_name");
    std::string layerName = dataSource->getLayerName();
    std::string layerTitle = dataSource->getLayerTitle();

    std::string dq = "\"";

    CT::printfconcat(eProfileJson, "\"units_y\":%s,", (unitsY != NULL ? dq + std::string(unitsY->toString().c_str()) + dq : "null").c_str());
    CT::printfconcat(eProfileJson, "\"standard_name\":%s,", (standardName != NULL ? dq + std::string(standardName->toString().c_str()) + dq : "null").c_str());
    CT::printfconcat(eProfileJson, "\"long_name\":%s,", (longName != NULL ? dq + encodeJSON(longName->toString()) + dq : "null").c_str());
    CT::printfconcat(eProfileJson, "\"layer_name\":%s,", (layerName.empty() == false ? dq + encodeJSON(layerName) + dq : "null").c_str());
    CT::printfconcat(eProfileJson, "\"layer_title\":%s,", (layerTitle.empty() == false ? dq + encodeJSON(layerTitle) + dq : "null").c_str());
    CT::printfconcat(eProfileJson, "\"numValues\":%zu,", varRange->getSize());
    CT::printfconcat(eProfileJson, "\"name\":\"%s\",", encodeJSON(CT::replace(variable->name, "_backup", "")).c_str());

    CDBDebug("%lu", variable->getSize());

    size_t colOffset = varRange->getSize() * 0;
    if (count[0] > 1) {
      for (size_t timeIndex = 0; timeIndex < count[0] - 1; timeIndex++) {
        double dataTimeStart = ((double *)varTime->data)[timeIndex];
        double dataTimeEnd = ((double *)varTime->data)[timeIndex + 1];
        if (startGraphTime > dataTimeStart && startGraphTime < dataTimeEnd) {

          colOffset = timeIndex * varRange->getSize();
        }
      }
    }

    CDBDebug("Querying for time index %lu and file %s", colOffset, dataSource->getFileName().c_str());

    // Make profile object
    eProfileJson += "\"profile\":{";
    // Make height object
    eProfileJson += "\n\"heights\":[";
    CDBDebug("startGraphRange %f %f", startGraphRange, stopGraphRange);
    bool firstElDone = false;
    for (size_t j = 0; j < varRange->getSize(); j += 1) {
      float v = float(data[j]);
      if (v >= startGraphRange && v < stopGraphRange) {
        if (firstElDone) {
          eProfileJson += ",";
        };
        firstElDone = true;
        if (v == v) {
          CT::printfconcat(eProfileJson, "%g", v);
        } else {
          CT::printfconcat(eProfileJson, "null");
        }
      }
    }
    eProfileJson += "],";
    // Make values object
    eProfileJson += "\n\"values\":[";
    firstElDone = false;
    for (size_t j = 0; j < varRange->getSize(); j += 1) {
      float v = float(data[j]);
      if (v >= startGraphRange && v < stopGraphRange) {
        if (firstElDone) {
          eProfileJson += ",";
        };
        firstElDone = true;
        if (variable->getType() == CDF_FLOAT) {
          float v = ((float *)variable->data)[j + colOffset];
          if (v == v) {
            CT::printfconcat(eProfileJson, "%g", v);
          } else {
            CT::printfconcat(eProfileJson, "null");
          }
        }
        if (variable->getType() == CDF_DOUBLE) {
          double v = ((double *)variable->data)[j + colOffset];
          if (v == v) {
            CT::printfconcat(eProfileJson, "%g", v);
          } else {
            CT::printfconcat(eProfileJson, "null");
          }
        }
      }
    }
    eProfileJson += "]";

    eProfileJson += "\n}}";

    return 0;
  }

  double graphWidth = stopGraphTime - startGraphTime;
  double graphHeight = stopGraphRange - startGraphRange;

  double imageWidth = drawImage->geoParams.width;
  double imageHeight = drawImage->geoParams.height;

  if (graphWidth <= 0) {
    graphWidth = imageWidth;
  }
  if (graphHeight <= 0) {
    graphHeight = imageHeight;
  }

  if (CMakeEProfile_DEBUG) {
    CDBDebug("startGraphTime = %f stopGraphTime = %f graphWidth = %f imageWidth = %f", startGraphTime, stopGraphTime, graphWidth, imageWidth);
    CDBDebug("startGraphRange = %f stopGraphTime = %f graphWidth = %f imageWidth = %f", startGraphRange, stopGraphTime, graphHeight, imageHeight);
  }

  if (CMakeEProfile_DEBUG) {
    CDBDebug("Number of timesteps: %zu", count[0]);
  }

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();

  double dfNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  float nodataValue = (float)dfNodataValue;
  float legendValueRange = styleConfiguration->hasLegendValueRange;
  float legendLowerRange = styleConfiguration->legendLowerRange;
  float legendUpperRange = styleConfiguration->legendUpperRange;
  bool hasNodataValue = dataSource->getDataObject(0)->hasNodataValue;
  float legendLogAsLog = 0;
  float legendLog = styleConfiguration->legendLog;
  if (legendLog > 0) {
    legendLogAsLog = log10(legendLog);
  }
  float legendScale = styleConfiguration->legendScale;
  float legendOffset = styleConfiguration->legendOffset;

  std::vector<CMakeEProfile::DayPass> dayPasses;
  int minWidth = 0;

  // Fallback validity length of the observation, in case there is only one observation in the file
  double duration = DEFAULT_VALIDITY_LENGTH_OF_OBSERVATION_IN_SECONDS;

  CDF::Attribute *durationAttribute = varTime->getAttributeNE("duration");
  if (durationAttribute != NULL && durationAttribute->getType() == CDF_DOUBLE) {
    durationAttribute->getData(&duration, 1);
  }

  for (size_t time = 0; time < count[0]; time++) {

    int x1 = int(((((double *)varTime->data)[time] - startGraphTime) / graphWidth) * imageWidth);
    int x2 = 0;

    if (time < count[0] - 1) {
      x2 = int(((((double *)varTime->data)[time + 1] - startGraphTime) / graphWidth) * imageWidth);
      if (minWidth == 0) {
        minWidth = x2 - x1;
      } else {
        if (x2 - x1 < minWidth) {
          minWidth = x2 - x1;
        }
      }
    } else {
      x2 = x1 + int(((duration / graphWidth) * imageWidth) + 0.5) + 1;
    }
    if (x2 >= 0 && x1 < imageWidth && x1 < x2) {

      for (size_t range = 0; range < count[1] - 1; range++) {

        int y1 = imageHeight - int(((((float *)varRange->data)[range + 1] - startGraphRange) / graphHeight) * imageHeight);
        int y2 = imageHeight - int(((((float *)varRange->data)[range] - startGraphRange) / graphHeight) * imageHeight);
        if (y2 >= 0 && y1 < imageHeight && y1 < y2) {
          float *data = (float *)(variable->data);
          size_t p = range + time * count[1];
          float val = data[p];

          bool isNodata = false;
          if (hasNodataValue) {
            if (val == nodataValue) isNodata = true;
          }
          if (!(val == val)) isNodata = true;
          if (!isNodata)
            if (legendValueRange)
              if (val < legendLowerRange || val > legendUpperRange) isNodata = true;
          if (!isNodata) {
            if (legendLog != 0) {
              if (val > 0) {
                val = (log10(val) / legendLogAsLog);
              } else
                val = (-legendOffset);
            }

            int pcolorind = (int)(val * legendScale + legendOffset);
            if (pcolorind >= 239)
              pcolorind = 239;
            else if (pcolorind <= 0)
              pcolorind = 0;

            for (int y = y1; y < y2; y++) {
              for (int x = x1; x < x2; x++) {
                drawImage->setPixelIndexed(x, y, pcolorind);
              }
            }
          }
        }
      }
    }

    dayPasses.push_back(CMakeEProfile::DayPass(x1, ((double *)varTime->data)[time]));
  }

  plotHeightRetrieval(drawImage, ((CDFObject *)variable->getParentCDFObject()), "cbh", CColor(0, 0, 255, 255), count[0], startGraphTime, startGraphRange, graphWidth, graphHeight, minWidth);

  for (size_t j = 0; j < dayPasses.size(); j++) {
    CTime::Date d = adagucTime->offsetToDate(dayPasses[j].offset);
    if (d.minute == 0 && d.hour == 0) {
      std::string dateStr = adagucTime->dateToISOString(d);
      dateStr.resize(10);
      drawImage->setText(dateStr.c_str(), dayPasses[j].x + 4, 5, CColor(0, 0, 0, 0));

      for (int y = 0; y < imageHeight; y++) {
        drawImage->setPixelTrueColor(dayPasses[j].x, y, 0, 0, 255, 255);
      }
    }
  }

  return 0;
}
