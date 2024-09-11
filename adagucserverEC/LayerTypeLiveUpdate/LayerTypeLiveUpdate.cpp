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

int layerTypeLiveUpdateConfigureWMSLayerForGetCapabilities(WMSLayer *myWMSLayer) {
  if (myWMSLayer->dataSource->cfgLayer->Title.size() != 0) {
    myWMSLayer->title.copy(myWMSLayer->dataSource->cfgLayer->Title[0]->value.c_str());
  } else {
    myWMSLayer->title.copy(myWMSLayer->dataSource->cfgLayer->Name[0]->value.c_str());
  }
  CTime timeInstance;
  timeInstance.init("seconds since 1970", "standard");
  double epochTime = timeInstance.getEpochTimeFromDateString(CTime::currentDateTime());
  // CTime::Date cdate = timeInstance.getDate(epochTime);
  double startTimeOffset = timeInstance.quantizeTimeToISO8601(epochTime - 3600 * 24 * 365, "PT10M", "low");
  double stopTimeOffset = timeInstance.quantizeTimeToISO8601(epochTime, "PT10M", "low");
  CT::string startTime = timeInstance.dateToISOString(timeInstance.offsetToDate(startTimeOffset));
  CT::string stopTime = timeInstance.dateToISOString(timeInstance.offsetToDate(stopTimeOffset));
  CT::string resTime = "PT10M";
  WMSLayer::Dim *dim = new WMSLayer::Dim();
  myWMSLayer->dimList.push_back(dim);
  dim->name.copy("time");
  dim->units.copy("ISO8601");
  dim->values.copy(startTime + "/" + stopTime + "/" + resTime);
  dim->defaultValue.copy(stopTime.c_str());
  dim->hasMultipleValues = true;
  return 0;
}