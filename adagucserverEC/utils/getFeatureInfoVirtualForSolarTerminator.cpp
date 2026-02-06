#include "getFeatureInfoVirtualForSolarTerminator.h"
#include <LayerTypeLiveUpdate/LayerTypeLiveUpdate.h>

// When dealing with the solar terminator in particular (or any future liveupdate layer)
// and testing it with the Adaguc viewer, we run the risk, depending on the interval, of
// returning too many results. This is why we define a hard limit here, and the output
// will be centered around "now"
#define LIVEUPDATE_MAX_TIMESTEPS 800
int getFeatureInfoVirtualForSolarTerminator(CImageDataWriter *img, std::vector<CDataSource *> dataSources, int dataSourceIndex, int dX, int dY, CServerParams *srvParams) {

  img->isProfileData = false;
  // Determine available range
  LiveUpdateTimeRange timeRange = calculateLiveUpdateTimeRange();

  // Before parsing time, make sure at lease one timestamp (default) requested
  if (srvParams->requestDims.size() == 0) {
    // We put a time request for the current time
    COGCDims *timeDim = new COGCDims();
    timeDim->name.copy("TIME");
    // CT::string timeVal = timeRange.defaultTime + "/" + timeRange.defaultTime;
    timeDim->value.copy(timeRange.defaultTime);
    srvParams->requestDims.push_back(timeDim);
  }

  // Parse time
  CT::string requestedTime, startTime, stopTime;
  size_t timeIdx = 0;
  for (size_t i = 0; i < srvParams->requestDims.size(); i++) {
    if (srvParams->requestDims[i]->name.equals("TIME")) {
      timeIdx = i;
      requestedTime.copy(srvParams->requestDims[i]->value);
      break;
    }
  }

  std::vector<CT::string> timeRangeStrings = requestedTime.split("/");
  if (timeRangeStrings.size() == 2) {
    startTime = timeRangeStrings[0];
    stopTime = timeRangeStrings[1];
  } else {
    // Assume a single timestamp (range with identical start/stop so we iterate once)
    startTime = requestedTime;
    stopTime = requestedTime;
  }

  if (timeRange.interval.empty()) timeRange.interval = LIVEUPDATE_DEFAULT_INTERVAL;

  // Determine overlap ((sub)range of the server time param)
  CTime timeInstance;
  timeInstance.init("seconds since 1970", "standard");

  double reqStart = timeInstance.getEpochTimeFromDateString(startTime);
  double reqStop = timeInstance.getEpochTimeFromDateString(stopTime);
  double availStart = timeInstance.getEpochTimeFromDateString(timeRange.startTime);
  double availStop = timeInstance.getEpochTimeFromDateString(timeRange.stopTime);

  double interStart = std::max(reqStart, availStart);
  double interStop = std::min(reqStop, availStop);

  if (interStart > interStop) {
    CDBDebug("No overlap between requested and available ranges.");
    return 1;
  }

  // Parse the ISO interval
  CTime::Date delta = CTime::periodToDate(timeRange.interval);
  double intervalSeconds = delta.second + delta.minute * 60 + delta.hour * 3600 + delta.day * 86400;

  // Limit the number of generated timeSteps
  size_t expectedSteps = (size_t)((interStop - interStart) / intervalSeconds) + 1;
  std::vector<CT::string> generatedTimestamps;
  if (expectedSteps <= LIVEUPDATE_MAX_TIMESTEPS) {
    // Build list of timesteps with all of them
    for (double t = interStart; t <= interStop; t += intervalSeconds) {
      generatedTimestamps.push_back(timeInstance.dateToISOString(timeInstance.getDate(t)));
      CDBDebug("Generated timestamp %s", generatedTimestamps.back().c_str());
    }

  } else {
    // Extract only central timesteps
    size_t M = LIVEUPDATE_MAX_TIMESTEPS;
    size_t centerIndex = expectedSteps / 2;
    size_t startIndex = centerIndex - M / 2;
    size_t endIndex = startIndex + M;

    for (size_t i = startIndex; i < endIndex; ++i) {
      double t = interStart + i * intervalSeconds;
      generatedTimestamps.push_back(timeInstance.dateToISOString(timeInstance.getDate(t)));
    }
  }

  CDataSource *dataSource = dataSources[dataSourceIndex];
  // Set Geo (bbox)
  getCDPPExecutor()->executeProcessors(dataSource, CDATAPOSTPROCESSOR_RUNBEFOREREADING);

#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("Current dataSource index is %d", dataSourceIndex);
  CDBDebug("dX=%d dY=%d", dX, dY);
  CDBDebug("nativeProj4: '%s'", dataSource->nativeProj4.c_str());
  CDBDebug("dataSource dWidth=%d dHeight=%d", dataSource->dWidth, dataSource->dHeight);
  CDBDebug("dataSource dfBBOX=[%.10f, %.10f, %.10f, %.10f]", dataSource->dfBBOX[0], dataSource->dfBBOX[1], dataSource->dfBBOX[2], dataSource->dfBBOX[3]);
  if (drawImage.Geo != NULL) {
    CDBDebug("drawImage Geo dWidth=%d dHeight=%d", drawImage.Geo->dWidth, drawImage.Geo->dHeight);
    CDBDebug("drawImage Geo BBOX=[%.10f, %.10f, %.10f, %.10f]", drawImage.Geo->dfBBOX[0], drawImage.Geo->dfBBOX[1], drawImage.Geo->dfBBOX[2], drawImage.Geo->dfBBOX[3]);
  } else {
    CDBDebug("drawImage.Geo is NULL!");
  }
#endif

  // Calculate the corresponding lat/lon, according to the pixel of interest
  CT::string ckey;
  ckey.print("%d:%d:%s", dX, dY, dataSource->nativeProj4.c_str());
  CImageWarper warper;
  CImageDataWriter::ProjCacheInfo projInfo = CImageDataWriter::GetProjInfo(ckey, &img->drawImage, dataSource, &warper, srvParams, dX, dY);

  CImageDataWriter::GetFeatureInfoResult *result = new CImageDataWriter::GetFeatureInfoResult();
  result->layerName = dataSource->layerName;
  result->layerTitle = dataSource->layerTitle; // "Virtual feature info";
  result->dataSourceIndex = dataSourceIndex;
  result->lon_coordinate = projInfo.lonX;
  result->lat_coordinate = projInfo.lonY;

  // Special case when there are no server coords
  // Happens when we have a point instead of a bbox
  size_t ptr = 0;
  if (dataSource->srvParams->dX == -1 && dataSource->srvParams->dY == -1) {
    dataSource->srvParams->dX = projInfo.lonX;
    dataSource->srvParams->dY = projInfo.lonY;
    ptr = 0;
  } else {
    // Calculate pointer to results
    ptr = projInfo.imx + projInfo.imy * projInfo.dWidth;
  }

#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("Preparing result with name %s", result->layerName.c_str());
  CDBDebug("lon_coordinate: %f, lat_coordinate: %f", result->lon_coordinate, result->lat_coordinate);
  CDBDebug("Preparing result with imx %d, imy %d, dWidth %d", projInfo.imx, projInfo.imy, projInfo.dWidth);
  CDBDebug("Number of timestamps is %d", generatedTimestamps.size());
#endif

  // Generate one element per time step;
  for (size_t i = 0; i < generatedTimestamps.size(); ++i) {
    CImageDataWriter::GetFeatureInfoResult::Element *element = new CImageDataWriter::GetFeatureInfoResult::Element();
    element->dataSource = dataSource;
    element->var_name.copy(dataSource->getDataObject(0)->cdfVariable->name);
    element->standard_name.copy(dataSource->getDataObject(0)->cdfVariable->getAttributeNE("standard_name")->toString());
    element->long_name.copy(dataSource->getDataObject(0)->cdfVariable->getAttributeNE("long_name")->toString());
    element->units.copy(dataSource->getDataObject(0)->cdfVariable->getAttributeNE("units")->toString());

    // Update dataSource timestamp and apply postprocessors
    if (!dataSource->srvParams->requestDims.empty()) {
      // Note: Check what to do when the requested dims are empty
      dataSource->srvParams->requestDims[timeIdx]->value.copy(generatedTimestamps[i].c_str());
    }

    getCDPPExecutor()->executeProcessors(dataSource, CDATAPOSTPROCESSOR_RUNAFTERREADING);
    float *vals = (float *)dataSource->getDataObject(0)->cdfVariable->data;
    element->value.print("%f", vals[ptr]);

    element->cdfDims.addDimension("time", generatedTimestamps[i].c_str(), i);
    result->elements.push_back(element);
  }
  // Add results to result array
  img->getFeatureInfoResultList.push_back(result);
  return 0;
}
