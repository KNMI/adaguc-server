#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include "CMakeJSONTimeSeries.h"
#include "CImageDataWriter.h"
#include "CURUniqueRequests.h"
#include "CDFObjectStore.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CImageWarper.h"
#include "CServerParams.h"
#include "CXMLParser.h"
#include "Types/CPointTypes.h"
#include "CDPPInterface.h"
#include "trajectorySampling.h"
#include "CDataPostProcessors/CDataPostProcessor.h"
#include "CDataPostProcessors/CDataPostProcessor_ConvertUnits.h"

static const bool enableLogsForCURUniqueRequests = false;

void nestRequest(std::map<std::string, CURDimInfo>::iterator diminfomapiterator, std::vector<CURAggregatedDimension> &aggregatedDimensions, CURFileInfo &fileInfo, int depth) {
  if (diminfomapiterator != fileInfo.dimInfoMap.end()) {
    auto currentIt = diminfomapiterator;
    auto currentDepth = depth;
    diminfomapiterator++;
    depth++;
    for (const auto &aggregatedValue: currentIt->second.aggregatedValues) {
      aggregatedDimensions[currentDepth] = {.name = currentIt->first, .start = aggregatedValue.start, .values = aggregatedValue.values};
      nestRequest(diminfomapiterator, aggregatedDimensions, fileInfo, depth);
    }
    return;
  } else {
    if (enableLogsForCURUniqueRequests) {
      CDBDebug("Add request with following:");
    }
    std::vector<CURAggregatedDimension> d;
    std::copy(aggregatedDimensions.begin(), aggregatedDimensions.begin() + depth, back_inserter(d));
    fileInfo.requests.push_back(d);
    return;
  }
}

void sortAndAggregate(std::map<std::string, CURFileInfo> &fileInfoMap) {
  for (auto &[_, fileInfo]: fileInfoMap) {
    for (auto &[dimName, dimInfo]: fileInfo.dimInfoMap) {
      int currentDimIndex = -1;
      int startDimIndex = 0;
      std::vector<std::string> dimValues;
      for (const auto &[dimindex, dimvalue]: dimInfo.dimValuesMap) {
        if (currentDimIndex != -1) {
          if (currentDimIndex == dimindex - 1) {
            currentDimIndex = dimindex;
          } else {
            //*** GO ***
            if (enableLogsForCURUniqueRequests) {
              CDBDebug("Print stop at %d", currentDimIndex);
            }
            currentDimIndex = -1;
            dimInfo.aggregatedValues.push_back({.start = startDimIndex, .values = dimValues});
          }
        }

        if (currentDimIndex == -1) {
          if (enableLogsForCURUniqueRequests) {
            CDBDebug("Print start at %d", dimindex);
          }
          currentDimIndex = dimindex;
          startDimIndex = dimindex;
          dimValues.clear();
        }

        if (currentDimIndex != -1) {
          if (enableLogsForCURUniqueRequests) {
            CDBDebug("Add %d/%s", dimindex, dimvalue.c_str());
          }

          dimValues.push_back(dimvalue);
        }
      }
      if (currentDimIndex != -1) {
        currentDimIndex = -1;
        dimInfo.aggregatedValues.push_back({.start = startDimIndex, .values = dimValues});
      }
    }
  }

  // Generate CURUniqueRequests

  for (auto &[_, fileInfo]: fileInfoMap) {
    std::vector<CURAggregatedDimension> aggregatedDimensions(fileInfo.dimInfoMap.size());
    nestRequest(fileInfo.dimInfoMap.begin(), aggregatedDimensions, fileInfo, 0);
  }
}

void recurDataStructure(CXMLParser::XMLElement *dataStructure, const CURResult *result, int depth, const std::vector<int> &dimOrdering, const std::vector<int> &dimIndicesToSkip) {
  int dimIndex = dimOrdering[depth];

  if (std::ranges::find(dimIndicesToSkip, depth) != dimIndicesToSkip.end()) {
    return recurDataStructure(dataStructure, result, depth + 1, dimOrdering, dimIndicesToSkip);
  }
  std::string dimIndexName = result->dimensionKeys[dimIndex].name;

  auto el = dataStructure->get(dimIndexName);
  if (el == nullptr) {
    dataStructure->add(CXMLParser::XMLElement(dimIndexName));
    el = dataStructure->getLast();
  }
  if (depth + 1 < result->numDims) {
    recurDataStructure(el, result, depth + 1, dimOrdering, dimIndicesToSkip);
  } else {
    el->setValue(result->value);
  }
}

void createStructure(const std::vector<CURResult> &results, DataObject *dataObject, CDataSource *dataSource, CXMLParser::XMLElement *gfiStructure, const std::vector<int> &dimOrdering,
                     const std::string &coordinates) {

  size_t numberOfDims = dataSource->requiredDims.size();

  CXMLParser::XMLElement layerStructure;
  layerStructure.name = "root";
  layerStructure.add(CXMLParser::XMLElement("name", dataObject->dataObjectName.empty() ? dataSource->getLayerName() : dataObject->dataObjectName));
  layerStructure.add(CXMLParser::XMLElement("layername", dataSource->getLayerName()));
  layerStructure.add(CXMLParser::XMLElement("variablename", dObjgetVariableName(*dataObject)));

  /* Add metadata */
  std::string standardName = dObjGetStdName(*dataObject);
  layerStructure.add(CXMLParser::XMLElement("standard_name", standardName));
  layerStructure.add(CXMLParser::XMLElement("units", dObjgetUnits(*dataObject)));

  CXMLParser::XMLElement point("point");
  point.add(CXMLParser::XMLElement("SRS", "EPSG:4326"));
  point.add(CXMLParser::XMLElement("coords", coordinates));
  layerStructure.add(point);

  // Sections need the actual hidden height in native JSON; Python omits hidden custom properties.
  std::vector<int> dimIndicesToSkip;
  for (size_t i = 0; i < numberOfDims; i++) {
    COGCDims &ogcDim = dataSource->requiredDims[dimOrdering[i]];
    if (ogcDim.hidden && !dataSource->srvParams->isTrajectory) {
      dimIndicesToSkip.push_back(i);
    } else {
      layerStructure.add(CXMLParser::XMLElement("dims", ogcDim.name));
    }
  }
  layerStructure.add(CXMLParser::XMLElement("data"));
  auto dataStructure = layerStructure.getLast();

  if (enableLogsForCURUniqueRequests) {
    CDBDebug("Found %lu elements", results.size());
  }

  for (const auto &result: results) {
    recurDataStructure(dataStructure, &result, 0, dimOrdering, dimIndicesToSkip);
  }
  gfiStructure->add(layerStructure);
}

void CURUniqueRequests::makeRequests(std::map<std::string, CURFileInfo> &fileInfoMap, CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY,
                                     CXMLParser::XMLElement *gfiStructure) {

  sortAndAggregate(fileInfoMap);
  if (enableLogsForCURUniqueRequests) {
    CDBDebug("======================== makeRequests ========================================");
  }
  CDataReader reader;
  int numberOfDataSourceDims = dataSource->requiredDims.size();

  /* Determine ordering of dimensions */
  std::vector<int> dimOrdering(numberOfDataSourceDims);
  std::iota(dimOrdering.begin(), dimOrdering.end(), 0);
  for (int dimnr = 0; dimnr < numberOfDataSourceDims; dimnr++) {
    if (CT::toLowerCase(dataSource->requiredDims[dimnr].name) == "reference_time" && dimnr != 0) {
      std::swap(dimOrdering[dimnr], dimOrdering[0]);
      break;
    }
  }

  for (int dimnr = 0; dimnr < numberOfDataSourceDims; dimnr++) {
    if (CT::toLowerCase(dataSource->requiredDims[dimOrdering[dimnr]].name) == "time" && dimnr != numberOfDataSourceDims - 1) {
      std::swap(dimOrdering[dimnr], dimOrdering[numberOfDataSourceDims - 1]);
      break;
    }
  }

  if (enableLogsForCURUniqueRequests) {
    for (int dimnr = 0; dimnr < numberOfDataSourceDims; dimnr++) {
      CDBDebug("New order = %d/%d = [%s]", dimnr, dimOrdering[dimnr], dataSource->requiredDims[dimOrdering[dimnr]].name.c_str());
    }
  }

  std::vector<CURResult> results;

  if (reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER) != 0) {
    if (dataSource->srvParams->isTrajectory) {
      CDBError("Could not read trajectory grid from %s", dataSource->getFileName().c_str());
      throw(__LINE__);
    }
    if (enableLogsForCURUniqueRequests) {
      CDBDebug("Could not read initial header from %s", dataSource->getFileName().c_str());
    }
  }

  if (enableLogsForCURUniqueRequests) {
    CDBDebug("===================== iterating data objects ==================");
  }
  const bool isTrajectory = dataSource->srvParams->isTrajectory;
  TrajectorySampling sampling;
  bool outsideGrid = false;
  std::string pointCoordinates;
  if (isTrajectory) {
    sampling = sampleTrajectoryGrid(dataSource, imageWarper);
    if (sampling.samples.empty()) return;
  } else {
    auto ckey = CT::printf("%d%d%s", dX, dY, dataSource->nativeProj4.c_str());
    const auto projCacheInfo = CImageDataWriter::GetProjInfo(ckey, drawImage, dataSource, imageWarper, dataSource->srvParams, dX, dY);
    outsideGrid = projCacheInfo.isOutsideBBOX;
    pointCoordinates = CT::printf("%f,%f", projCacheInfo.lonX, projCacheInfo.lonY);
    sampling.runs.push_back({projCacheInfo.imx, projCacheInfo.imy, 1});
  }
  for (size_t dataObjectNr = 0; dataObjectNr < dataSource->dataObjects.size(); dataObjectNr++) {
    const auto dataObject = dataSource->getDataObject(dataObjectNr);
    std::map<std::pair<int, int>, std::vector<CURResult>> spatialResults;
    std::string variableName = dataObject->cdfVariable->name;
    for (const auto &[dimName, fileInfo]: fileInfoMap) {
      if (!outsideGrid) {
        if (isTrajectory) prepareTrajectoryFile(reader, dataSource, fileInfo.representativeStep, sampling);
        CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource, dataSource->srvParams, dimName.c_str());

        if (enableLogsForCURUniqueRequests) {
          CDBDebug("Getting data variable [%s]", variableName.c_str());
        }

        CDF::Variable *variable = cdfObject->getVariableNE(variableName);
        dataObject->cdfVariable = variable;
        if (variable == NULL) {
          CDBError("Variable %s not found", variableName.c_str());
          throw(__LINE__);
        }

        // Hyperslab arrays follow the physical variable, not virtual/configured dimensions.
        const size_t numberOfDims = variable->dimensionlinks.size();
        std::vector<size_t> start(numberOfDims), count(numberOfDims);
        std::vector<ptrdiff_t> stride(numberOfDims);

        // Position has one cell run; trajectory adds unique contiguous row runs.
        for (const auto &run: sampling.runs) {
          for (const auto &dimensionRequest: fileInfo.requests) {

            if (enableLogsForCURUniqueRequests) {
              CDBDebug("Reading file %s  for variable %s", dimName.c_str(), variable->name.c_str());
            }

            // readData() otherwise reuses the previous array buffer, even for a different slice.
            variable->freeData();

            std::fill(start.begin(), start.end(), 0);
            std::fill(count.begin(), count.end(), 1);
            std::fill(stride.begin(), stride.end(), 1);
            if (enableLogsForCURUniqueRequests) {
              CDBDebug("Querying raster run x=%d y=%d count=%d", run.x, run.y, run.count);
              CDBDebug("Querying raster dimIndex %d %d", dataSource->dimXIndex, dataSource->dimYIndex);
            }
            if (dataSource->dimXIndex == -1 || dataSource->dimYIndex == -1) {
              CDBError("DataSource does not have x and y dimensions defined");
              throw(__LINE__);
            }
            start[(size_t)dataSource->dimXIndex] = run.x;
            start[(size_t)dataSource->dimYIndex] = run.y;
            count[(size_t)dataSource->dimXIndex] = run.count;

            for (const auto &request: dimensionRequest) {
              int netcdfDimIndex = -1;
              CDataReader::DimensionType dtype = CDataReader::getDimensionType(cdfObject, request.name);
              if (dtype != CDataReader::dtype_reference_time) {
                if (dtype == CDataReader::dtype_none) {
                  CDBWarning("dtype_none for %d with name %s", dtype, request.name.c_str());
                }

                netcdfDimIndex = variable->getDimIndex(request.name);
                if (netcdfDimIndex == -1) {
                  if (enableLogsForCURUniqueRequests) {
                    CDBDebug("Unable to find dimension [%s] for variable %s", request.name.c_str(), variable->name.c_str());
                  }

                  if (dtype == CDataReader::dtype_reference_time) {
                    if (enableLogsForCURUniqueRequests) {
                      CDBDebug("IS REFERENCE TIME %s", request.name.c_str());
                    }
                  } else if (variable->getParentCDFObject()->getDimensionNE(request.name) != nullptr) {
                    if (enableLogsForCURUniqueRequests) {
                      CDBDebug("IT IS IN THE DATAMODEL");
                    }

                  } else {
                    CDBError("Unable to find dimension [%s] for variable %s", request.name.c_str(), variable->name.c_str());
                    throw(__LINE__);
                  }
                }
                if (netcdfDimIndex == dataSource->dimXIndex || netcdfDimIndex == dataSource->dimYIndex) {
                  CDBWarning("netcdfDimIndex %d already taken for %s", netcdfDimIndex, request.name.c_str());
                }
                if (netcdfDimIndex != -1) {
                  start[netcdfDimIndex] = request.start;
                  count[netcdfDimIndex] = request.values.size();
                }
                if (enableLogsForCURUniqueRequests) {
                  CDBDebug("  request  netcdfdimindex %d  %s %d %lu", netcdfDimIndex, request.name.c_str(), request.start, request.values.size());
                }
              }
            }

            // Always read as double: packed data gets unpacked, and the conversions below work on doubles.
            variable->setType(CDF_DOUBLE);
            int status = variable->readData(CDF_DOUBLE, start.data(), count.data(), stride.data(), true);
            if (enableLogsForCURUniqueRequests) {
              CDBDebug("Read %lu elements", variable->getSize());
            }
            if (status != 0) {
              CDBError("Unable to read variable %s", variable->name.c_str());
              throw(__LINE__);
            }

            if (enableLogsForCURUniqueRequests) {
              for (size_t j = 0; j < variable->getSize(); j++) {
                CDBDebug("Orig Data value %lu is \t %f", j, ((double *)variable->data)[j]);
              }
            }
            /**
             * DataPostProc: Here our datapostprocessor comes into action!
             */
            for (const auto &proc: dataSource->cfgLayer->DataPostProc) {
              // Apply units:
              if (proc.attr.units.empty() == false) {
                dataObject->overruledUnits = proc.attr.units;
              }
            }
            // Unit conversions (ax+b, convert_units, ...) are applied to every read, independent of which file it came from.
            applyUnitConversions(dataSource->cfgLayer->DataPostProc, dataObject->hasNodataValue ? dataObject->dfNodataValue : NAN, (double *)variable->data, variable->getSize());
            getCDPPExecutor()->executeProcessors(dataSource, CDATAPOSTPROCESSOR_RUNAFTERREADING, (double *)variable->data, variable->getSize());
            /* End of data postproc */
            if (enableLogsForCURUniqueRequests) {
              CDBDebug("Read %lu elements", variable->getSize());

              for (size_t j = 0; j < variable->getSize(); j++) {
                CDBDebug("New Data value %lu is \t %f", j, ((double *)variable->data)[j]);
              }
            }
            try {
              std::vector<int> multiplies(variable->dimensionlinks.size());
              for (size_t d = 0; d < variable->dimensionlinks.size(); d += 1) {
                int m = 1;

                for (size_t j = 0; j < variable->dimensionlinks.size() - (d + 1); j += 1) {
                  int index = variable->dimensionlinks.size() - 1 - j;
                  m *= count[index];
                }
                multiplies[d] = m;
              }
              // Assign keys
              for (size_t indexInVariable = 0; indexInVariable < variable->getSize(); indexInVariable++) {
                CURResult currentResultForIndex;
                // currentResultForIndex.parent = this;
                currentResultForIndex.numDims = numberOfDataSourceDims;
                currentResultForIndex.dimensionKeys.resize(dataSource->requiredDims.size());

                // Fill in the dimension keys
                for (size_t dataSourceDimIndex = 0; dataSourceDimIndex < dataSource->requiredDims.size(); dataSourceDimIndex++) {
                  std::string requestDimNameToFind = dataSource->requiredDims[dataSourceDimIndex].netCDFDimName;
                  currentResultForIndex.dimensionKeys[dataSourceDimIndex].name = isTrajectory ? dataSource->getDimensionValue(dataSourceDimIndex) : dataSource->requiredDims[dataSourceDimIndex].value;
                  auto dimVariable = variable->getParentCDFObject()->getVariableNE(requestDimNameToFind);
                  if (dimVariable != nullptr) {
                    auto isTime = dataSource->requiredDims[dataSourceDimIndex].isATimeDimension;
                    currentResultForIndex.dimensionKeys[dataSourceDimIndex].cdfDimensionVariable = dimVariable;
                    currentResultForIndex.dimensionKeys[dataSourceDimIndex].isNumeric = isTime ? false : CDF::isCDFNumeric(dimVariable->getType());
                  }

                  const int variableDimIndex = variable->getDimIndex(requestDimNameToFind);
                  if (variableDimIndex != -1) {

                    const auto requestDim = std::ranges::find_if(dimensionRequest, [&](const auto &dimension) { return dimension.name == requestDimNameToFind; });
                    if (requestDim == dimensionRequest.end()) {
                      CDBError("Unable to find dimension %s in request", requestDimNameToFind.c_str());
                      throw(__LINE__);
                    }
                    const auto &values = requestDim->values;
                    size_t numValues = values.size();
                    size_t multiplyIndex = multiplies[variableDimIndex];
                    currentResultForIndex.dimensionKeys[dataSourceDimIndex].name = values[(indexInVariable / multiplyIndex) % numValues];
                  }
                }

                double pixelValueAtIndex = CImageDataWriter::convertValue(variable->getType(), variable->data, indexInVariable);
                std::string pixelValueAsString = "nodata";
                if ((pixelValueAtIndex != dataObject->dfNodataValue && dataObject->hasNodataValue == true && pixelValueAtIndex == pixelValueAtIndex) || dataObject->hasNodataValue == false) {
                  if (dataObject->hasStatusFlag) {
                    std::string flagMeaning = CDataSource::getFlagMeaningHumanReadable(dataObject->statusFlagList, pixelValueAtIndex);
                    pixelValueAsString = CT::printf("%s (%d)", flagMeaning.c_str(), (int)pixelValueAtIndex);
                  } else {
                    pixelValueAsString = CT::printf("%f", pixelValueAtIndex);
                  }
                }

                // Set the value
                currentResultForIndex.value = pixelValueAsString;
                currentResultForIndex.dimOrdering = &dimOrdering;
                if (isTrajectory) {
                  const int x = run.x + (indexInVariable / multiplies[dataSource->dimXIndex]) % run.count;
                  spatialResults[{x, run.y}].push_back(std::move(currentResultForIndex));
                } else {
                  results.push_back(std::move(currentResultForIndex));
                }
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
      if (enableLogsForCURUniqueRequests) {
        CDBDebug("**************** Create Structure ********************");
        CDBDebug("dataObjectNr: %lu", dataObjectNr);
      }
      if (isTrajectory) {
        std::map<std::pair<int, int>, size_t> structureIndices;
        for (const auto &sample: sampling.samples) {
          const auto key = std::pair{sample.cell.x, sample.cell.y};
          const auto [entry, firstVisit] = structureIndices.try_emplace(key, gfiStructure->xmlElements.size());
          if (firstVisit) {
            auto &cellResults = spatialResults.at(key);
            std::sort(cellResults.begin(), cellResults.end(), compareFunctionCurResult());
            const auto coordinates = CT::printf("%.15g,%.15g", sample.lonlat.x, sample.lonlat.y);
            createStructure(cellResults, dataObject, dataSource, gfiStructure, dimOrdering, coordinates);
          } else {
            // Revisits reuse the first structure; indices survive vector reallocation.
            gfiStructure->add(gfiStructure->xmlElements.at(entry->second));
          }
        }
      } else {
        std::sort(results.begin(), results.end(), compareFunctionCurResult());
        createStructure(results, dataObject, dataSource, gfiStructure, dimOrdering, pointCoordinates);
      }
    } catch (int e) {
      CDBError("Error in createStructure at line %d", e);
      throw(__LINE__);
    }
  }
  reader.close();

  if (enableLogsForCURUniqueRequests) {
    CDBDebug("/makeRequests");
  }
}
