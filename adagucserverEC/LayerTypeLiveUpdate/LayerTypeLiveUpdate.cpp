#include "LayerTypeLiveUpdate.h"
#include "CServerParams.h"

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
    dataSource->addStep("", NULL);
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
      dataSource->addStep(fileList[0].c_str(), NULL);
    }
  }
  return 0;
}

int layerTypeLiveUpdateRender(CDataSource *dataSource, CServerParams *srvParam) {
  CDBDebug("in special liveupdate case with timesteps %d", dataSource->getNumTimeSteps());

  if (dataSource->cfgLayer->DataPostProc.empty()) {
    // Demo case: render the current time in an image for testing purposes / frontend development
    CDrawImage image;
    layerTypeLiveUpdateRenderIntoDrawImage(&image, srvParam);
    printf("%s%c%c\n", "Content-Type:image/png", 13, 10);
    CDBDebug("***Number of timesteps %d", dataSource->getNumTimeSteps());
    return image.printImagePng8(true);
  } else {
    // General case: Liveupdate with some data postprocessors
    return layerTypeLiveUpdateRenderIntoImageDataWriter(dataSource, srvParam);
  }
}

int layerTypeLiveUpdateRenderIntoDrawImage(CDrawImage *image, CServerParams *srvParam) {
  image->enableTransparency(true);
  image->setTrueColor(true);
  image->createImage(srvParam->Geo);
  image->create685Palette();
  image->rectangle(0, 0, srvParam->Geo->dWidth, srvParam->Geo->dHeight, CColor(255, 255, 255, 0), CColor(255, 255, 255, 255));
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
  status = imageDataWriter.addData(dataSourceRef);
  CDBDebug("Adding data status was %d", status);

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

  CT::string timeResolution = "PT10M";

  for (auto dim : metadataLayer->layer->Dimension) {
    if (dim->value.equals("time") && !dim->attr.interval.empty()) {
      timeResolution = dim->attr.interval;
    }
  }

  timeInstance.init("seconds since 1970", "standard");
  double epochTime = timeInstance.getEpochTimeFromDateString(CTime::currentDateTime());
  // CTime::Date cdate = timeInstance.getDate(epochTime);
  double startTimeOffset = timeInstance.quantizeTimeToISO8601(epochTime - 3600 * 24 * 365, timeResolution.c_str(), "low");
  double stopTimeOffset = timeInstance.quantizeTimeToISO8601(epochTime, timeResolution.c_str(), "low");
  CT::string startTime = timeInstance.dateToISOString(timeInstance.offsetToDate(startTimeOffset));
  CT::string stopTime = timeInstance.dateToISOString(timeInstance.offsetToDate(stopTimeOffset));
  LayerMetadataDim dim = {.serviceName = "time",
                          .cdfName = "time",
                          .units = "ISO8601",
                          .values = startTime + "/" + stopTime + "/" + timeResolution,
                          .defaultValue = stopTime,
                          .hasMultipleValues = true,
                          .hidden = false,
                          .isvertical = false,
                          .iscustom = false};
  metadataLayer->layerMetadata.dimList.push_back(dim);

  return 0;
}