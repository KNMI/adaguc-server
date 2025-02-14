#include <vector>
#include <algorithm>
#include "CMakeJSONTimeSeries.h"
#include "CImageDataWriter.h"
#include "CURUniqueRequests.h"

// #define CCUniqueRequests_DEBUG
// #define CCUniqueRequests_DEBUG_HIGH
const char *CURUniqueRequests::className = "CURUniqueRequests";

CURUniqueRequests::CURUniqueRequests() { readDataAsCDFDouble = false; }

CURUniqueRequests::~CURUniqueRequests() {
  typedef std::map<std::string, CURFileInfo *>::iterator it_type_file;
  for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {

    delete filemapiterator->second;
  }

  results.clear();
}

int *CURUniqueRequests::getDimOrder() { return dimOrdering; }

void CURUniqueRequests::set(const char *filename, const char *dimName, size_t dimIndex, CT::string dimValue) {

  /* Find the right file based on filename */
  CURFileInfo *fileInfo = NULL;
  std::map<std::string, CURFileInfo *>::iterator itf = fileInfoMap.find(filename);
  if (itf != fileInfoMap.end()) {
    fileInfo = (*itf).second;
  } else {
    fileInfo = new CURFileInfo();
    fileInfoMap.insert(std::pair<std::string, CURFileInfo *>(filename, fileInfo));
  }

  /* Find the right diminfo based on dimension name */
  CURDimInfo *dimInfo = NULL;
  std::map<std::string, CURDimInfo *>::iterator itd = fileInfo->dimInfoMap.find(dimName);
  if (itd != fileInfo->dimInfoMap.end()) {
    dimInfo = (*itd).second;
  } else {
    dimInfo = new CURDimInfo();
    fileInfo->dimInfoMap.insert(std::pair<std::string, CURDimInfo *>(dimName, dimInfo));
  }

  /* Find the right dimension indexes and values based on dimension index */
  CT::string *dimIndexesAndValues = NULL;
  std::map<int, CT::string *>::iterator itdi = dimInfo->dimValuesMap.find(dimIndex);
  if (itdi != dimInfo->dimValuesMap.end()) {
    dimIndexesAndValues = (*itdi).second;
  } else {
    dimIndexesAndValues = new CT::string();
    dimInfo->dimValuesMap.insert(std::pair<int, CT::string *>(dimIndex, dimIndexesAndValues));
  }

  dimIndexesAndValues->copy(dimValue.c_str());
#ifdef CCUniqueRequests_DEBUG
  CDBDebug("Adding %s %d %s", dimName, dimIndex, dimValue.c_str());
#endif
}

void CURUniqueRequests::addDimSet(CURDimInfo *dimInfo, int start, std::vector<CT::string> valueList) {
#ifdef CCUniqueRequests_DEBUG
  CDBDebug("Adding %d with %d values", start, valueList.size());
#endif
  CURAggregatedDimension *aggregatedValue = new CURAggregatedDimension();
  aggregatedValue->start = start;
  aggregatedValue->values.assign(valueList.begin(), valueList.end());
  dimInfo->aggregatedValues.push_back(aggregatedValue);
}

void CURUniqueRequests::nestRequest(it_type_diminfo diminfomapiterator, CURFileInfo *fileInfo, int depth) {
  if (diminfomapiterator != fileInfo->dimInfoMap.end()) {
    it_type_diminfo currentIt = diminfomapiterator;
    int currentDepth = depth;
    diminfomapiterator++;
    depth++;
    for (size_t j = 0; j < (currentIt->second)->aggregatedValues.size(); j++) {
      CURAggregatedDimension *aggregatedValue = (currentIt->second)->aggregatedValues[j];
      aggregatedValue->name = (currentIt->first).c_str();
      dimensions[currentDepth] = aggregatedValue;
      nestRequest(diminfomapiterator, fileInfo, depth);
    }
    return;
  } else {
#ifdef CCUniqueRequests_DEBUG_HIGH
    CDBDebug("Add request with following:");
#endif
    CURRequest *request = new CURRequest();
    for (int j = 0; j < depth; j++) {
#ifdef CCUniqueRequests_DEBUG_HIGH
      CDBDebug("  %d %s %d %d", j, dimensions[j]->name.c_str(), dimensions[j]->start, dimensions[j]->values.size());
#endif
      request->dimensions[j] = dimensions[j];
    }
    request->numDims = depth;
    fileInfo->requests.push_back(request);
    return;
  }
}

void CURUniqueRequests::sortAndAggregate() {
  for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {

    for (it_type_diminfo diminfomapiterator = (filemapiterator->second)->dimInfoMap.begin(); diminfomapiterator != (filemapiterator->second)->dimInfoMap.end(); diminfomapiterator++) {
#ifdef CCUniqueRequests_DEBUG
      CDBDebug("%s/%s", (filemapiterator->first).c_str(), (diminfomapiterator->first).c_str());
#endif
      std::map<int, CT::string *> *dimValuesMap = &diminfomapiterator->second->dimValuesMap;
      int currentDimIndex = -1;
      int dimindex;

      int startDimIndex;
      std::vector<CT::string> dimValues;
      for (it_type_dimvalindex dimvalindexmapiterator = dimValuesMap->begin(); dimvalindexmapiterator != dimValuesMap->end(); dimvalindexmapiterator++) {
        dimindex = dimvalindexmapiterator->first;
        const char *dimvalue = dimvalindexmapiterator->second->c_str();

        if (currentDimIndex != -1) {
          if (currentDimIndex == dimindex - 1) {
            currentDimIndex = dimindex;
          } else {

            //*** GO ***
#ifdef CCUniqueRequests_DEBUG
            CDBDebug("Print stop at %d", currentDimIndex);
#endif
            currentDimIndex = -1;
            addDimSet(diminfomapiterator->second, startDimIndex, dimValues);
          }
        }

        if (currentDimIndex == -1) {
#ifdef CCUniqueRequests_DEBUG
          CDBDebug("Print start at %d", dimindex);
#endif
          currentDimIndex = dimindex;
          startDimIndex = dimindex;
          dimValues.clear();
        }

        if (currentDimIndex != -1) {
#ifdef CCUniqueRequests_DEBUG
          CDBDebug("Add %d/%s", dimindex, dimvalue);
#endif

          dimValues.push_back(dimvalue);
        }
      }
      if (currentDimIndex != -1) {
        //*** GO ***
#ifdef CCUniqueRequests_DEBUG
        CDBDebug("Print stop at %d", dimindex);
#endif
        currentDimIndex = -1;
        addDimSet(diminfomapiterator->second, startDimIndex, dimValues);
      }
    }
  }

  // Generate CURUniqueRequests
  for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
    nestRequest((filemapiterator->second)->dimInfoMap.begin(), filemapiterator->second, 0);
  }
}

void CURUniqueRequests::recurDataStructure(CXMLParser::XMLElement *dataStructure, CURResult *result, int depth, int *dimOrdering, std::vector<int> dimIndicesToSkip) {
  int dimIndex = dimOrdering[depth];

  for (const int &dimIndexToSkip : dimIndicesToSkip) {
    if (depth == dimIndexToSkip) {
      return recurDataStructure(dataStructure, result, depth + 1, dimOrdering, dimIndicesToSkip);
    }
  }
  CT::string dimindexvalue = result->dimensionKeys[dimIndex].name.c_str();

  CXMLParser::XMLElement *el = NULL;
  try {
    el = dataStructure->get(dimindexvalue.c_str());
  } catch (int e) {
    dataStructure->add(CXMLParser::XMLElement(dimindexvalue.c_str()));
    el = dataStructure->getLast();
  }
  if (depth + 1 < result->numDims) {
    recurDataStructure(el, result, depth + 1, dimOrdering, dimIndicesToSkip);
  } else {
    el->setValue(result->value.c_str());
  }
}

void CURUniqueRequests::createStructure(CDataSource::DataObject *dataObject, CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY,
                                        CXMLParser::XMLElement *gfiStructure) {

  int numberOfDims = dataSource->requiredDims.size();

  CXMLParser::XMLElement *layerStructure = gfiStructure->add("root");
  layerStructure->add(CXMLParser::XMLElement("name", dataSource->getLayerName()));

  /* Add metadata */
  CT::string standardName = dataObject->variableName.c_str();
  CDF::Attribute *attr_standard_name = dataObject->cdfVariable->getAttributeNE("standard_name");
  if (attr_standard_name != NULL) {
    standardName = attr_standard_name->toString();
  }

  layerStructure->add(CXMLParser::XMLElement("standard_name", standardName.c_str()));
  layerStructure->add(CXMLParser::XMLElement("units", dataObject->getUnits().c_str()));

  CT::string ckey;
  ckey.print("%d%d%s", dX, dY, dataSource->nativeProj4.c_str());
  CImageDataWriter::ProjCacheInfo projCacheInfo = CImageDataWriter::GetProjInfo(ckey, drawImage, dataSource, imageWarper, dataSource->srvParams, dX, dY);
  CXMLParser::XMLElement point("point");
  point.add(CXMLParser::XMLElement("SRS", "EPSG:4326"));
  CT::string coord;
  coord.print("%f,%f", projCacheInfo.lonX, projCacheInfo.lonY);
  point.add(CXMLParser::XMLElement("coords", coord.c_str()));
  layerStructure->add(point);

  std::vector<int> dimIndicesToSkip;
  for (size_t i = 0; i < size_t(numberOfDims); i++) {
    COGCDims *ogcDim = dataSource->requiredDims[dimOrdering[i]];
    if (ogcDim->hidden) {
      dimIndicesToSkip.push_back(i);
    }
  }

  for (size_t i = 0; i < size_t(numberOfDims); i++) {

    if (std::find(dimIndicesToSkip.begin(), dimIndicesToSkip.end(), i) == dimIndicesToSkip.end()) {
      CT::string dimNameToFind = dataSource->requiredDims[dimOrdering[i]]->name.c_str();

      layerStructure->add(CXMLParser::XMLElement("dims", dimNameToFind.c_str()));
    }
  }

  CXMLParser::XMLElement *dataStructure = NULL;
  try {
    dataStructure = layerStructure->get("data");
  } catch (int e) {
    layerStructure->add(CXMLParser::XMLElement("data"));
    dataStructure = layerStructure->getLast();
  }

  std::sort(results.begin(), results.end(), less_than_key());
  CDBDebug("Found %d elements", results.size());

  for (size_t j = 0; j < results.size(); j++) {
    recurDataStructure(dataStructure, &results[j], 0, dimOrdering, dimIndicesToSkip);
  }
}

void CURUniqueRequests::makeRequests(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure) {
#ifdef CCUniqueRequests_DEBUG_HIGH
  CDBDebug("======================== makeRequests ========================================");
#endif
  CDataReader reader;
  int numberOfDataSourceDims = dataSource->requiredDims.size();

  /* Determine ordering of dimensions */

  for (int dimnr = 0; dimnr < numberOfDataSourceDims; dimnr++) {
    dimOrdering[dimnr] = dimnr;
  }
  for (int dimnr = 0; dimnr < numberOfDataSourceDims; dimnr++) {
    COGCDims *ogcDim = dataSource->requiredDims[dimnr];
    if (ogcDim->name.equalsIgnoreCase("reference_time") && dimnr != 0) {
      std::swap(dimOrdering[dimnr], dimOrdering[0]);
      break;
    }
  }

  for (int dimnr = 0; dimnr < numberOfDataSourceDims; dimnr++) {
    COGCDims *ogcDim = dataSource->requiredDims[dimOrdering[dimnr]];
    if (ogcDim->name.equalsIgnoreCase("time") && dimnr != numberOfDataSourceDims - 1) {
      std::swap(dimOrdering[dimnr], dimOrdering[numberOfDataSourceDims - 1]);
      break;
    }
  }

#ifdef CCUniqueRequests_DEBUG
  for (int dimnr = 0; dimnr < numberOfDataSourceDims; dimnr++) {
    CDBDebug("New order = %d/%d = [%s]", dimnr, dimOrdering[dimnr], dataSource->requiredDims[dimOrdering[dimnr]]->name.c_str());
  }
#endif

  /* Including x and y dimensions, without virtual dims */
  int numberOfDims = numberOfDataSourceDims + 2;
  size_t start[numberOfDims], count[numberOfDims];
  ptrdiff_t stride[numberOfDims];
  CT::string dimName[numberOfDims];

  reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);

#ifdef CCUniqueRequests_DEBUG_HIGH
  CDBDebug("===================== iterating data objects ==================");
#endif
  for (size_t dataObjectNr = 0; dataObjectNr < dataSource->dataObjects.size(); dataObjectNr++) {

    results.clear();
    CDataSource::DataObject *dataObject = dataSource->getDataObject(dataObjectNr);
    CT::string variableName = dataObject->cdfVariable->name;
    // Show all requests

    for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
      CT::string ckey;
      ckey.print("%d%d%s", dX, dY, dataSource->nativeProj4.c_str());
      CImageDataWriter::ProjCacheInfo projCacheInfo = CImageDataWriter::GetProjInfo(ckey, drawImage, dataSource, imageWarper, dataSource->srvParams, dX, dY);

      if (projCacheInfo.isOutsideBBOX == false) {
        CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource, dataSource->srvParams, (filemapiterator->first).c_str());

        //   if (cdfObject->getVariableNE("forecast_reference_time") != NULL) {
        //     CDBDebug("Has forecast_reference_time");
        //     CDF::Dimension *forecastRefDim = new CDF::Dimension();
        //     forecastRefDim->name = "forecast_reference_time";
        //     forecastRefDim->setSize(1);
        //     forecastRefDim->isVirtualDimension = true;
        //     cdfObject->addDimension(forecastRefDim);
        //   }

        //   bool foundReferenceTime = false;
        //   for (size_t j = 0; j < dataObject->cdfVariable->dimensionlinks.size(); j++) {
        //     if (dataObject->cdfVariable->dimensionlinks[j]->name.equals("forecast_reference_time")) {
        //       foundReferenceTime = true;
        //       break;
        //     }
        //   }
        //   if (foundReferenceTime == false) {
        //     CDF::Dimension *forecastRefDim = cdfObject->getDimensionNE("forecast_reference_time");
        //     if (forecastRefDim != NULL) {
        //       // TODO: Maarten Plieger 2022-01-20, This is inserted at thewrong place
        //       // dataObject->cdfVariable->dimensionlinks.insert(dataObject->cdfVariable->dimensionlinks.begin() + dataObject->cdfVariable->dimensionlinks.size() - 2, forecastRefDim);
        //       // dataObject->cdfVariable->dimensionlinks.insert(dataObject->cdfVariable->dimensionlinks.begin(), forecastRefDim);
        //       /* Add the forecast_reference_time dimension to the end of the list to avoid messing up other indices */
        //       dataObject->cdfVariable->dimensionlinks.push_back(forecastRefDim);
        //     }
        //   }

#ifdef CCUniqueRequests_DEBUG
        CDBDebug("Getting data variable [%s]", variableName.c_str());
#endif

        CDF::Variable *variable = cdfObject->getVariableNE(variableName.c_str());
        dataObject->cdfVariable = variable;
        if (variable == NULL) {
          CDBError("Variable %s not found", variableName.c_str());
          throw(__LINE__);
        }

        for (size_t j = 0; j < (filemapiterator->second)->requests.size(); j++) {

          CURRequest *request = (filemapiterator->second)->requests[j];
#ifdef CCUniqueRequests_DEBUG
          CDBDebug("Reading file %s  for variable %s", (filemapiterator->first).c_str(), variable->name.c_str());
#endif

          variable->freeData();

          for (int j = 0; j < numberOfDims; j++) {
            start[j] = 0;
            count[j] = 1;
            stride[j] = 1;
          }
#ifdef CCUniqueRequests_DEBUG
          CDBDebug("Querying raster location %d %d", projCacheInfo.imx, projCacheInfo.imy);
          CDBDebug("Querying raster dimIndex %d %d", dataSource->dimXIndex, dataSource->dimYIndex);
#endif
          start[dataSource->dimXIndex] = projCacheInfo.imx;
          start[dataSource->dimYIndex] = projCacheInfo.imy;
          dimName[dataSource->dimXIndex] = "x";
          dimName[dataSource->dimYIndex] = "y";

          for (int i = 0; i < request->numDims; i++) {
            int netcdfDimIndex = -1;
            CDataReader::DimensionType dtype = CDataReader::getDimensionType(cdfObject, request->dimensions[i]->name.c_str());
            if (dtype != CDataReader::dtype_reference_time) {
              if (dtype == CDataReader::dtype_none) {
                CDBWarning("dtype_none for %s", dtype, request->dimensions[i]->name.c_str());
              }
              try {
                /* CHECK */

                netcdfDimIndex = variable->getDimensionIndex(request->dimensions[i]->name.c_str());
              } catch (int e) {
                // CDBError("Unable to find dimension [%s]",request->dimensions[i]->name.c_str());
                if (dtype == CDataReader::dtype_reference_time) {
                  CDBDebug("IS REFERENCE TIME %s", request->dimensions[i]->name.c_str());

                } else {
                  CDBError("IS NOT REFERENCE TIME %s", request->dimensions[i]->name.c_str());
                  throw(__LINE__);
                }
              }
              if (netcdfDimIndex == dataSource->dimXIndex || netcdfDimIndex == dataSource->dimYIndex) {
                CDBWarning("netcdfDimIndex %d already taken for %s", netcdfDimIndex, request->dimensions[i]->name.c_str());
              }
              start[netcdfDimIndex] = request->dimensions[i]->start;
              count[netcdfDimIndex] = request->dimensions[i]->values.size();
              dimName[netcdfDimIndex] = request->dimensions[i]->name;
#ifdef CCUniqueRequests_DEBUG
              CDBDebug("  request index: %d  netcdfdimindex %d  %s %d %d", i, netcdfDimIndex, request->dimensions[i]->name.c_str(), request->dimensions[i]->start,
                       request->dimensions[i]->values.size());
#endif
            }
          }
#ifdef CCUniqueRequests_DEBUG_HIGH
          for (int i = 0; i < numberOfDims; i++) {
            CDBDebug("  %d %s [%d:%d:%d]", i, dimName[i].c_str(), start[i], count[i], stride[i]);
          }
#endif

          /*
           * In case a scale_factor and add_offset attribute is present, we need to read the data into the same datatype as this attribute
           * This allows it to be unpacked properly to the final scaled values
           */
          CDF::Attribute *scale_factor = variable->getAttributeNE("scale_factor");
          if (scale_factor != NULL) {
            variable->setType(CDF_FLOAT);
            if (scale_factor->getType() == CDF_DOUBLE) {
              variable->setType(CDF_DOUBLE);
            }
          }

          if (readDataAsCDFDouble) {
            variable->setType(CDF_DOUBLE);
          }

#ifdef CCUniqueRequests_DEBUG_HIGH
          CDBDebug("Starting read data as type %s", CDF::getCDFDataTypeName(variable->currentType).c_str());
#endif
          int status = variable->readData(variable->currentType, start, count, stride, true);
#ifdef CCUniqueRequests_DEBUG_HIGH
          CDBDebug("Read %d elements", variable->getSize());
#endif
          if (status != 0) {
            CDBError("Unable to read variable %s", variable->name.c_str());
            throw(__LINE__);
          }

#ifdef CCUniqueRequests_DEBUG_HIGH
          for (size_t j = 0; j < variable->getSize(); j++) {
            CDBDebug("Orig Data value %d is \t %f", j, ((double *)variable->data)[j]);
          }
#endif
          if (status == 0) {
            /**
             * DataPostProc: Here our datapostprocessor comes into action!
             */
            for (size_t dpi = 0; dpi < dataSource->cfgLayer->DataPostProc.size(); dpi++) {
              CServerConfig::XMLE_DataPostProc *proc = dataSource->cfgLayer->DataPostProc[dpi];
              // Algorithm ax+b:
              if (proc->attr.algorithm.equals("ax+b")) {
                double dfadd_offset = 0;
                double dfscale_factor = 1;

                CT::string offsetStr = proc->attr.b.c_str();
                dfadd_offset = offsetStr.toDouble();
                CT::string scaleStr = proc->attr.a.c_str();
                dfscale_factor = scaleStr.toDouble();
                double *_data = (double *)variable->data;
                for (size_t j = 0; j < variable->getSize(); j++) {
                  // if(j%10000==0){CDBError("%d = %f",j,_data[j]);}
                  _data[j] = _data[j] * dfscale_factor + dfadd_offset;
                }
                // Convert the nodata type
                dataSource->getDataObject(dataObjectNr)->dfNodataValue = dataSource->getDataObject(dataObjectNr)->dfNodataValue * dfscale_factor + dfadd_offset;
              }
              // Apply units:
              if (proc->attr.units.empty() == false) {
                dataSource->getDataObject(dataObjectNr)->setUnits(proc->attr.units.c_str());
              }
            }
            if (readDataAsCDFDouble) {
              CDataPostProcessor::getCDPPExecutor()->executeProcessors(dataSource, CDATAPOSTPROCESSOR_RUNAFTERREADING, (double *)variable->data, variable->getSize());
            }
            /* End of data postproc */
#ifdef CCUniqueRequests_DEBUG
            CDBDebug("Read %d elements", variable->getSize());

            for (size_t j = 0; j < variable->getSize(); j++) {
              CDBDebug("New Data value %d is \t %f", j, ((double *)variable->data)[j]);
            }
#endif
            try {
              int multiplies[variable->dimensionlinks.size()];
              for (size_t d = 0; d < variable->dimensionlinks.size(); d += 1) {
                int m = 1;

                for (size_t j = 0; j < variable->dimensionlinks.size() - (d + 1); j += 1) {
                  int index = variable->dimensionlinks.size() - 1 - j;
                  m *= count[index];
                }
                multiplies[d] = m;
              }

              for (size_t indexInVariable = 0; indexInVariable < variable->getSize(); indexInVariable++) {

                CURResult currentResultForIndex;
                currentResultForIndex.parent = this;
                currentResultForIndex.numDims = numberOfDataSourceDims;

                // Fill in the dimension keys
                for (size_t dataSourceDimIndex = 0; dataSourceDimIndex < dataSource->requiredDims.size(); dataSourceDimIndex++) {
                  CT::string requestDimNameToFind = dataSource->requiredDims[dataSourceDimIndex]->netCDFDimName.c_str();
                  currentResultForIndex.dimensionKeys[dataSourceDimIndex].name = dataSource->requiredDims[dataSourceDimIndex]->value;
                  int variableDimIndex = -1;
                  for (size_t d = 0; d < variable->dimensionlinks.size() - 2; d += 1) {
                    if (variable->dimensionlinks[d]->name.equals(requestDimNameToFind)) {
                      variableDimIndex = d;
                    }
                  }
                  if (variableDimIndex != -1) {
                    int requestDimIndex = -1;
                    for (int i = 0; i < request->numDims; i++) {
                      if (request->dimensions[i]->name.equals(requestDimNameToFind.c_str())) {
                        requestDimIndex = i;
                      }
                    }
                    if (requestDimIndex == -1) {
                      CDBError("Unable to find dimension %s in request", requestDimNameToFind.c_str());
                      throw(__LINE__);
                    }
                    auto values = request->dimensions[requestDimIndex]->values;
                    size_t numValues = values.size();
                    size_t multiplyIndex = multiplies[variableDimIndex];
                    CT::string dimStr = values[(indexInVariable / multiplyIndex) % numValues].c_str();
                    currentResultForIndex.dimensionKeys[dataSourceDimIndex].name = dimStr;
                  }
                }

                double pixelValueAtIndex = CImageDataWriter::convertValue(variable->getType(), variable->data, indexInVariable);
                CT::string pixelValueAsString = "nodata";
                if ((pixelValueAtIndex != dataObject->dfNodataValue && dataObject->hasNodataValue == true && pixelValueAtIndex == pixelValueAtIndex) || dataObject->hasNodataValue == false) {
                  if (dataObject->hasStatusFlag) {
                    CT::string flagMeaning;
                    CDataSource::getFlagMeaningHumanReadable(&flagMeaning, &dataObject->statusFlagList, pixelValueAtIndex);
                    pixelValueAsString.print("%s (%d)", flagMeaning.c_str(), (int)pixelValueAtIndex);
                  } else {
                    pixelValueAsString.print("%f", pixelValueAtIndex);
                  }
                }

                // Set the value
                currentResultForIndex.value = pixelValueAsString;
                results.push_back(currentResultForIndex);
              }
            } catch (int e) {
              CDBError("Error in expandData at line %d", e);
              throw(__LINE__);
            }
          }
        }
      }
    }

    try {
#ifdef CCUniqueRequests_DEBUG_HIGH
      CDBDebug("**************** Create Structure ********************");
      CDBDebug("dataObjectNr: %d", dataObjectNr);
#endif
      createStructure(dataObject, drawImage, imageWarper, dataSource, dX, dY, gfiStructure);
    } catch (int e) {
      CDBError("Error in createStructure at line %d", e);
      throw(__LINE__);
    }
  }
  reader.close();

#ifdef CCUniqueRequests_DEBUG
  CDBDebug("/makeRequests");
#endif
}

size_t CURUniqueRequests::size() { return fileInfoMap.size(); }

CURFileInfo *CURUniqueRequests::get(size_t index) {
  typedef std::map<std::string, CURFileInfo *>::iterator it_type_file;
  size_t s = 0;
  for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
    if (s == index) return filemapiterator->second;
    s++;
  }
  return NULL;
}
