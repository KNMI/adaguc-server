#include "CCreateTiles.h"
#include "CDBAdapter.h"
#include "CDBFactory.h"
#include "CReporter.h"
#include "CRequest.h"
#include "CNetCDFDataWriter.h"

const char *CCreateTiles::className = "CCreateTiles";

int CCreateTiles::createTiles(CDataSource *dataSource, int scanFlags) {
  if (dataSource->isConfigured == false) {
    CDBError("Error! dataSource->isConfigured == false");
    return 1;
  }

  if (dataSource->srvParams->configObj->Configuration.size() == 0) {
    CDBError("Error! dataSource->srvParams->configObj->Configuration.size() == 0");
    return 1;
  }

  if (dataSource->cfgLayer->TileSettings.size() != 1) {
    CDBDebug("TileSettings is not set for this layer");
    return 0;
  }
  if (dataSource->cfgLayer->Dimension.size() == 0) {
    if (CAutoConfigure::autoConfigureDimensions(dataSource) != 0) {
      CREPORT_ERROR_NODOC("Unable to configure dimensions automatically", CReportMessage::Categories::GENERAL);
      return 1;
    }
  }
  /* Find all files on disk */
  std::vector<std::string> fileList;
  CT::string filter = dataSource->cfgLayer->FilePath[0]->attr.filter.c_str();
  CT::string tailPath;
  if (scanFlags & CDBFILESCANNER_IGNOREFILTER) {
    filter = "^.*$";
  }
  try {
    fileList = CDBFileScanner::searchFileNames(dataSource->cfgLayer->FilePath[0]->value.c_str(), filter.c_str(), tailPath.c_str());
  } catch (int linenr) {
    CDBError("Exception in searchFileNames [%s] [%s]", dataSource->cfgLayer->FilePath[0]->value.c_str(), filter.c_str(), tailPath.c_str());
    return 1;
  }
  if (fileList.size() == 0) {
    CDBError("No files found");
    return 1;
  }

  CDBDebug("Found %d files", fileList.size());
  for (size_t j = 0; j < fileList.size(); j++) {
    CCreateTiles::createTilesForFile(dataSource, scanFlags, fileList[j].c_str());
  }

  return 0;
}

int CCreateTiles::createTilesForFile(CDataSource *dataSource, int, CT::string fileToTile) {
  if (dataSource->isConfigured == false) {
    CDBError("Error! dataSource->isConfigured == false");
    return 1;
  }

  if (dataSource->srvParams->configObj->Configuration.size() == 0) {
    CDBError("Error! dataSource->srvParams->configObj->Configuration.size() == 0");
    return 1;
  }

  if (dataSource->cfgLayer->TileSettings.size() != 1) {
    CDBDebug("TileSettings is not set for this layer");
    return 0;
  }
  CDBAdapter *dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  if (dataSource->cfgLayer->Dimension.size() == 0) {
    if (CAutoConfigure::autoConfigureDimensions(dataSource) != 0) {
      CREPORT_ERROR_NODOC("Unable to configure dimensions automatically", CReportMessage::Categories::GENERAL);
      return 1;
    }
  }

  CDBDebug("*** Starting createTiles for layer '%s' ***", dataSource->cfgLayer->Name[0]->value.c_str());

  CDataReader *reader = new CDataReader();
  ;
  reader->enableReporting(false);
  dataSource->addStep(fileToTile.c_str(), NULL);
  reader->open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);
  delete reader;
#ifdef CDBFILESCANNER_DEBUG
  CDBDebug("CRS:  [%s]", dataSource->nativeProj4.c_str());
  CDBDebug("BBOX: [%f %f %f %f]", dataSource->dfBBOX[0], dataSource->dfBBOX[1], dataSource->dfBBOX[2], dataSource->dfBBOX[3]);
#endif

  CServerConfig::XMLE_TileSettings *ts = dataSource->cfgLayer->TileSettings[0];

  if (ts->attr.readonly.equals("true")) {
    CDBDebug("Skipping create tiles for layer %s, because readonly in "
             "TileSettings is set to true",
             dataSource->getLayerName());
    return 0;
  };
  int configErrors = 0;
  if (ts->attr.tilewidthpx.empty()) {
    CDBError("tilewidthpx not defined");
    configErrors++;
  }
  if (ts->attr.tileheightpx.empty()) {
    CDBError("tileheightpx not defined");
    configErrors++;
  }

  bool autoTileExtent = false;
  if (ts->attr.left.empty() && ts->attr.bottom.empty() && ts->attr.right.empty() && ts->attr.top.empty()) {
    autoTileExtent = true;
  } else {
    if (ts->attr.left.empty()) {
      CDBError("left not defined");
      configErrors++;
    }
    if (ts->attr.bottom.empty()) {
      CDBError("bottom not defined");
      configErrors++;
    }
    if (ts->attr.right.empty()) {
      CDBError("right not defined");
      configErrors++;
    }
    if (ts->attr.top.empty()) {
      CDBError("top not defined");
      configErrors++;
    }
  }

  if (configErrors != 0) {
    return 1;
  }

  CT::string tilemode = ts->attr.tilemode;

  int tilewidthpx = ts->attr.tilewidthpx.toInt();
  int tileheightpx = ts->attr.tileheightpx.toInt();

  double tilecellsizex = dataSource->dfCellSizeX;
  double tilecellsizey = dataSource->dfCellSizeY;

  if (!ts->attr.tilecellsizex.empty()) {
    ts->attr.tilecellsizex.toDouble();
  }
  if (!ts->attr.tilecellsizey.empty()) {
    ts->attr.tilecellsizey.toDouble();
  }

  CT::string tileprojection = dataSource->nativeProj4;

  if (ts->attr.tileprojection.empty() == false) {
    tileprojection = ts->attr.tileprojection;
  }

  double tilesetstartx = ts->attr.left.toDouble();
  double tilesetstarty = ts->attr.bottom.toDouble();
  double tilesetstopx = ts->attr.right.toDouble();
  double tilesetstopy = ts->attr.top.toDouble();

  if (autoTileExtent == true) {
    CImageWarper warper;
    CGeoParams sourceGeo;
    sourceGeo.CRS = tileprojection;

    int status = warper.initreproj(dataSource, &sourceGeo, &dataSource->srvParams->cfg->Projection);
    if (status != 0) {
      CREPORT_ERROR_NODOC("Unable to initialize projection", CReportMessage::Categories::GENERAL);
      return 1;
    }

    if (warper.isProjectionRequired()) {
      double dfBBOX[4];
      warper.findExtent(dataSource, dfBBOX);

      CDBDebug("Found TILEEXTENT BBOX [%f, %f, %f, %f]", dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]);
      tilesetstartx = dfBBOX[0];
      tilesetstarty = dfBBOX[1];
      tilesetstopx = dfBBOX[2];
      tilesetstopy = dfBBOX[3];
    } else {
      tilesetstartx = dataSource->dfBBOX[0];
      tilesetstarty = dataSource->dfBBOX[1];
      tilesetstopx = dataSource->dfBBOX[2];
      tilesetstopy = dataSource->dfBBOX[3];
    }
  }

  double tileBBOXWidth = fabs(tilecellsizex * double(tilewidthpx));
  double tileBBOXHeight = fabs(tilecellsizey * double(tileheightpx));

#ifdef CDBFILESCANNER_DEBUG
  CDBDebug("tilecellsizexy     : [%f,%f]", tilecellsizex, tilecellsizey);
  CDBDebug("tileBBOXWH         : [%f,%f]", tileBBOXWidth, tileBBOXHeight);
  CDBDebug("tileWH             : [%d,%d]", tilewidthpx, tileheightpx);
#endif

  CT::string tableName;

  if (CAutoConfigure::autoConfigureDimensions(dataSource) != 0) {
    CREPORT_ERROR_NODOC("Unable to configure dimensions automatically", CReportMessage::Categories::GENERAL);
    return 1;
  }

  try {
    CRequest::fillDimValuesForDataSource(dataSource, dataSource->srvParams);
  } catch (ServiceExceptionCode e) {
    CDBError("Exception in setDimValuesForDataSource");
  }

  dataSource->queryLevel = 0;
  dataSource->queryBBOX = 1;

  dataSource->nativeViewPortBBOX.left = -1000000000;
  dataSource->nativeViewPortBBOX.bottom = -1000000000;
  dataSource->nativeViewPortBBOX.right = 1000000000;
  ;
  dataSource->nativeViewPortBBOX.top = 1000000000;
  ;

  if (dataSource->requiredDims[0]->name.equals("time")) {
    if (fileToTile.equals("*")) {
      dataSource->requiredDims[0]->value = "*";
    } else {
      CT::string tableName =
          dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), "time", dataSource);
      CT::string value = dbAdapter->getDimValueForFileName(fileToTile.c_str(), tableName.c_str());
      dataSource->requiredDims[0]->value = value;
    }
    CDBDebug("dataSource->requiredDims value %s", dataSource->requiredDims[0]->value.c_str());

    CDBStore::Store *store = dbAdapter->getFilesAndIndicesForDimensions(dataSource, 3000, false);
    if (store == NULL) {
      CDBError("Unable to get results");
      return 1;
    }
    for (size_t k = 0; k < store->getSize(); k++) {
      CDBStore::Record *record = store->getRecord(k);
      dataSource->addStep(record->get(0)->c_str(), NULL);
      for (size_t i = 0; i < dataSource->requiredDims.size(); i++) {
        CT::string value = record->get(1 + i * 2)->c_str();
        dataSource->getCDFDims()->addDimension(dataSource->requiredDims[i]->netCDFDimName.c_str(), value.c_str(), atoi(record->get(2 + i * 2)->c_str()));
        dataSource->requiredDims[i]->uniqueValues.clear();
        dataSource->requiredDims[i]->addValue(value.c_str()); //,atoi(record->get(2+i*2)->c_str()));
      }
    }
    delete store;
    store = NULL;
  } else {
    int status = CRequest::queryDimValuesForDataSource(dataSource, dataSource->srvParams);
    if (status != 0) return status;
  }

  for (size_t j = 0; j < dataSource->requiredDims[0]->uniqueValues.size(); j++) {
    CDBDebug("Found %s", dataSource->requiredDims[0]->uniqueValues[j].c_str());
  }

  for (size_t dd = 0; dd < dataSource->requiredDims.size(); dd++) {
    for (size_t ddd = 0; ddd < dataSource->requiredDims[dd]->uniqueValues.size(); ddd++) {
      dataSource->requiredDims[dd]->value = dataSource->requiredDims[dd]->uniqueValues[ddd].c_str();
      try {

        CT::string dimName = dataSource->cfgLayer->Dimension[dd]->attr.name.c_str();
        tableName =
            dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str(), dataSource);

        double globalBBOX[4];

        CDBStore::Store *store = NULL;

        globalBBOX[0] = tilesetstartx;
        globalBBOX[2] = tilesetstopx;
        globalBBOX[1] = tilesetstarty;
        globalBBOX[3] = tilesetstopy;

        double tilesetWidth = globalBBOX[2] - globalBBOX[0];
        double tilesetHeight = globalBBOX[3] - globalBBOX[1];
        int nrTilesX = floor(tilesetWidth / tileBBOXWidth + 0.5);
        int nrTilesY = floor(tilesetHeight / tileBBOXHeight + 0.5);

        int minlevel = 1;
        if (ts->attr.minlevel.empty() == false) {
          minlevel = ts->attr.minlevel.toInt();
          if (minlevel <= 1) minlevel = 1;
        }
        int maxlevel = ts->attr.maxlevel.toInt();
        for (int level = maxlevel; level >= minlevel; level--) {

          int numFound = 0;
          int numCreated = 0;

          int levelInc = pow(2, level - 1);
          for (int y = 0; y < nrTilesY; y = y + levelInc) {
            for (int x = 0; x < nrTilesX; x = x + levelInc) {
              int tilesCreated = 0;
              CDataSource ds;
              CServerParams newSrvParams;
              newSrvParams.cfg = dataSource->srvParams->cfg;
              ds.srvParams = &newSrvParams;
              ds.cfgLayer = dataSource->cfgLayer;
              ds.cfg = dataSource->srvParams->cfg;

              double dfMinX = globalBBOX[0] + tileBBOXWidth * x - tileBBOXWidth / 2;
              double dfMinY = globalBBOX[1] + tileBBOXHeight * y - tileBBOXHeight / 2;
              double dfMaxX = globalBBOX[0] + tileBBOXWidth * (x + levelInc) + tileBBOXWidth / 2;
              double dfMaxY = globalBBOX[1] + tileBBOXHeight * (y + levelInc) + tileBBOXHeight / 2;
              try {
                CRequest::fillDimValuesForDataSource(&ds, dataSource->srvParams);
              } catch (ServiceExceptionCode e) {
                CDBError("Exception in setDimValuesForDataSource");
                return 1;
              }
              ds.requiredDims[dd]->value = dataSource->requiredDims[dd]->value;

              CT::string tileBasePath = ts->attr.tilepath.c_str();
              CT::string prefix = "";
              if (ts->attr.prefix.empty() == false) {
                prefix = ts->attr.prefix + "/";
              }
              CT::string timeValue = "1970-01-01T00:00:00Z";
              for (size_t i = 0; i < ds.requiredDims.size(); i++) {
                if (ds.requiredDims[i]->isATimeDimension) {
                  timeValue = ds.requiredDims[i]->value;
                  timeValue.replaceSelf(":", "");
                  timeValue.replaceSelf("-", "");
                }
              }

              tileBasePath.printconcat("/%s/%s/%slevel%0.2d/", tableName.c_str(), timeValue.c_str(), prefix.c_str(), level);

              /* Now make the fileNameToWrite */
              CT::string fileNameToWrite = tileBasePath;
              fileNameToWrite.printconcat("level[%d]row[%d]_col[%d]", level, x, y);
              for (size_t i = 0; i < ds.requiredDims.size(); i++) {
                fileNameToWrite.printconcat("_%s", ds.requiredDims[i]->value.c_str());
              }
              fileNameToWrite.concat(".nc");
              fileNameToWrite = CDirReader::makeCleanPath(fileNameToWrite.c_str());

              /* Check if the tile was already done */
              bool isFileInTable = false;

              int status = dbAdapter->checkIfFileIsInTable(tableName.c_str(), fileNameToWrite.c_str());
              if (status == 0) {
                isFileInTable = true;
              }

              if (!isFileInTable) {
                CDirReader::makePublicDirectory(tileBasePath.c_str());
                ds.srvParams->Geo->CRS = tileprojection;

                ds.srvParams->Geo->dfBBOX[0] = dfMinX;
                ds.srvParams->Geo->dfBBOX[1] = dfMinY;
                ds.srvParams->Geo->dfBBOX[2] = dfMaxX;
                ds.srvParams->Geo->dfBBOX[3] = dfMaxY;

                ds.nativeViewPortBBOX.left = dfMinX;
                ds.nativeViewPortBBOX.bottom = dfMinY;
                ds.nativeViewPortBBOX.right = dfMaxX;
                ds.nativeViewPortBBOX.top = dfMaxY;

                try {

                  bool baseOnRootDataFile = true;

                  if (baseOnRootDataFile) {
                    ds.queryBBOX = 0;
                    ds.queryLevel = 1; // TODO MAKE CONFIGURABLE
                    store = dbAdapter->getFilesAndIndicesForDimensions(dataSource, 1, false);
                  } else {
                    ds.queryBBOX = 1;
                    ds.queryLevel = level - 1;
                    store = dbAdapter->getFilesAndIndicesForDimensions(&ds, 3000, false);
                  }

                  if (store != NULL) {
#ifdef CDBFILESCANNER_DEBUG
                    CDBDebug("Store size = %d for level %d", store->getSize(), level);
#endif
                    if (level == minlevel && store->getSize() == 0) {
                      delete store;

#ifdef CDBFILESCANNER_DEBUG
                      CDBDebug("Finding root, dataSource->requiredDims.size() = %d", dataSource->requiredDims.size());
#endif

                      store = dbAdapter->getFilesAndIndicesForDimensions(dataSource, 1, false);
                    }
                    if (store == NULL) {
                      CREPORT_ERROR_NODOC("Found no data!", CReportMessage::Categories::GENERAL);
                      return 1;
                    }
#ifdef CDBFILESCANNER_DEBUG
                    CDBDebug("*** Found %d %d:%d / %d:%d= (%f %f %f %f) - (%f,%f)", store->getSize(), x, y, int(nrTilesX), int(nrTilesY), dfMinX, dfMinY, dfMaxX, dfMaxY, dfMaxX - dfMinX,
                             dfMaxY - dfMinY);
#endif
                    if (store->getSize() > 0) {
                      for (size_t k = 0; k < store->getSize(); k++) {
                        CDBStore::Record *record = store->getRecord(k);
#ifdef CDBFILESCANNER_DEBUG
                        CDBDebug("Adding %s", record->get(0)->c_str());
#endif
                        ds.addStep(record->get(0)->c_str(), NULL);
                        for (size_t i = 0; i < ds.requiredDims.size(); i++) {
#ifdef CDBFILESCANNER_DEBUG
                          CDBDebug("%s", ds.requiredDims[i]->netCDFDimName.c_str());
#endif
                          CT::string value = record->get(1 + i * 2)->c_str();
                          ds.getCDFDims()->addDimension(ds.requiredDims[i]->netCDFDimName.c_str(), value.c_str(), atoi(record->get(2 + i * 2)->c_str()));
                          ds.requiredDims[i]->addValue(value.c_str());
                        }
                      }
                      int status = 0;

                      CNetCDFDataWriter *wcsWriter = new CNetCDFDataWriter();
                      if (tilemode.equals("avg_rgba")) {
                        wcsWriter->setInterpolationMode(CNetCDFDataWriter_AVG_RGB);
                      }
                      try {
                        newSrvParams.Geo->dWidth = (tilewidthpx);
                        newSrvParams.Geo->dHeight = (tileheightpx);
                        dfMinX = globalBBOX[0] + tileBBOXWidth * x;
                        dfMinY = globalBBOX[1] + tileBBOXHeight * y;
                        dfMaxX = globalBBOX[0] + tileBBOXWidth * (x + levelInc);
                        dfMaxY = globalBBOX[1] + tileBBOXHeight * (y + levelInc);

                        newSrvParams.Geo->dfBBOX[0] = dfMinX;
                        newSrvParams.Geo->dfBBOX[1] = dfMinY;
                        newSrvParams.Geo->dfBBOX[2] = dfMaxX;
                        newSrvParams.Geo->dfBBOX[3] = dfMaxY;
                        newSrvParams.Format = "adagucnetcdf";
                        newSrvParams.WCS_GoNative = 0;
                        newSrvParams.Geo->CRS.copy(&tileprojection);
                        int ErrorAtLine = 0;
                        try {
                          int layerNo = dataSource->datasourceIndex;
                          if (ds.setCFGLayer(dataSource->srvParams, dataSource->srvParams->configObj->Configuration[0], dataSource->srvParams->cfg->Layer[layerNo], NULL, layerNo) != 0) {
                            return 1;
                          }

#ifdef CDBFILESCANNER_DEBUG
                          CDBDebug("Checking tile for [%f,%f,%f,%f] with WH [%d,%d]", dfMinX, dfMinY, dfMaxX, dfMaxY, newSrvParams.Geo->dWidth, newSrvParams.Geo->dHeight);
                          CDBDebug("wcswriter init");
#endif
                          status = wcsWriter->init(&newSrvParams, &ds, ds.getNumTimeSteps());
                          if (status != 0) {
                            throw(__LINE__);
                          };
                          std::vector<CDataSource *> dataSources;
                          ds.varX->freeData();
                          ds.varY->freeData();

                          dataSources.push_back(&ds);
                          int numFailedWarps = 0;
                          int numDoneWarps = 0;

                          for (size_t k = 0; k < (size_t)dataSources[0]->getNumTimeSteps(); k++) {
                            for (size_t d = 0; d < dataSources.size(); d++) {
                              dataSources[d]->setTimeStep(k);
                            }
#ifdef CDBFILESCANNER_DEBUG
                            CDBDebug("wcswriter adddata");
#endif
                            numDoneWarps++;
                            status = wcsWriter->addData(dataSources);
                            if (status != 0) {
                              numFailedWarps++;
                            };
#ifdef CDBFILESCANNER_DEBUG
                            CDBDebug("wcswriter adddata done");
#endif
                          }

                          if (numDoneWarps != numFailedWarps) {
                            status = wcsWriter->writeFile(fileNameToWrite.c_str(), level, false);
                            if (status != 0) {
                              throw(__LINE__);
                            };
                            CDBDebug("Starting updatedb from createTiles for "
                                     "file %s",
                                     fileNameToWrite.c_str());
                            int r = 0;
                            std::vector<std::string> fileList;
                            fileList.push_back(fileNameToWrite.c_str());
                            status = CDBFileScanner::DBLoopFiles(dataSource, r, &fileList,
                                                                 CDBFILESCANNER_DONTREMOVEDATAFROMDB | CDBFILESCANNER_UPDATEDB | CDBFILESCANNER_IGNOREFILTER | CDBFILESCANNER_DONOTTILE);
                            if (status != 0) throw(__LINE__);
                            tilesCreated++;
                          } else {
                            status = 0;
                          }

                          if (!baseOnRootDataFile) {
                            for (size_t d = 0; d < dataSources.size(); d++) {
                              for (size_t k = 0; k < (size_t)dataSources[d]->getNumTimeSteps(); k++) {
                                dataSources[d]->setTimeStep(k);
                                if (dataSources[d]->queryLevel != 0) {
                                  /* Remove from the cache the datasources we
                                   * just read from  */
                                  CDFObjectStore::getCDFObjectStore()->deleteCDFObject(dataSources[d]->getFileName());
                                }
                              }
                            }
                          }
                          /* Remove the datasource from the cache we just wrote
                           */
                          // TODO: Removing this cdfobject causes memory violations. Checkout in issue https://github.com/KNMI/adaguc-server/issues/289
                          // CDFObjectStore::getCDFObjectStore()->deleteCDFObject(fileNameToWrite.c_str());

#ifdef CDBFILESCANNER_DEBUG
                          CDBDebug("DONE: Open CDFObjects: [%d]", CDFObjectStore::getCDFObjectStore()->getNumberOfOpenObjects());
#endif
                        } catch (int e) {
                          CDBError("Error at line %d", e);
                          ErrorAtLine = e;
                        }
                        newSrvParams.cfg = NULL;

                        if (ErrorAtLine != 0) throw(ErrorAtLine);
                      } catch (int e) {
                        CDBError("Exception at line %d", e);
                        status = 1;
                      }
                      delete wcsWriter;

                      if (status != 0) {
                        CDBError("Status is nonzero");
                        delete store;
                        store = NULL;
                        return 1;
                      }
                    }
                    numFound += store->getSize();
                    if (store->getSize() > 0) {
                      numCreated++;
                    }
                  }
                  delete store;
                  store = NULL;
                } catch (ServiceExceptionCode e) {
                  CDBError("Catched ServiceExceptionCode %d at line %d", e, __LINE__);
                } catch (int e) {
                  CDBError("Catched integerException %d at line %d", e, __LINE__);
                }

                delete store;
                store = NULL;

                if (tilesCreated > 0) {

                  CDBDebug("**** Percentage done for level: %.1f. Scanning tile "
                           "dimnr %d/%d dimval %d/%d: Level %d/%d,  ***",
                           (float(x + y * nrTilesX) / float(nrTilesY * nrTilesX)) * 100, dd, dataSource->requiredDims.size(), ddd, dataSource->requiredDims[dd]->uniqueValues.size(), level, maxlevel);
                }
              }
            }
          }
          if (numCreated != 0) {
            CDBDebug("Level %d Found %d file, created %d tiles", level, numFound, numCreated);
          }
        }
      } catch (int e) {

        CDBError("Unable to create tableName from '%s' '%s' ", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str());
        return 1;
      }
    }
  }
  return 0;
};
