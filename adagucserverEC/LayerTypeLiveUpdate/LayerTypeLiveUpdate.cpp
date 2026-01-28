#include "LayerTypeLiveUpdate.h"
#include "CServerParams.h"
#include "CDataPostProcessors/CDataPostProcessor.h"

int layerTypeLiveUpdateConfigureDimensionsInDataSource(CDataSource *dataSource) {
  // This layer has no dimensions, but we need to add one timestep with data in order to make the next code work.
  if (dataSource->requiredDims.size() < 1) {

    COGCDims *requiredDim = new COGCDims();
    requiredDim->isATimeDimension = true;
    requiredDim->name = "time";
    requiredDim->netCDFDimName = "time";
    // The following values need to be filled in and are updated during XML GetCapabilities generation
    requiredDim->uniqueValues.push_back("2020-01-01T00:00:00Z");
    requiredDim->uniqueValues.push_back("2020-01-02T00:00:00Z");
    requiredDim->value = "2020-01-02T00:00:00Z";
    dataSource->requiredDims.push_back(requiredDim);
  }
  // Basic case of liveupdate layer
  if (dataSource->cfgLayer->DataPostProc.empty()) {
    // // Add step to empty file
    dataSource->addStep("");
    dataSource->getCDFDims()->addDimension("none", "0", 0);
  } else {
    // Case of liveupdate layers with data post processors (such as solar terminator)
    // Currently one (dummy) file is required
    std::vector<std::string> fileList;
    if (!dataSource->cfgLayer->FilePath.empty()) {
      try {
        fileList = CDBFileScanner::searchFileNames(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter, NULL);
      } catch (int e) {
        CDBError("Could not find any filename");
        return 1;
      }

      if (fileList.size() == 0) {
        CDBError("fileList.size()==0");
        return 1;
      }
      dataSource->addStep(fileList[0].c_str());
    }
  }
  return 0;
}

void layerTypeLiveUpdatePopulateDataSource(CDataSource *dataSource, CServerParams *srvParam) {
  // Solar Terminator case (uses a data postprocessor)
  dataSource->srvParams = srvParam;
  dataSource->isConfigured = true;
  dataSource->currentAnimationStep = 0;
  dataSource->layerName.copy("liveupdate_memory");
  CDFObject *cdfObject = new CDFObject();

  // Make x,y (why can this value be anything?)
  int nx = 1, ny = 1;
  CDF::Dimension *dimX = new CDF::Dimension();
  dimX = new CDF::Dimension();
  dimX->name = "x";
  dimX->setSize(nx);

  CDF::Dimension *dimY = new CDF::Dimension();
  dimY = new CDF::Dimension();
  dimY->name = "y";
  dimY->setSize(ny);

  cdfObject->addDimension(dimX);
  cdfObject->addDimension(dimY);

  // Corresponding variables for the dimensions
  CDF::Variable *varX = new CDF::Variable();
  varX->setType(CDF_DOUBLE);
  varX->name.copy("x");
  varX->isDimension = true;
  varX->dimensionlinks.push_back(dimX);
  cdfObject->addVariable(varX);
  varX->addAttribute(new CDF::Attribute("long_name", "x coordinate of projection"));
  varX->addAttribute(new CDF::Attribute("standard_name", "projection_x_coordinate"));
  varX->addAttribute(new CDF::Attribute("units", "metre"));
  CDF::Variable::CustomMemoryReader *memoryReaderX = CDF::Variable::CustomMemoryReaderInstance;
  varX->setCustomReader(memoryReaderX);

  CDF::Variable *varY = new CDF::Variable();
  varY->setType(CDF_DOUBLE);
  varY->name.copy("y");
  varY->isDimension = true;
  varY->dimensionlinks.push_back(dimY);
  cdfObject->addVariable(varY);
  varY->addAttribute(new CDF::Attribute("long_name", "y coordinate of projection"));
  varY->addAttribute(new CDF::Attribute("standard_name", "projection_y_coordinate"));
  varY->addAttribute(new CDF::Attribute("units", "metre"));
  CDF::Variable::CustomMemoryReader *memoryReaderY = CDF::Variable::CustomMemoryReaderInstance;
  varY->setCustomReader(memoryReaderY);

  CDF::Variable *solTVar = new CDF::Variable();
  solTVar->setType(CDF_FLOAT);
  float fillValue[] = {-1};
  solTVar->setAttribute("_FillValue", solTVar->getType(), fillValue, 1);
  solTVar->dimensionlinks.push_back(dimY);
  solTVar->dimensionlinks.push_back(dimX);
  solTVar->setType(CDF_FLOAT);
  solTVar->name = "solarterminator";
  solTVar->setAttributeText("standard_name", "solarterminator");
  solTVar->setAttributeText("long_name", "solar terminator");
  solTVar->setAttributeText("units", "light phase");
  solTVar->setAttributeText("grid_mapping", "projection");
  cdfObject->addVariable(solTVar);
  CDF::Variable::CustomMemoryReader *memoryReaderSolT = CDF::Variable::CustomMemoryReaderInstance;
  solTVar->setCustomReader(memoryReaderSolT);

  // Projection (has no dimensions)
  CDF::Variable *projVar = new CDF::Variable();
  projVar->setType(CDF_SHORT);
  projVar->name.copy("projection");
  projVar->isDimension = false;

  cdfObject->addVariable(projVar);

  // String projection params
  projVar->addAttribute(new CDF::Attribute("long_name", "projection"));
  projVar->addAttribute(new CDF::Attribute("proj4_params", "+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 "
                                                           "+x_0=155000 +y_0=463000 +ellps=bessel "
                                                           "+towgs84=565.417,50.3319,465.552,-0.398957,0.343988,-1.8774,4.0725 "
                                                           "+units=m +no_defs"));
  projVar->addAttribute(new CDF::Attribute("EPSG_code", "EPSG:28992"));
  // Test what this is???
  // dataSource->srvParams->geoParams.crs.copy("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
  projVar->addAttribute(new CDF::Attribute("grid_mapping_name", "stereographic"));

  // Numeric projection params
  double lat0 = 52.15616;
  double lon0 = 5.387639;
  double scale0 = 0.999908;
  double fe = 155000.0;
  double fn = 463000.0;
  double a = 6377397.0;
  double b = 6356079.0;

  projVar->addAttribute(new CDF::Attribute("latitude_of_projection_origin", CDF_DOUBLE, &lat0, 1));
  projVar->addAttribute(new CDF::Attribute("longitude_of_projection_origin", CDF_DOUBLE, &lon0, 1));
  projVar->addAttribute(new CDF::Attribute("scale_factor_at_projection_origin", CDF_DOUBLE, &scale0, 1));
  projVar->addAttribute(new CDF::Attribute("false_easting", CDF_DOUBLE, &fe, 1));
  projVar->addAttribute(new CDF::Attribute("false_northing", CDF_DOUBLE, &fn, 1));
  projVar->addAttribute(new CDF::Attribute("semi_major_axis", CDF_DOUBLE, &a, 1));
  projVar->addAttribute(new CDF::Attribute("semi_minor_axis", CDF_DOUBLE, &b, 1));

  projVar->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

  // Add dummy step
  dataSource->addStep("");
  // Set styles, and solarterminator variable, among other things
  dataSource->setCFGLayer(srvParam, srvParam->configObj->Configuration[0], srvParam->cfg->Layer[0], NULL, 0);
  auto *obj = dataSource->getDataObjectsVector()->at(0);
  obj->cdfObject = cdfObject;
  obj->cdfVariable = solTVar;
  obj->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
}

int layerTypeLiveUpdateRender(CDataSource *dataSource, CServerParams *srvParam) {
  // Do we need something like this?
  // if (dataSource->dWidth == 1) dataSource->dWidth = 2;
  // if (dataSource->dHeight == 1) dataSource->dHeight = 2;

  if (dataSource->cfgLayer->DataPostProc.empty()) {
    // Demo case: render the current time in an image for testing purposes / frontend development
    CDBDebug("LAYER TYPE LIVE UPDATE RENDER");
    CDrawImage image;
    layerTypeLiveUpdateRenderIntoDrawImage(&image, srvParam);
    printf("%s%c%c\n", "Content-Type:image/png", 13, 10);
    CDBDebug("***Number of timesteps %d", dataSource->getNumTimeSteps());
    return image.printImagePng8(true);
  } else {
    layerTypeLiveUpdatePopulateDataSource(dataSource, srvParam);
    return layerTypeLiveUpdateRenderIntoImageDataWriter(dataSource, srvParam);
  }
}

int layerTypeLiveUpdateRenderIntoDrawImage(CDrawImage *image, CServerParams *srvParam) {
  image->enableTransparency(true);
  image->setTrueColor(true);
  image->createImage(srvParam->geoParams);
  image->create685Palette();
  image->rectangle(0, 0, srvParam->geoParams.width, srvParam->geoParams.height, CColor(255, 255, 255, 0), CColor(255, 255, 255, 255));
  const char *fontFile = image->getFontLocation();
  CT::string timeValue = "No time dimension specified";
  if (srvParam->requestDims.size() == 1) {
    timeValue = srvParam->requestDims[0]->value.c_str();
  }
  int stepY = 100;
  for (int y = 0; y < image->getHeight(); y = y + stepY) {
    for (int x = 0; x < image->getWidth(); x = x + 300) {
      image->drawText(x + (((y % (stepY * 2)) / stepY) * 150), y, fontFile, 15, 0.1, timeValue.c_str(), CColor(0, 0, 0, 255));
    }
  }
  return 0;
}

int layerTypeLiveUpdateRenderIntoImageDataWriter(CDataSource *dataSource, CServerParams *srvParam) {
  // General case: Liveupdate with some data postprocessors
  // Covers case of Solar Terminator
  CImageDataWriter imageDataWriter;
  int status = imageDataWriter.init(srvParam, dataSource, dataSource->getNumTimeSteps());
  CDBDebug("Init imageDataWriter status %d", status);

  if (dataSource->getNumTimeSteps() > 1) {
    CDBDebug("Status from create animation was %d", imageDataWriter.createAnimation());
  }

  std::vector<CDataSource *> dataSourceRef = {dataSource};

  if (srvParam->requestType == REQUEST_WMS_GETFEATUREINFO) {
    status = imageDataWriter.getFeatureInfoVirtual(dataSourceRef, 0, int(srvParam->dX), int(srvParam->dY), srvParam);
  }
  if (srvParam->requestType == REQUEST_WMS_GETMAP) {
    status = imageDataWriter.addData(dataSourceRef);
  }

  status = imageDataWriter.end();
  CDBDebug("Ending image data writing with status %d", status);
  return status;
}

int layerTypeLiveUpdateConfigureWMSLayerForGetCapabilities(MetadataLayer *metadataLayer) {
  if (metadataLayer->dataSource->cfgLayer->Title.size() != 0) {
    metadataLayer->layerMetadata.title.copy(metadataLayer->dataSource->cfgLayer->Title[0]->value.c_str());
  } else {
    metadataLayer->layerMetadata.title.copy(metadataLayer->dataSource->cfgLayer->Name[0]->value.c_str());
  }
  CTime timeInstance;

  CT::string timeResolution = LIVEUPDATE_DEFAULT_INTERVAL;
  CT::string offset = LIVEUPDATE_DEFAULT_OFFSET;

  for (auto dim : metadataLayer->layer->Dimension) {
    if (dim->value.equals("time") && !dim->attr.interval.empty()) {
      timeResolution = dim->attr.interval;
    }
  }
  CServerConfig::XMLE_DataPostProc *soltConfig =
      metadataLayer->dataSource->cfgLayer->DataPostProc[CDataPostProcessor::findDataPostProcIndex(metadataLayer->dataSource->cfgLayer->DataPostProc, "solarterminator")];
  if (soltConfig != nullptr) {
    // Calculate the offset based on said parameter
    if (soltConfig->attr.offset.c_str() != nullptr && !soltConfig->attr.offset.empty()) offset.copy(soltConfig->attr.offset.c_str());
    CDBDebug("Offset string is %s", soltConfig->attr.offset.c_str());
    CDBDebug("Assigned offset is %s", offset.c_str());
  }

  LiveUpdateTimeRange range = calculateLiveUpdateTimeRange(timeResolution.c_str(), offset.c_str());

  timeInstance.init("seconds since 1970", "standard");
  double epochTime = timeInstance.getEpochTimeFromDateString(CTime::currentDateTime());
  double defaultOffset = timeInstance.quantizeTimeToISO8601(epochTime, timeResolution.c_str(), "low");
  CT::string defaultTime = timeInstance.dateToISOString(timeInstance.offsetToDate(defaultOffset));
  LayerMetadataDim dim = {.serviceName = "time",
                          .cdfName = "time",
                          .units = "ISO8601",
                          .values = range.startTime + "/" + range.stopTime + "/" + range.interval,
                          .defaultValue = range.defaultTime,
                          .type = "dimtype_time",
                          .hasMultipleValues = true,
                          .hidden = false};
  metadataLayer->layerMetadata.dimList.push_back(dim);

  return 0;
}

LiveUpdateTimeRange calculateLiveUpdateTimeRange(const char *interval, const char *offset) {
  LiveUpdateTimeRange range;
  range.interval = interval;

  CTime timeInstance;
  timeInstance.init("seconds since 1970", "standard");

  CTime::Date delta = CTime::periodToDate(offset);
  // Note: timeToOffset does not work in the case of an interval (only for full dates)
  int offsetSeconds = delta.second + delta.minute * 60 + delta.hour * 3600 + delta.day * 86400 + delta.month * 30 * 86400 + delta.year * 365 * 86400;
  double epochTime = timeInstance.getEpochTimeFromDateString(CTime::currentDateTime());

  double startTimeOffset = timeInstance.quantizeTimeToISO8601(epochTime - offsetSeconds, interval, "low");
  double stopTimeOffset = timeInstance.quantizeTimeToISO8601(epochTime + offsetSeconds, interval, "low");
  double defaultOffset = timeInstance.quantizeTimeToISO8601(epochTime, interval, "low");

  range.startTime = timeInstance.dateToISOString(timeInstance.offsetToDate(startTimeOffset));
  range.stopTime = timeInstance.dateToISOString(timeInstance.offsetToDate(stopTimeOffset));
  range.defaultTime = timeInstance.dateToISOString(timeInstance.offsetToDate(defaultOffset));
  return range;
}
