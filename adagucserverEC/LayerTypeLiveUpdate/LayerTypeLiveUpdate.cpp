#include "LayerTypeLiveUpdate.h"

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
  dataSource->addStep("", NULL);
  dataSource->getCDFDims()->addDimension("none", "0", 0);
  return 0;
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

int layerTypeLiveUpdateConfigureWMSLayerForGetCapabilities(MetadataLayer *metadataLayer) {
  if (metadataLayer->dataSource->cfgLayer->Title.size() != 0) {
    metadataLayer->layerMetadata.title.copy(metadataLayer->dataSource->cfgLayer->Title[0]->value.c_str());
  } else {
    metadataLayer->layerMetadata.title.copy(metadataLayer->dataSource->cfgLayer->Name[0]->value.c_str());
  }
  CTime timeInstance;
  timeInstance.init("seconds since 1970", "standard");
  double epochTime = timeInstance.getEpochTimeFromDateString(CTime::currentDateTime());
  // CTime::Date cdate = timeInstance.getDate(epochTime);
  double startTimeOffset = timeInstance.quantizeTimeToISO8601(epochTime - 3600, "PT1S", "low");
  double stopTimeOffset = timeInstance.quantizeTimeToISO8601(epochTime, "PT1S", "low");
  CT::string startTime = timeInstance.dateToISOString(timeInstance.offsetToDate(startTimeOffset));
  CT::string stopTime = timeInstance.dateToISOString(timeInstance.offsetToDate(stopTimeOffset));
  CT::string resTime = "PT1S";
  LayerMetadataDim dim = {
      .serviceName = "time",
      .cdfName = "time",
      .units = "ISO8601",
      .values = startTime + "/" + stopTime + "/" + resTime,
      .defaultValue = stopTime,
      .hasMultipleValues = true,
      .hidden = false,
      .isvertical = false,
      .iscustom = false,
  };

  metadataLayer->layerMetadata.dimList.push_back(dim);

  return 0;
}