#include <vector>
#include <algorithm>
#include "CMakeJSONTimeSeries.h"
#include "CImageDataWriter.h"
#include "CURUniqueRequests.h"

// #define CCUniqueRequests_DEBUG
// #define CCUniqueRequests_DEBUG_HIGH
const char *CURUniqueRequests::className = "CURUniqueRequests";

int *CURUniqueRequests::__getDimOrder() { return dimOrdering; }

void CURUniqueRequests::addDimensionRangeRequest(const char *filename, const char *dimName, size_t dimIndex, std::string dimValue) {
  fileInfoMap[filename].dimInfoMap[dimName].dimValuesMap[dimIndex] = dimValue;
}

void CURUniqueRequests::addDimSet(CURDimInfo &dimInfo, int start, std::vector<std::string> valueList) { dimInfo.aggregatedValues.push_back({.start = start, .values = valueList}); }

void CURUniqueRequests::nestRequest(it_type_diminfo diminfomapiterator, CURFileInfo &fileInfo, int depth) {
  if (diminfomapiterator != fileInfo.dimInfoMap.end()) {
    it_type_diminfo currentIt = diminfomapiterator;
    int currentDepth = depth;
    diminfomapiterator++;
    depth++;
    for (size_t j = 0; j < (currentIt->second).aggregatedValues.size(); j++) {
      auto *aggregatedValue = &(currentIt->second).aggregatedValues[j];
      aggregatedDimensions[currentDepth] = {.name = (currentIt->first).c_str(), .start = aggregatedValue->start, .values = aggregatedValue->values};
      nestRequest(diminfomapiterator, fileInfo, depth);
    }
    return;
  } else {
#ifdef CCUniqueRequests_DEBUG_HIGH
    CDBDebug("Add request with following:");
#endif
    std::vector<CURAggregatedDimension> d;
    for (int j = 0; j < depth; j++) {
      d.push_back(aggregatedDimensions[j]);
    }
    fileInfo.requests.push_back(d);
    return;
  }
}

void CURUniqueRequests::sortAndAggregate() {
  for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
    auto dimInfoMap = &(filemapiterator->second).dimInfoMap;
    for (it_type_diminfo diminfomapiterator = dimInfoMap->begin(); diminfomapiterator != dimInfoMap->end(); diminfomapiterator++) {
#ifdef CCUniqueRequests_DEBUG
      CDBDebug("%s/%s", (filemapiterator->first).c_str(), (diminfomapiterator->first).c_str());
#endif

      int currentDimIndex = -1;
      int dimindex;

      int startDimIndex;
      std::vector<std::string> dimValues;
      map_type_dimvalindex dimValuesMap = diminfomapiterator->second.dimValuesMap;
      for (it_type_dimvalindex dimvalindexmapiterator = dimValuesMap.begin(); dimvalindexmapiterator != dimValuesMap.end(); dimvalindexmapiterator++) {
        dimindex = dimvalindexmapiterator->first;
        const char *dimvalue = dimvalindexmapiterator->second.c_str();

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
    nestRequest((filemapiterator->second).dimInfoMap.begin(), filemapiterator->second, 0);
  }
}

void CURUniqueRequests::recurDataStructure(CXMLParser::XMLElement *dataStructure, CURResult *result, int depth, int *dimOrdering, std::vector<int> dimIndicesToSkip) {
  int dimIndex = dimOrdering[depth];

  for (const int &dimIndexToSkip : dimIndicesToSkip) {
    if (depth == dimIndexToSkip) {
      return recurDataStructure(dataStructure, result, depth + 1, dimOrdering, dimIndicesToSkip);
    }
  }
  std::string dimIndexName = result->dimensionKeys[dimIndex].name;

  CXMLParser::XMLElement *el = NULL;
  try {
    el = dataStructure->get(dimIndexName.c_str());
  } catch (int e) {
    dataStructure->add(CXMLParser::XMLElement(dimIndexName.c_str()));
    el = dataStructure->getLast();
  }
  if (depth + 1 < result->numDims) {
    recurDataStructure(el, result, depth + 1, dimOrdering, dimIndicesToSkip);
  } else {
    el->setValue(result->value.c_str());
  }
}

void CURUniqueRequests::createStructure(std::vector<CURResult> results, CDataSource::DataObject *dataObject, CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY,
                                        CXMLParser::XMLElement *gfiStructure) {

  int numberOfDims = dataSource->requiredDims.size();

  CXMLParser::XMLElement *layerStructure = gfiStructure->add("root");
  layerStructure->add(CXMLParser::XMLElement("name", dataSource->getLayerName()));

  /* Add metadata */
  std::string standardName = dataObject->variableName.c_str();
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
      layerStructure->add(CXMLParser::XMLElement("dims", dataSource->requiredDims[dimOrdering[i]]->name.c_str()));
    }
  }

  CXMLParser::XMLElement *dataStructure = NULL;
  try {
    dataStructure = layerStructure->get("data");
  } catch (int e) {
    layerStructure->add(CXMLParser::XMLElement("data"));
    dataStructure = layerStructure->getLast();
  }

  std::sort(results.begin(), results.end(), compareFunctionCurResult());
  CDBDebug("Found %d elements", results.size());

  for (size_t j = 0; j < results.size(); j++) {
    recurDataStructure(dataStructure, &results[j], 0, dimOrdering, dimIndicesToSkip);
  }
}

void CURUniqueRequests::makeRequests(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure) {
  sortAndAggregate();
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
  std::string dimName[numberOfDims];

  std::vector<CURResult> results;

  reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);

#ifdef CCUniqueRequests_DEBUG_HIGH
  CDBDebug("===================== iterating data objects ==================");
#endif
  for (size_t dataObjectNr = 0; dataObjectNr < dataSource->dataObjects.size(); dataObjectNr++) {

    CDataSource::DataObject *dataObject = dataSource->getDataObject(dataObjectNr);
    std::string variableName = dataObject->cdfVariable->name.c_str();
    // Show all requests

    for (it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
      CT::string ckey;
      ckey.print("%d%d%s", dX, dY, dataSource->nativeProj4.c_str());
      CImageDataWriter::ProjCacheInfo projCacheInfo = CImageDataWriter::GetProjInfo(ckey, drawImage, dataSource, imageWarper, dataSource->srvParams, dX, dY);

      if (projCacheInfo.isOutsideBBOX == false) {
        CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource, dataSource->srvParams, (filemapiterator->first).c_str());

#ifdef CCUniqueRequests_DEBUG
        CDBDebug("Getting data variable [%s]", variableName.c_str());
#endif

        CDF::Variable *variable = cdfObject->getVariableNE(variableName.c_str());
        dataObject->cdfVariable = variable;
        if (variable == NULL) {
          CDBError("Variable %s not found", variableName.c_str());
          throw(__LINE__);
        }

        for (auto request : (filemapiterator->second).requests) {

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

          for (size_t i = 0; i < request.size(); i++) {
            int netcdfDimIndex = -1;
            CDataReader::DimensionType dtype = CDataReader::getDimensionType(cdfObject, request[i].name.c_str());
            if (dtype != CDataReader::dtype_reference_time) {
              if (dtype == CDataReader::dtype_none) {
                CDBWarning("dtype_none for %s", dtype, request[i].name.c_str());
              }
              try {
                /* CHECK */

                netcdfDimIndex = variable->getDimensionIndex(request[i].name.c_str());
              } catch (int e) {
                // CDBError("Unable to find dimension [%s]",request[i]->name.c_str());
                if (dtype == CDataReader::dtype_reference_time) {
                  CDBDebug("IS REFERENCE TIME %s", request[i].name.c_str());

                } else {
                  CDBError("IS NOT REFERENCE TIME %s", request[i].name.c_str());
                  throw(__LINE__);
                }
              }
              if (netcdfDimIndex == dataSource->dimXIndex || netcdfDimIndex == dataSource->dimYIndex) {
                CDBWarning("netcdfDimIndex %d already taken for %s", netcdfDimIndex, request[i].name.c_str());
              }
              start[netcdfDimIndex] = request[i].start;
              count[netcdfDimIndex] = request[i].values.size();
              dimName[netcdfDimIndex] = request[i].name;
#ifdef CCUniqueRequests_DEBUG
              CDBDebug("  request index: %d  netcdfdimindex %d  %s %d %d", i, netcdfDimIndex, request[i]->name.c_str(), request[i]->start, request[i]->values.size());
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
                  std::string requestDimNameToFind = dataSource->requiredDims[dataSourceDimIndex]->netCDFDimName.c_str();
                  currentResultForIndex.dimensionKeys[dataSourceDimIndex].name = dataSource->requiredDims[dataSourceDimIndex]->value;
                  auto dimVariable = variable->getParentCDFObject()->getVariableNE(requestDimNameToFind.c_str());
                  auto isTime = dataSource->requiredDims[dataSourceDimIndex]->isATimeDimension;
                  auto varType = dimVariable->getType();
                  currentResultForIndex.dimensionKeys[dataSourceDimIndex].cdfDimensionVariable = dimVariable;
                  currentResultForIndex.dimensionKeys[dataSourceDimIndex].isNumeric = isTime ? false : CDF::isCDFNumeric(varType);

                  int variableDimIndex = -1;
                  for (size_t d = 0; d < variable->dimensionlinks.size() - 2; d += 1) {
                    if (variable->dimensionlinks[d]->name.equals(requestDimNameToFind)) {
                      variableDimIndex = d;
                    }
                  }
                  if (variableDimIndex != -1) {

                    int requestDimIndex = -1;
                    for (size_t i = 0; i < request.size(); i++) {
                      if (request[i].name.equals(requestDimNameToFind.c_str())) {
                        requestDimIndex = i;
                      }
                    }
                    if (requestDimIndex == -1) {
                      CDBError("Unable to find dimension %s in request", requestDimNameToFind.c_str());
                      throw(__LINE__);
                    }
                    auto values = request[requestDimIndex].values;
                    size_t numValues = values.size();
                    size_t multiplyIndex = multiplies[variableDimIndex];
                    currentResultForIndex.dimensionKeys[dataSourceDimIndex].name = values[(indexInVariable / multiplyIndex) % numValues];
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
      createStructure(results, dataObject, drawImage, imageWarper, dataSource, dX, dY, gfiStructure);
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
