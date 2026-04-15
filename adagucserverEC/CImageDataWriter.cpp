/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include <set>

#include <vector>
#include <sstream>
#include <algorithm>
#include "CCreateScaleBar.h"
#include "CLegendRenderers/CCreateLegend.h"

#include "CImageDataWriter.h"
#include "CMakeJSONTimeSeries.h"
#include "CMakeEProfile.h"
#include "CReporter.h"
#include "CImgWarpHillShaded.h"
#include "CImgWarpGeneric/CImgWarpGeneric.h"
#include "CDataPostProcessors/CDataPostProcessor_UVComponents.h"
#include "GenericDataWarper/gdwFindPixelExtent.h"
#include "traceTimings/traceTimings.h"
#include "LayerTypeLiveUpdate/LayerTypeLiveUpdate.h"
#include "utils/getFeatureInfoVirtualForSolarTerminator.h"

std::string months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
std::map<std::string, CImageDataWriter::ProjCacheInfo> CImageDataWriter::projCacheMap;
std::map<std::string, CImageDataWriter::ProjCacheInfo>::iterator CImageDataWriter::projCacheIter;

CImageDataWriter::ProjCacheInfo CImageDataWriter::GetProjInfo(std::string key, CDrawImage *drawImage, CDataSource *dataSource, CImageWarper *imageWarper, CServerParams *srvParam, int dX, int dY) {
  ProjCacheInfo projCacheInfo;

  try {

    projCacheIter = projCacheMap.find(key);
    if (projCacheIter == projCacheMap.end()) {
      throw 1;
    }
    projCacheInfo = (*projCacheIter).second;

  } catch (int e) {
    projCacheInfo.isOutsideBBOX = false;

    int status = imageWarper->initreproj(dataSource, drawImage->geoParams, &srvParam->cfg->Projection);
    if (status != 0) {
      CDBError("initreproj failed");
      throw(1);
    }

    double x, y, sx, sy;
    sx = dX;
    sy = dY;

    x = double(sx) / double(drawImage->geoParams.width);
    y = double(sy) / double(drawImage->geoParams.height);
    x *= (drawImage->geoParams.bbox.right - drawImage->geoParams.bbox.left);
    y *= (drawImage->geoParams.bbox.bottom - drawImage->geoParams.bbox.top);
    x += drawImage->geoParams.bbox.left;
    y += drawImage->geoParams.bbox.top;

    projCacheInfo.isOutsideBBOX = false;

    double y1 = dataSource->dfBBOX[1];
    double y2 = dataSource->dfBBOX[3];
    double x1 = dataSource->dfBBOX[0];
    double x2 = dataSource->dfBBOX[2];
    if (y2 < y1) {
      if (y1 > -360 && y2 < 360 && x1 > -720 && x2 < 720) {
        // projInvertedFirst = true;
        double checkBBOX[4];
        for (int j = 0; j < 4; j++) checkBBOX[j] = dataSource->dfBBOX[j];

        // CDBDebug("Current BBOX:  %f %f %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
        bool hasError = false;
        if (imageWarper->reprojpoint_inv(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;
        if (imageWarper->reprojpoint(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;

        if (imageWarper->reprojpoint_inv(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;
        if (imageWarper->reprojpoint(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;

        if (checkBBOX[2] + 200 < dataSource->dfBBOX[2]) {
          checkBBOX[2] += 360;
        }

        if (hasError == false) {
          for (int j = 0; j < 4; j++) dataSource->dfBBOX[j] = checkBBOX[j];
        }
      }
    }

    projCacheInfo.CoordX = x;
    projCacheInfo.CoordY = y;

    imageWarper->reprojpoint(x, y);
    if (isLonLatProjection(&dataSource->nativeProj4)) {
      if (x >= -180 && x < 180) {
        while (x < dataSource->dfBBOX[0]) x += 360;
      } else {
        projCacheInfo.isOutsideBBOX = true;
      }
    }

    projCacheInfo.nativeCoordX = x;
    projCacheInfo.nativeCoordY = y;

    x -= dataSource->dfBBOX[0];
    y -= dataSource->dfBBOX[1];
    x /= (dataSource->dfBBOX[2] - dataSource->dfBBOX[0]);
    y /= (dataSource->dfBBOX[3] - dataSource->dfBBOX[1]);
    x *= double(dataSource->dWidth);
    y *= double(dataSource->dHeight);

    projCacheInfo.dWidth = dataSource->dWidth;
    projCacheInfo.dHeight = dataSource->dHeight;
    projCacheInfo.dX = (dataSource->dfBBOX[2] - dataSource->dfBBOX[0]) / double(dataSource->dWidth);
    projCacheInfo.dY = (dataSource->dfBBOX[3] - dataSource->dfBBOX[1]) / double(dataSource->dHeight);

    if (x < 0 || x >= dataSource->dWidth) {
      projCacheInfo.imx = -1;
      projCacheInfo.isOutsideBBOX = true;
    } else {
      projCacheInfo.imx = int(x);
    }
    if (y < 0 || y >= dataSource->dHeight) {
      projCacheInfo.imy = -1;
      projCacheInfo.isOutsideBBOX = true;
    } else {
      projCacheInfo.imy = dataSource->dHeight - (int)y - 1;
    }
    projCacheInfo.lonX = projCacheInfo.CoordX;
    projCacheInfo.lonY = projCacheInfo.CoordY;
    // Get lat/lon
    imageWarper->reprojToLatLon(projCacheInfo.lonX, projCacheInfo.lonY);
    imageWarper->closereproj();
    projCacheMap[key] = projCacheInfo;
  }
  return projCacheInfo;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  return split(s, delim, elems);
}

CImageDataWriter::CImageDataWriter() {

  // Mode can be "uninitialized"0 "initialized"(1) and "finished" (2)
  writerStatus = uninitialized;
}

int CImageDataWriter::_setTransparencyAndBGColor(CServerParams *srvParam, CDrawImage *drawImage) {
  if (srvParam->Transparent == true) {
    drawImage->enableTransparency(true);
  } else {
    drawImage->enableTransparency(false);
    // Set BGColor
    if (srvParam->BGColor.length() > 0) {
      if (srvParam->BGColor.length() != 8) {
        CDBError("BGCOLOR does not comply to format 0xRRGGBB");
        return 1;
      }
      if (srvParam->BGColor.c_str()[0] != '0' || srvParam->BGColor.c_str()[1] != 'x') {
        CDBError("BGCOLOR does not comply to format 0xRRGGBB");
        return 1;
      }
      int hexa[8];

      for (size_t j = 0; j < 6; j++) {
        hexa[j] = srvParam->BGColor.charAt(j + 2);
        hexa[j] -= 48;
        if (hexa[j] > 16) hexa[j] -= 7;
      }
      drawImage->setBGColor(hexa[0] * 16 + hexa[1], hexa[2] * 16 + hexa[3], hexa[4] * 16 + hexa[5]);

    } else {
      drawImage->setBGColor(255, 255, 255);
    }
  }
  return 0;
}

void CImageDataWriter::getFeatureInfoGetPointDataResults(CDataSource *dataSource, GetFeatureInfoResult &getFeatureInfoResult, int dataObjectNrInDataSource, int maxPixelDistance) {
  if (dataSource->getDataObject(dataObjectNrInDataSource)->points.size() == 0 || dataSource->getDataObject(dataObjectNrInDataSource)->cdfVariable->getAttributeNE("ADAGUC_SKIP_POINTS") != NULL) {
    return;
  }
  int closestIndex = findClosestPoint(dataSource->getDataObject(dataObjectNrInDataSource)->points, getFeatureInfoResult.lon_coordinate, getFeatureInfoResult.lat_coordinate);
  if (closestIndex == -1) {
    return;
  }
  /* Now we have detected the closest point, check if it is in a certain distance in the map in pixel coordinates */
  double pointPX = dataSource->getDataObject(dataObjectNrInDataSource)->points[closestIndex].lon, pointPY = dataSource->getDataObject(dataObjectNrInDataSource)->points[closestIndex].lat;
  double gfiPX = getFeatureInfoResult.lon_coordinate, gfiPY = getFeatureInfoResult.lat_coordinate;
  CImageWarper warper;
  /* We can always use LATLONPROJECTION, because getFeatureInfoResult.lon_coordinate and getFeatureInfoResult.lat_coordinate are always converted to latlon in CImageDataWriter */
  int status = warper.initreproj(LATLONPROJECTION, srvParam->geoParams, &srvParam->cfg->Projection);
  if (status != 0) {
    CDBError("Unable to initialize projection");
  }
  warper.reprojpoint_inv_topx(pointPX, pointPY, srvParam->geoParams);
  warper.reprojpoint_inv_topx(gfiPX, gfiPY, srvParam->geoParams);
  float pixelDistance = hypot(pointPX - gfiPX, pointPY - gfiPY);

  if (pixelDistance > maxPixelDistance) {
    return;
  }

  auto dataObject = dataSource->getDataObject(dataObjectNrInDataSource);
  auto &point = dataObject->points[closestIndex];
  auto &element = getFeatureInfoResult.elements.back();
  if (element.value.empty() || element.value == "nodata" || (dataObject->hasNodataValue && element.value == CT::printf("%f", dataObject->dfNodataValue))) {
    if (dataObject->hasStatusFlag) {
      // Add status flag
      std::string flagMeaning = CDataSource::getFlagMeaningHumanReadable(dataObject->statusFlagList, (int)point.v);
      element.value = CT::printf("%s (%d)", flagMeaning.c_str(), (int)point.v);
      element.units = "";
    } else {
      element.value = CT::printf("%f", point.v);
    }
  }

  // Loop over point paramlist
  for (const auto &param: point.paramList) {
    getFeatureInfoResult.elements.push_back(GFIElement());
    auto &pointID = getFeatureInfoResult.elements.back();
    pointID.dataSource = dataSource;
    pointID.cdfDims = *dataSource->getCDFDims();
    pointID.long_name = param.description;
    pointID.var_name = param.key;
    pointID.standard_name = param.key;
    pointID.feature_name = param.key;
    pointID.value = param.value;
    pointID.units = "";
  }
}

int CImageDataWriter::init(CServerParams *srvParam, CDataSource *dataSource, int) {
  int status = 0;
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("init");
#endif
  if (writerStatus != uninitialized) {
    CDBError("Already initialized");
    return 1;
  }
  this->srvParam = srvParam;

  if (_setTransparencyAndBGColor(this->srvParam, &drawImage) != 0) return 1;

  CStyleConfiguration *styleConfiguration = NULL;

  if (dataSource != NULL) {
    styleConfiguration = dataSource->getStyle();
  }

  if (srvParam->imageMode == SERVERIMAGEMODE_RGBA || srvParam->Styles.indexOf("HQ") > 0) {
    drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
  }

  if (styleConfiguration != NULL) {

    if (styleConfiguration->renderMethod & RM_RGBA) {

      drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
      if (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) {
        // CDBDebug("drawImage.create685Palette();");
        writerStatus = initialized;
        drawImage.createImage(40, 20);
        drawImage.create685Palette();
        return 0;
      }
    }
  }

  // WMS Format in layer always overrides all
  if (dataSource != NULL) {
    if (dataSource->cfgLayer->WMSFormat.size() > 0) {
      if (dataSource->cfgLayer->WMSFormat[0]->attr.name.equals("image/png32")) {
        drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
      }
      if (dataSource->cfgLayer->WMSFormat[0]->attr.format.equals("image/png32")) {
        drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
      }
      if (dataSource->cfgLayer->WMSFormat[0]->attr.format.equals("image/png24")) {
        drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
      }
      if (dataSource->cfgLayer->WMSFormat[0]->attr.format.equals("image/webp")) {
        drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
        srvParam->imageFormat = IMAGEFORMAT_IMAGEWEBP;
      }
      if (dataSource->cfgLayer->WMSFormat[0]->attr.quality.empty() == false) {
        srvParam->imageQuality = dataSource->cfgLayer->WMSFormat[0]->attr.quality.toInt();
      }
    }
  }
  // Set font location
  if (srvParam->cfg->WMS[0]->ContourFont.size() != 0) {
    if (srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.empty() == false) {
      drawImage.setTTFFontLocation(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str());

      if (srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.empty() == false) {
        std::string fontSize = srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.c_str();
        drawImage.setTTFFontSize(std::stod(fontSize));
      }
      // CDBError("Font %s",srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str());
      // return 1;

    } else {
      CDBError("In <Font>, attribute \"location\" missing");
      return 1;
    }
  }

  // Set background opacity, if applicable
  if (srvParam->wmsExtensions.opacity < 100) {
    // CDBDebug("srvParam->wmsExtensions.opacity %d",srvParam->wmsExtensions.opacity);
    drawImage.setBackGroundAlpha((unsigned char)(float(srvParam->wmsExtensions.opacity) * 2.55));
  }

  writerStatus = initialized;

  if (srvParam->requestType == REQUEST_WMS_GETMAP) {
    //  CDBDebug("CREATING IMAGE FOR WMS GETMAP ---------------------------------------");
    status = drawImage.createImage(srvParam->geoParams);

    if (status != 0) return 1;
  }
  if (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) {
    // drawImage.setTrueColor(false);

    int w = LEGEND_WIDTH;
    int h = LEGEND_HEIGHT;
    if (srvParam->geoParams.width != 1) w = srvParam->geoParams.width;
    if (srvParam->geoParams.height != 1) h = srvParam->geoParams.height;

    if (w > h) {
      status = drawImage.createImage(h, w);
    } else {
      status = drawImage.createImage(w, h);
    }

    if (status != 0) return 1;
  }
  if (srvParam->requestType == REQUEST_WMS_GETFEATUREINFO || srvParam->requestType == REQUEST_WMS_GETHISTOGRAM) {
    // status = drawImage.createImage(2,2);
    drawImage.geoParams = srvParam->geoParams;

#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("/init");
#endif
    return 0;
  }

  if (dataSource != NULL) {
    // Create 6-8-5 palette for cascaded layer
    if (dataSource->dLayerType == CConfigReaderLayerTypeGraticule) {
      status = drawImage.create685Palette();
      if (status != 0) {
        CDBError("Unable to create standard 6-8-5 palette");
        return 1;
      }
    }

    if (styleConfiguration != NULL) {
      if (styleConfiguration->legendIndex != -1) {
        // Create palette for internal WNS layer
        if (dataSource->dLayerType != CConfigReaderLayerTypeGraticule) {
          status = drawImage.createPalette(srvParam->cfg->Legend[styleConfiguration->legendIndex]);
          if (status != 0) {
            CDBError("Unknown palette type for %s", srvParam->cfg->Legend[styleConfiguration->legendIndex]->attr.name.c_str());
            return 1;
          }
        }
      }
    }
  }
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("/init");
#endif
  return 0;
}

double CImageDataWriter::convertValue(CDFType type, void *data, size_t ptr) {
  switch (type) {
  case CDF_CHAR:
    return ((char *)data)[ptr];
  case CDF_BYTE:
    return ((char *)data)[ptr];
  case CDF_UBYTE:
    return ((unsigned char *)data)[ptr];
  case CDF_SHORT:
    return ((short *)data)[ptr];
  case CDF_USHORT:
    return ((ushort *)data)[ptr];
  case CDF_INT:
    return ((int *)data)[ptr];
  case CDF_UINT:
    return ((uint *)data)[ptr];
  case CDF_FLOAT:
    return ((float *)data)[ptr];
  case CDF_DOUBLE:
    return ((double *)data)[ptr];
  default:
    CDBError("Unknown type detected in convertValue, type = %d", type);
    break;
  }
  return 0;
}
void CImageDataWriter::setValue(CDFType type, void *data, size_t ptr, double pixel) {
  if (type == CDF_CHAR || type == CDF_BYTE) ((char *)data)[ptr] = (char)pixel;
  if (type == CDF_UBYTE) ((unsigned char *)data)[ptr] = (unsigned char)pixel;
  if (type == CDF_SHORT) ((short *)data)[ptr] = (short)pixel;
  if (type == CDF_USHORT) ((unsigned short *)data)[ptr] = (unsigned short)pixel;
  if (type == CDF_INT) ((int *)data)[ptr] = (int)pixel;
  if (type == CDF_UINT) ((unsigned int *)data)[ptr] = (unsigned int)pixel;
  if (type == CDF_FLOAT) ((float *)data)[ptr] = (float)pixel;
  if (type == CDF_DOUBLE) ((double *)data)[ptr] = (double)pixel;
}

int CImageDataWriter::getFeatureInfo(std::vector<CDataSource *> dataSources, int dataSourceIndex, int dX, int dY) {
  CImageWarper imageWarper;
#ifdef MEASURETIME
  StopWatch_Stop("getFeatureInfo");
#endif

#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("[getFeatureInfo] %lu, %d, [%d,%d]", dataSources.size(), dataSourceIndex, dX, dY);
#endif
  // Create a new getFeatureInfoResult object and push it into the vector.
  int status = 0;
  isProfileData = false;
  for (auto dataSource: dataSources) {
    if (dataSource == NULL) {
      CDBError("dataSource == NULL");
      return 1;
    }

    getFeatureInfoResultList.push_back(GetFeatureInfoResult());
    GetFeatureInfoResult &getFeatureInfoResult = (getFeatureInfoResultList.back());
    bool headerIsAvailable = false;
    bool sameHeaderForAll = false;
    bool openAll = false;

    bool everythingIsInBBOX = true;

    CDataReader reader;
    reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);
    if (dataSource->getNumDataObjects() > 0) {
      if (dataSource->getFirstAvailableDataObject()->cdfVariable != NULL) {
        if (dataSource->isConfigured) {
          if (dataSource->nativeProj4.length() > 0) {
            headerIsAvailable = true;
          }
        }
        if (dataSource->getFirstAvailableDataObject()->cdfVariable->getAttributeNE("ADAGUC_VECTOR") != NULL) {
          openAll = true;
        }

        if (dataSource->getFirstAvailableDataObject()->cdfVariable->getAttributeNE("ADAGUC_POINT") != NULL) {
          openAll = true;
        }

        if (dataSource->getFirstAvailableDataObject()->cdfVariable->getAttributeNE("UGRID_MESH") != NULL) {
          openAll = true;
        }
        if (dataSource->getFirstAvailableDataObject()->cdfVariable->getAttributeNE("ADAGUC_PROFILE") != NULL) {
          isProfileData = true;
        }
        if (dataSource->getFirstAvailableDataObject()->cdfObject->getAttributeNE("ADAGUC_GEOJSON") != NULL) {
          openAll = true;
        }

        if (dataSource->getFirstAvailableDataObject()->cdfObject->getAttributeNE("USE_ADAGUC_LATLONBNDS_CONVERTER") != NULL) {
          openAll = true;
        }

        if (dataSource->getFirstAvailableDataObject()->cdfObject->getAttributeNE(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ID) != NULL) {
          sameHeaderForAll = true;
        }

        if (dataSource->cfgLayer->FilePath.size() == 1 && dataSource->cfgLayer->FilePath[0]->attr.gfi_openall.equals("true")) {
          openAll = true;
        }
        if (dataSource->cfgLayer->FilePath.size() == 1 && dataSource->cfgLayer->FilePath[0]->attr.gfi_openall.equals("headers")) {
          sameHeaderForAll = true;
        }
      }
    }
    // CDBDebug("gfi_openall: %d %d",dataSource->cfgLayer->FilePath.size(),openAll);

    if (dataSource->cfgLayer->TileSettings.size() == 1) {
      openAll = true;
    }
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("isProfileData:[%d] openAll:[%d] sameHeaderForAll:[%d] infoFormat:[%s]", isProfileData, openAll, sameHeaderForAll, srvParam->InfoFormat.c_str());
#endif
    if (isProfileData) {
      int status = CMakeEProfile::MakeEProfile(&drawImage, &imageWarper, dataSource, dX, dY, &eProfileJson);
      if (status != 0) {
        CDBError("CMakeEProfile::MakeEProfile failed");
        return status;
      }
    } else if (sameHeaderForAll == false && openAll == false && srvParam->InfoFormat.equals("application/json")) {
      int status = CMakeJSONTimeSeries::MakeJSONTimeSeries(&drawImage, &imageWarper, dataSource, dX, dY, &gfiStructure);
      if (status != 0) {
        CDBError("CMakeJSONTimeSeries::MakeJSONTimeSeries failed");
        return status;
      }
    } else {
      // return 1;
      CDBDebug("Num time steps for dataSource %d", dataSource->getNumTimeSteps());

      std::map<std::string, bool> dimensionKeyValueMap; // A map for every dimensionvalue linked to a value

      for (int step = 0; step < dataSource->getNumTimeSteps(); step++) {
        dataSource->setTimeStep(step);

        CCDFDims *cdfDims = dataSource->getCDFDims();

        // Copy layer name
        getFeatureInfoResult.layerName = dataSource->layerName;
        getFeatureInfoResult.layerTitle = dataSource->layerName;

        getFeatureInfoResult.dataSourceIndex = dataSourceIndex;

#ifdef CIMAGEDATAWRITER_DEBUG
        CDBDebug("Processing dataSource %s with result %d of %d results (%d) %f", dataSource->getLayerName(), step, dataSource->getNumTimeSteps(),
                 dataSource->getFirstAvailableDataObject()->hasNodataValue, dataSource->getFirstAvailableDataObject()->dfNodataValue);
#endif

        CDataReader reader;
        // if(!headerIsAvailable)
        {
          if (openAll) {
// CDBDebug("OPEN ALL");
#ifdef CIMAGEDATAWRITER_DEBUG
            CDBDebug("OPEN ALL");
#endif
            status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_ALL);
          } else {
// CDBDebug("OPEN HEADER");
#ifdef CIMAGEDATAWRITER_DEBUG
            CDBDebug("OPEN Header %d", headerIsAvailable);
#endif
            if (!headerIsAvailable || sameHeaderForAll == true) {
              headerIsAvailable = true;
              status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);
            }
          }

          if (status != 0) {
            CDBError("Could not open file: %s", dataSource->getFileName().c_str());
            return 1;
          }
        }

        std::string ckey = CT::printf("%d:%d:%d:%d:%s:%f:%f:%f:%f", dX, dY, dataSource->dWidth, dataSource->dHeight, dataSource->nativeProj4.c_str(), dataSource->dfBBOX[0], dataSource->dfBBOX[1],
                                      dataSource->dfBBOX[2], dataSource->dfBBOX[3]);
        CImageDataWriter::ProjCacheInfo projCacheInfo = GetProjInfo(ckey, &drawImage, dataSource, &imageWarper, srvParam, dX, dY);

        // Projections coordinates in latlon
        getFeatureInfoResult.lon_coordinate = projCacheInfo.lonX;
        getFeatureInfoResult.lat_coordinate = projCacheInfo.lonY;

        // Pixel X and Y on the image
        getFeatureInfoResult.x_imagePixel = dX;
        getFeatureInfoResult.y_imagePixel = dY;

        // Projection coordinates X and Y on the image
        getFeatureInfoResult.x_imageCoordinate = projCacheInfo.CoordX;
        getFeatureInfoResult.y_imageCoordinate = projCacheInfo.CoordY;

        // Projection coordinates X and Y in the raster
        getFeatureInfoResult.x_rasterCoordinate = projCacheInfo.nativeCoordX;
        getFeatureInfoResult.y_rasterCoordinate = projCacheInfo.nativeCoordY;

        // Pixel X and Y on the raster
        getFeatureInfoResult.x_rasterIndex = projCacheInfo.imx;
        getFeatureInfoResult.y_rasterIndex = projCacheInfo.imy;

        if (projCacheInfo.imx >= 0 && projCacheInfo.imy >= 0 && projCacheInfo.imx < projCacheInfo.dWidth && projCacheInfo.imy < projCacheInfo.dHeight && projCacheInfo.isOutsideBBOX == false) {

          if (!openAll) {
            status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_ALL, projCacheInfo.imx, projCacheInfo.imy);
          }
          if (status != 0) {
            CDBError("Could not open file: %s", dataSource->getFileName().c_str());
            return 1;
          }
        }

        if (everythingIsInBBOX == true) {

          // Retrieve variable names
          for (size_t o = 0; o < dataSource->getNumDataObjects(); o++) {
            if (dataSource->getDataObject(o)->cdfVariable->data == nullptr) {
              CDBWarning("No variable defined for dataObject %lu for [%s]", o, dataSource->getDataObject(o)->cdfVariable->name.c_str());
              continue;
            }
            if (dataSource->getDataObject(o)->filterFromOutput) {
              continue;
            }

            std::string dimkey = "";
            for (size_t j = 0; j < dataSource->requiredDims.size(); j++) {
              CT::printfconcat(dimkey, "[%s=%s]", cdfDims->at(j).value.c_str(), cdfDims->at(j).name.c_str());
            }
            CT::printfconcat(dimkey, "[dataobject=%lu]", o);

            std::map<std::string, bool>::iterator dimensionKeyValueMapIterator = dimensionKeyValueMap.find(dimkey.c_str());
            if (dimensionKeyValueMapIterator != dimensionKeyValueMap.end()) {
              if (dimensionKeyValueMapIterator->second == false) {
                if (getFeatureInfoResult.elements.size() > 0) {
                  getFeatureInfoResult.elements.pop_back();
                }
              }
            } else {
              dimensionKeyValueMap[dimkey.c_str()] = false;
            }
            getFeatureInfoResult.elements.push_back(GFIElement());
            GFIElement &element = getFeatureInfoResult.elements.back();

            element.dataSource = dataSource;
            // Get variable name
            element.var_name = dObjgetVariableName(*dataSource->getDataObject(o));
            // Get variable units
            std::string units = dObjgetUnits(*dataSource->getDataObject(o));
            element.units = units;

            // Get variable standard name
            CDF::Attribute *attr_standard_name = dataSource->getDataObject(o)->cdfVariable->getAttributeNE("standard_name");
            if (attr_standard_name != NULL) {
              std::string standardName = attr_standard_name->toString();
              element.standard_name = standardName;
              // Make a more clean standard name.
              element.feature_name = CT::printf("%s_%lu", standardName.c_str(), o);
            }
            if (element.standard_name.empty()) {
              element.standard_name = element.var_name;
              element.feature_name = element.var_name;
              element.feature_name = CT::printf("%s_%lu", element.var_name.c_str(), o);
            }

            // Get variable long name
            CDF::Attribute *attr_long_name = dataSource->getDataObject(o)->cdfVariable->getAttributeNE("long_name");
            if (attr_long_name != NULL) {
              element.long_name = attr_long_name->toString();
            } else {
              element.long_name = element.var_name;
            }

            // Assign CDF::Variable Pointer
            element.variable = dataSource->getDataObject(o)->cdfVariable;
            element.value = "nodata";
            element.cdfDims = *dataSource->getCDFDims();

            bool hasData = false;
            if (projCacheInfo.imx >= 0 && projCacheInfo.imy >= 0 && projCacheInfo.imx < projCacheInfo.dWidth && projCacheInfo.imy < projCacheInfo.dHeight) {
              size_t ptr = 0;
              if (openAll) {
                ptr = projCacheInfo.imx + projCacheInfo.imy * projCacheInfo.dWidth;
              }
              // Recalculate the pointer if there is no associated file
              // Case where calculations are based solely on postprocessors
              if (dataSource->getFileName().empty()) {
                ptr = projCacheInfo.imx + projCacheInfo.imy * projCacheInfo.dWidth;
              }

              dataSource->setTimeStep(step);
              double pixel = convertValue(dataSource->getDataObject(o)->cdfVariable->getType(), dataSource->getDataObject(o)->cdfVariable->data, ptr);

              if ((pixel != dataSource->getDataObject(o)->dfNodataValue && dataSource->getDataObject(o)->hasNodataValue == true && pixel == pixel && everythingIsInBBOX == true) ||
                  dataSource->getDataObject(o)->hasNodataValue == false || dataSource->getDataObject(o)->points.size() > 0) {
                if (dataSource->getDataObject(o)->hasStatusFlag) {
                  // Add status flag
                  std::string flagMeaning;
                  flagMeaning = CDataSource::getFlagMeaningHumanReadable(dataSource->getDataObject(o)->statusFlagList, pixel);
                  element.value = CT::printf("%s (%d)", flagMeaning.c_str(), (int)pixel);
                  element.units = "";
                } else {
                  element.value = CT::printf("%f", pixel);
                }

                dimensionKeyValueMap[dimkey.c_str()] = true;
                hasData = true;

                if (dataSource->getDataObject(o)->features.empty() == false) {
                  int closestIndex = pixel;
                  if (pixel == dataSource->getDataObject(o)->dfNodataValue) {
                    // Find the clicked pixel. For geojson this is done by rendering the polygon on the grid with its index.
                    // For points we will support finding the closest one in the list.
                    CDBDebug("Do findClosestPoint");
                    closestIndex = findClosestPoint(dataSource->getDataObject(o)->points, getFeatureInfoResult.lon_coordinate, getFeatureInfoResult.lat_coordinate);
                    if (closestIndex == -1) {
                      closestIndex = pixel;
                    } else {
                      // Calculate distance in pixels
                      double pointPX = dataSource->getDataObject(o)->points[closestIndex].lon, pointPY = dataSource->getDataObject(o)->points[closestIndex].lat;
                      double gfiPX = getFeatureInfoResult.lon_coordinate, gfiPY = getFeatureInfoResult.lat_coordinate;
                      CImageWarper warper;
                      /* We can always use LATLONPROJECTION, because getFeatureInfoResult.lon_coordinate and getFeatureInfoResult.lat_coordinate are always converted to latlon in CImageDataWriter */
                      int status = warper.initreproj(LATLONPROJECTION, srvParam->geoParams, &srvParam->cfg->Projection);
                      if (status != 0) {
                        CDBError("Unable to initialize projection");
                      }
                      warper.reprojpoint_inv_topx(pointPX, pointPY, srvParam->geoParams);
                      warper.reprojpoint_inv_topx(gfiPX, gfiPY, srvParam->geoParams);
                      float pixelDistance = hypot(pointPX - gfiPX, pointPY - gfiPY);
                      warper.closereproj();

                      if (pixelDistance > 30) {
                        hasData = false;
                        element.value = "nodata";
                        return 0;
                      };
                    }
                  }
                  std::map<int, CFeature>::iterator featureIt = dataSource->getDataObject(o)->features.find(closestIndex);
                  if (featureIt != dataSource->getDataObject(o)->features.end()) {
                    CFeature *feature = &featureIt->second;
                    if (feature->paramMap.empty() == false) {
                      for (auto [propertyName, propertyValue]: feature->paramMap) {
                        getFeatureInfoResult.elements.push_back(GFIElement());
                        GFIElement &featureParam = getFeatureInfoResult.elements.back();
                        featureParam.dataSource = dataSource;
                        featureParam.cdfDims = *dataSource->getCDFDims();
                        featureParam.long_name = propertyName.c_str();
                        featureParam.var_name = propertyName.c_str();
                        featureParam.standard_name = propertyName.c_str();
                        featureParam.feature_name = propertyName.c_str();
                        featureParam.value = propertyValue;
                        featureParam.units = "";
                      }
                    }
                  }
                }

              } else {

                element.value = "nodata";
              }
            } else {

              if (hasData == false && dimensionKeyValueMap.find(dimkey.c_str())->second == true) {
                //  getFeatureInfoResult.elements.pop_back();
              }
            }
            // CDBDebug("Elemeent %s", getFeatureInfoResult.elements.back().value.c_str());
            getFeatureInfoGetPointDataResults(dataSource, getFeatureInfoResult, o, 30);
          }
        }
      }
    }
  }

  for (auto &gfiResult: getFeatureInfoResultList) {
    if (gfiResult.elements.size() > 0) {
      gfiResult.layerTitle = gfiResult.elements[0].long_name;
    }
  }

  return 0;
}

void CImageDataWriter::setDate(const char *szTemp) { drawImage.setTextStroke(drawImage.geoParams.width - 170, 5, 0, szTemp, NULL, 12.0, 0.75, CColor(0, 0, 0, 255), CColor(255, 255, 255, 255)); }

CImageDataWriter::IndexRange::IndexRange() {
  min = 0;
  max = 0;
}

std::vector<CImageDataWriter::IndexRange> getIndexRangesForRegex(const std::string &match, const std::vector<std::string> &attributeValues) {
  std::vector<CImageDataWriter::IndexRange> ranges;
  int ret;
  regex_t regex;
  ret = regcomp(&regex, match.c_str(), 0);

  if (!ret) {
    int first = -1;
    int last = -1;
    for (size_t i = 0; i < attributeValues.size(); i++) {
      int matched = 1;
      if (attributeValues[i].length() > 0) {
        matched = regexec(&regex, attributeValues[i].c_str(), 0, NULL, 0);
      }

      if (matched == 0) {
        if (first == -1) {
          first = i;
          last = i + 1;
        } else {
          if ((i - last) > 0) {
            ranges.push_back(CImageDataWriter::IndexRange(first, last));
            first = i;
            last = i + 1;
          } else {
            last = i + 1;
          }
        }
      }
    }
    ranges.push_back(CImageDataWriter::IndexRange(first, last));
  }
  regfree(&regex);
  return ranges;
}

int CImageDataWriter::getFeatureInfoVirtual(std::vector<CDataSource *> dataSources, int dataSourceIndex, int dX, int dY, CServerParams *srvParams) {
  return getFeatureInfoVirtualForSolarTerminator(this, dataSources, dataSourceIndex, dX, dY, srvParams);
}

int CImageDataWriter::warpImage(CDataSource *dataSource, CDrawImage *drawImage) {

  // Open the data of this dataSource
  int status = 0;

  CDataReader reader;

  bool usePixelExtent = false;
  if (usePixelExtent) {

    status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);
    CImageWarper warper;

    status = warper.initreproj(dataSource, srvParam->geoParams, &srvParam->cfg->Projection);
    if (status != 0) {
      CDBError("Unable to initialize projection");
      return 1;
    }

    GeoParameters sourceGeo;

    sourceGeo.width = dataSource->dWidth;
    sourceGeo.height = dataSource->dHeight;
    sourceGeo.bbox = dataSource->dfBBOX;
    sourceGeo.cellsizeX = dataSource->dfCellSizeX;
    sourceGeo.cellsizeY = dataSource->dfCellSizeY;
    sourceGeo.crs = dataSource->nativeProj4;
    int PXExtentBasedOnSource[4];
    gdwFindPixelExtent(PXExtentBasedOnSource, sourceGeo, srvParam->geoParams, &warper);

    status = reader.openExtent(dataSource, CNETCDFREADER_MODE_OPEN_EXTENT, PXExtentBasedOnSource);
  } else {
    status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_ALL);
  }

  if (status != 0) {
    CDBError("Could not open file: %s", dataSource->getFileName().c_str());
    return 1;
  }

  CImageWarperRenderInterface *imageWarperRenderer;
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  auto renderMethod = styleConfiguration->renderMethod;

  /** Apply FeatureInterval config */

  if (styleConfiguration->featureIntervals.size() > 0) {
    int numFeatures = 0;
    try {
      numFeatures = dataSource->getFirstAvailableDataObject()->cdfObject->getDimensionThrows("features")->getSize();
    } catch (int e) {
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Note: While configuring featureInterval: Unable to find features variable");
#endif
    }
    if (numFeatures > 0) {
      std::vector<std::string> attributeValues(numFeatures);
      /* Loop through all configured FeatureInterval elements */
      for (size_t j = 0; j < styleConfiguration->featureIntervals.size(); j++) {
        CServerConfig::XMLE_FeatureInterval *featureInterval = styleConfiguration->featureIntervals[j];
        if (featureInterval->attr.match.empty() == false && featureInterval->attr.matchid.empty() == false) {
          /* Get the matchid attribute for the feature */
          std::string attributeName = featureInterval->attr.matchid;
          for (int featureNr = 0; featureNr < numFeatures; featureNr++) {
            attributeValues[featureNr] = "";
            std::map<int, CFeature>::iterator feature = dataSource->getFirstAvailableDataObject()->features.find(featureNr);
            if (feature != dataSource->getFirstAvailableDataObject()->features.end()) {
              std::map<std::string, std::string>::iterator attributeValueItr = feature->second.paramMap.find(attributeName);
              if (attributeValueItr != feature->second.paramMap.end()) {
                attributeValues[featureNr] = attributeValueItr->second.c_str();
              }
            }
          }
          if (featureInterval->attr.fillcolor.empty() == false) {
            /*
              Make a shade interval configuration based on the match and matchid properties in the GeoJSON.
              The datafield is already populated with the feature indices of the geojson polygon.
              The shadeinterval configuration can style these indices of the polygons with colors.
              Actual rendering of this is done in CImageNearestNeighbour with the _plot function
            */
            std::vector<CImageDataWriter::IndexRange> ranges = getIndexRangesForRegex(featureInterval->attr.match, attributeValues);
            for (size_t i = 0; i < ranges.size(); i++) {
              auto shadeInterval = CServerConfig::XMLE_ShadeInterval();
              shadeInterval.attr.min.print("%d", ranges[i].min);
              shadeInterval.attr.max.print("%d", ranges[i].max);
              shadeInterval.attr.fillcolor = featureInterval->attr.fillcolor;
              shadeInterval.attr.bgcolor = featureInterval->attr.bgcolor;
              shadeInterval.attr.label = featureInterval->attr.label;
              styleConfiguration->shadeIntervals.push_back(std::move(shadeInterval));
            }
          }
        }
      }
    }
  }

  CImageWarper imageWarper;
  status = imageWarper.initreproj(dataSource, drawImage->geoParams, &srvParam->cfg->Projection);
  if (status != 0) {
    CDBError("initreproj failed");
    reader.close();
    return 1;
  }

  traceTimingsSpanStart(TraceTimingType::WARPIMAGERENDER);
  /**
   * Use fast nearest neighbourrenderer
   */
  if (renderMethod & RM_NEAREST || renderMethod & RM_POINT_LINEARINTERPOLATION) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Using CImgWarpNearestNeighbour");
#endif
    imageWarperRenderer = new CImgWarpNearestNeighbour();
    imageWarperRenderer->render(&imageWarper, dataSource, drawImage);
    delete imageWarperRenderer;
  }

  /**
   * Use RGBA renderer
   */
  if (renderMethod & RM_RGBA) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Using CImgWarpNearestRGBA");
#endif
    imageWarperRenderer = new CImgWarpNearestRGBA();
    imageWarperRenderer->render(&imageWarper, dataSource, drawImage);
    delete imageWarperRenderer;
  }

  /**
   * Use bilinear renderer
   */
  if (renderMethod & RM_CONTOUR || renderMethod & RM_BILINEAR || renderMethod & RM_SHADED || renderMethod & RM_VECTOR || renderMethod & RM_BARB || renderMethod & RM_THIN) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Using CImgWarpBilinear");
#endif
    if (dataSource->getFirstAvailableDataObject()->points.size() == 0) {
      imageWarperRenderer = new CImgWarpBilinear();
      std::string bilinearSettings;
      bool drawMap = false;
      bool drawContour = false;
      bool drawVector = false;
      bool drawBarb = false;
      bool drawShaded = false;
      bool drawGridVectors = false;

      if (renderMethod & RM_BILINEAR) drawMap = true;
      if (renderMethod & RM_CONTOUR) drawContour = true;
      if (renderMethod & RM_VECTOR) drawVector = true;
      if (renderMethod & RM_SHADED) drawShaded = true;
      if (renderMethod & RM_BARB) drawBarb = true;
      if (renderMethod & RM_THIN) drawGridVectors = true;

      /*
        Check the if we want to use discrete type with the bilinear rendermethod.
        The bilinear Rendermethod can shade using ShadeInterval if renderhint in RenderSettings is set to RENDERHINT_DISCRETECLASSES
      */
      if (styleConfiguration != nullptr) {
        for (auto renderSetting: styleConfiguration->renderSettings) {
          std::string renderHint = renderSetting->attr.renderhint;
          if (renderHint == RENDERHINT_DISCRETECLASSES) {
            drawMap = false;   // Don't use continous legends with the bilinear renderer
            drawShaded = true; // Use discrete legends defined by ShadeInterval with the bilinear renderer
          }
        }
      }

      if (drawMap == true) CT::printfconcat(bilinearSettings, "drawMap=true;");
      if (drawVector == true) CT::printfconcat(bilinearSettings, "drawVector=true;");
      if (drawBarb == true) CT::printfconcat(bilinearSettings, "drawBarb=true;");
      if (drawShaded == true) CT::printfconcat(bilinearSettings, "drawShaded=true;");
      if (drawContour == true) CT::printfconcat(bilinearSettings, "drawContour=true;");
      if (drawGridVectors) CT::printfconcat(bilinearSettings, "drawGridVectors=true;");
      CT::printfconcat(bilinearSettings, "smoothingFilter=%d;", styleConfiguration->smoothingFilter);
      if (drawShaded == true || drawContour == true) {
        CT::printfconcat(bilinearSettings, "shadeInterval=%0.12f;contourBigInterval=%0.12f;contourSmallInterval=%0.12f;", styleConfiguration->shadeInterval, styleConfiguration->contourIntervalH,
                         styleConfiguration->contourIntervalL);

        for (size_t j = 0; j < styleConfiguration->shadeIntervals.size(); j++) {
          const auto &shadeInterval = styleConfiguration->shadeIntervals[j];
          if (shadeInterval.attr.min.empty() == false && shadeInterval.attr.max.empty() == false) {
            CT::printfconcat(bilinearSettings, "shading=min(%s)$max(%s)$", shadeInterval.attr.min.c_str(), shadeInterval.attr.max.c_str());
            if (shadeInterval.attr.fillcolor.empty() == false) {
              CT::printfconcat(bilinearSettings, "$fillcolor(%s)$", shadeInterval.attr.fillcolor.c_str());
            }
            if (!shadeInterval.attr.bgcolor.empty()) {
              CT::printfconcat(bilinearSettings, "$bgcolor(%s)$", shadeInterval.attr.bgcolor.c_str());
            }
            CT::printfconcat(bilinearSettings, ";");
          }
        }
      }
      if (drawContour == true) {

        for (size_t j = 0; j < styleConfiguration->contourLines.size(); j++) {
          CServerConfig::XMLE_ContourLine *contourLine = styleConfiguration->contourLines[j];
          // Check if we have a interval contour line or a contourline with separate classes
          if (contourLine->attr.interval.empty() == false) {
            // ContourLine interval
            CT::printfconcat(bilinearSettings, "contourline=");
            if (contourLine->attr.width.empty() == false) {
              CT::printfconcat(bilinearSettings, "width(%s)$", contourLine->attr.width.c_str());
            }
            if (contourLine->attr.linecolor.empty() == false) {
              CT::printfconcat(bilinearSettings, "linecolor(%s)$", contourLine->attr.linecolor.c_str());
            }
            if (contourLine->attr.textcolor.empty() == false) {
              CT::printfconcat(bilinearSettings, "textcolor(%s)$", contourLine->attr.textcolor.c_str());
            }
            if (contourLine->attr.textsize.empty() == false) {
              CT::printfconcat(bilinearSettings, "textsize(%s)$", contourLine->attr.textsize.c_str());
            }
            if (contourLine->attr.textstrokewidth.empty() == false) {
              CT::printfconcat(bilinearSettings, "textstrokewidth(%s)$", contourLine->attr.textstrokewidth.c_str());
            }

            if (contourLine->attr.textstrokecolor.empty() == false) {
              CT::printfconcat(bilinearSettings, "textstrokecolor(%s)$", contourLine->attr.textstrokecolor.c_str());
            }
            if (contourLine->attr.interval.empty() == false) {
              CT::printfconcat(bilinearSettings, "interval(%s)$", contourLine->attr.interval.c_str());
            }
            if (contourLine->attr.textformatting.empty() == false) {
              CT::printfconcat(bilinearSettings, "textformatting(%s)$", contourLine->attr.textformatting.c_str());
            }
            if (contourLine->attr.dashing.empty() == false) {
              CT::printfconcat(bilinearSettings, "dashing(%s)$", contourLine->attr.dashing.c_str());
            }
            CT::printfconcat(bilinearSettings, ";");
          }
          if (contourLine->attr.classes.empty() == false) {
            // ContourLine classes
            CT::printfconcat(bilinearSettings, "contourline=");
            if (contourLine->attr.width.empty() == false) {
              CT::printfconcat(bilinearSettings, "width(%s)$", contourLine->attr.width.c_str());
            }
            if (contourLine->attr.linecolor.empty() == false) {
              CT::printfconcat(bilinearSettings, "linecolor(%s)$", contourLine->attr.linecolor.c_str());
            }
            if (contourLine->attr.textcolor.empty() == false) {
              CT::printfconcat(bilinearSettings, "textcolor(%s)$", contourLine->attr.textcolor.c_str());
            }
            if (contourLine->attr.classes.empty() == false) {
              CT::printfconcat(bilinearSettings, "classes(%s)$", contourLine->attr.classes.c_str());
            }
            if (contourLine->attr.textsize.empty() == false) {
              CT::printfconcat(bilinearSettings, "textsize(%s)$", contourLine->attr.textsize.c_str());
            }
            if (contourLine->attr.textstrokewidth.empty() == false) {
              CT::printfconcat(bilinearSettings, "textstrokewidth(%s)$", contourLine->attr.textstrokewidth.c_str());
            }
            if (contourLine->attr.textstrokecolor.empty() == false) {
              CT::printfconcat(bilinearSettings, "textstrokecolor(%s)$", contourLine->attr.textstrokecolor.c_str());
            }
            if (contourLine->attr.textformatting.empty() == false) {
              CT::printfconcat(bilinearSettings, "textformatting(%s)$", contourLine->attr.textformatting.c_str());
            }
            if (contourLine->attr.dashing.empty() == false) {
              CT::printfconcat(bilinearSettings, "dashing(%s)$", contourLine->attr.dashing.c_str());
            }
            CT::printfconcat(bilinearSettings, ";");
          }
        }
      }
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("bilinearSettings.c_str() %s", bilinearSettings.c_str());
#endif
      imageWarperRenderer->set(bilinearSettings.c_str());
      imageWarperRenderer->render(&imageWarper, dataSource, drawImage);
      delete imageWarperRenderer;
    }
  }

  /**
   * Use HillShade renderer
   */
  if (renderMethod & RM_HILLSHADED) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Using CImgWarpHillShaded");
#endif
    imageWarperRenderer = new CImgWarpHillShaded();
    imageWarperRenderer->render(&imageWarper, dataSource, drawImage);
    delete imageWarperRenderer;
  }

  /**
   * Use New bilinear renderer
   */
  if (renderMethod & RM_GENERIC) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Using CImgWarpGeneric");
#endif
    imageWarperRenderer = new CImgWarpGeneric();
    imageWarperRenderer->render(&imageWarper, dataSource, drawImage);
    delete imageWarperRenderer;
  }

  /**
   * Use stippling renderer
   */
  if (renderMethod & RM_STIPPLING) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Using CImgRenderStippling");
#endif
    imageWarperRenderer = new CImgRenderStippling();
    imageWarperRenderer->render(&imageWarper, dataSource, drawImage);
    delete imageWarperRenderer;
  }

  /**
   * Use point renderer
   */
  if (dataSource->getFirstAvailableDataObject()->points.size() != 0) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Using CImgRenderPoints");
#endif
    CImgRenderPoints imageWarperRenderer;
    imageWarperRenderer.render(&imageWarper, dataSource, drawImage);
  }

  /**
   * Use polyline renderer
   */
  if (renderMethod & RM_POLYLINE || renderMethod & RM_POLYGON) {
    if (dataSource->featureSet.length() != 0) {
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Using CImgRenderPolylines");
#endif
      imageWarperRenderer = new CImgRenderPolylines();
      std::string renderMethodAsString = getRenderMethodAsString(renderMethod);
      imageWarperRenderer->set(renderMethodAsString.c_str());
      imageWarperRenderer->render(&imageWarper, dataSource, drawImage);
      delete imageWarperRenderer;
    }
  }
#ifdef MEASURETIME
  StopWatch_Stop("Thread[%d]: warp finished", dataSource->threadNr);
#endif

  traceTimingsSpanEnd(TraceTimingType::WARPIMAGERENDER);
  // imageWarper.closereproj();
  reader.close();

  return 0;
}

int CImageDataWriter::addData(std::vector<CDataSource *> &dataSources) {
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("addData");
#endif
  int status = 0;

#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("Draw data. dataSources.size() =  %lu", dataSources.size());
#endif

  for (size_t j = 0; j < dataSources.size(); j++) {
    CDataSource *dataSource = dataSources[j];

    /* DataBase layers */
    if (dataSource->dLayerType != CConfigReaderLayerTypeGraticule) {
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Drawingnormal legend");
#endif
      if (j != 0) {
/*
 * Reinitialize legend for other type of legends, if possible (in true color mode it is always the case
 * For j==0, the legend is already initialized previously
 */
#ifdef CIMAGEDATAWRITER_DEBUG
        CDBDebug("REINITLEGEND");
#endif

        CStyleConfiguration *styleConfiguration = dataSource->getStyle();
        if (styleConfiguration->legendIndex != -1) {
          status = drawImage.createPalette(srvParam->cfg->Legend[styleConfiguration->legendIndex]);
          if (status != 0) {
            CDBError("Unknown palette type for %s", srvParam->cfg->Legend[styleConfiguration->legendIndex]->attr.name.c_str());
            return 1;
          }
        }
      }

#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Start warping");
#endif

      traceTimingsSpanStart(TraceTimingType::WARPIMAGE);
      status = warpImage(dataSource, &drawImage);
      traceTimingsSpanEnd(TraceTimingType::WARPIMAGE);

#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Finished warping %s for step %d/%d", dataSource->layerName.c_str(), dataSource->getCurrentTimeStep(), dataSource->getNumTimeSteps());
#endif
      if (status != 0) {
        CDBError("warpImage for layer %s failed", dataSource->layerName.c_str());
        return status;
      }
    }
    // if(j==dataSources.size()-1)
    {
      if (status == 0) {

        if (dataSource->cfgLayer->ImageText.size() > 0) {

          std::string imageText = "";
          if (dataSource->cfgLayer->ImageText[0]->value.empty() == false) {
            imageText = dataSource->cfgLayer->ImageText[0]->value;
          }

          if (dataSource->getNumDataObjects() > 0) {
            // Determine ImageText based on configured netcdf attribute
            const char *attrToSearch = dataSource->cfgLayer->ImageText[0]->attr.attribute.c_str();
            if (attrToSearch != NULL) {
              // CDBDebug("Determining ImageText based on netcdf attribute %s",attrToSearch);
              try {
                CDF::Attribute *attr = dataSource->getFirstAvailableDataObject()->cdfObject->getAttributeThrows(attrToSearch);
                if (attr->length > 0) {
                  imageText = attrToSearch;
                  imageText += ": ";
                  imageText += attr->toString().c_str();
                }
              } catch (int e) {
              }
            }
          }

          if (imageText.length() > 0) {
            size_t len = imageText.length();
            double scaling = dataSource->getScaling();
            // CDBDebug("Watermark: %s",imageText.c_str());
            float fontSize = 10;
            if (srvParam->cfg->WMS[0]->SubTitleFont.size() > 0) {
              fontSize = parseFloat(srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.size.c_str());
              fontSize = fontSize * scaling;
            }
            drawImage.drawText(int(drawImage.geoParams.width / 2 - len * 3), drawImage.geoParams.height - 2 * fontSize, srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.location.c_str(), fontSize, 0,
                               imageText.c_str(), CColor(0, 0, 0, 255));
          }
        }
      }
    }

    // draw a grid in lat/lon coordinates.
    if (dataSource->cfgLayer->Grid.size() == 1) {
      double gridSize = 10;
      double precision = 0.25;
      double numTestSteps = 5;
      CColor textColor(0, 0, 0, 128);
      float lineWidth = 0.25;
      int lineColor = 247;

      if (dataSource->cfgLayer->Grid[0]->attr.resolution.empty() == false) {
        gridSize = parseFloat(dataSource->cfgLayer->Grid[0]->attr.resolution.c_str());
      }
      precision = gridSize / 10;
      if (dataSource->cfgLayer->Grid[0]->attr.precision.empty() == false) {
        precision = parseFloat(dataSource->cfgLayer->Grid[0]->attr.precision.c_str());
      }

      bool useProjection = true;

      if (srvParam->geoParams.crs.equals("EPSG:4326")) {
        // CDBDebug("Not using projection");
        useProjection = false;
      }
      CImageWarper imageWarper;
      if (useProjection) {
#ifdef CIMAGEDATAWRITER_DEBUG
        CDBDebug("initreproj latlon");
#endif
        int status = imageWarper.initreproj(LATLONPROJECTION, drawImage.geoParams, &srvParam->cfg->Projection);
        if (status != 0) {
          CDBError("initreproj failed");
          return 1;
        }
      }

      f8point topLeft;
      f8box latLonBBOX;
      // Find lat lon BBox;
      topLeft.x = srvParam->geoParams.bbox.left;

      topLeft.y = srvParam->geoParams.bbox.bottom;
      if (useProjection) {
        imageWarper.reprojpoint(topLeft);
      }

      latLonBBOX.left = topLeft.x;
      latLonBBOX.right = topLeft.x;
      latLonBBOX.top = topLeft.y;
      latLonBBOX.bottom = topLeft.y;

      double numStepsX = (srvParam->geoParams.bbox.right - srvParam->geoParams.bbox.left) / numTestSteps;
      double numStepsY = (srvParam->geoParams.bbox.top - srvParam->geoParams.bbox.bottom) / numTestSteps;
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("dfBBOX: %f, %f, %f, %f", srvParam->geoParams.bbox.left, srvParam->geoParams.bbox.bottom, srvParam->geoParams.bbox.right, srvParam->geoParams.bbox.top);
#endif
      for (double y = srvParam->geoParams.bbox.bottom; y < srvParam->geoParams.bbox.top + numStepsY; y = y + numStepsY) {
        for (double x = srvParam->geoParams.bbox.left; x < srvParam->geoParams.bbox.right + numStepsX; x = x + numStepsX) {
#ifdef CIMAGEDATAWRITER_DEBUG
          CDBDebug("xy: %f, %f", x, y);
#endif
          topLeft.x = x;
          topLeft.y = y;
          if (useProjection) {
            imageWarper.reprojpoint(topLeft);
          }
          if (topLeft.x < latLonBBOX.left) latLonBBOX.left = topLeft.x;
          if (topLeft.x > latLonBBOX.right) latLonBBOX.right = topLeft.x;
          if (topLeft.y < latLonBBOX.top) latLonBBOX.top = topLeft.y;
          if (topLeft.y > latLonBBOX.bottom) latLonBBOX.bottom = topLeft.y;
        }
      }

#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("SIZE: %f, %f, %f, %f", latLonBBOX.left, latLonBBOX.right, latLonBBOX.top, latLonBBOX.bottom);
#endif

      latLonBBOX.left = double(int(latLonBBOX.left / gridSize)) * gridSize - gridSize;
      latLonBBOX.right = double(int(latLonBBOX.right / gridSize)) * gridSize + gridSize;
      latLonBBOX.top = double(int(latLonBBOX.top / gridSize)) * gridSize - gridSize;
      latLonBBOX.bottom = double(int(latLonBBOX.bottom / gridSize)) * gridSize + gridSize;

      int numPointsX = int((latLonBBOX.right - latLonBBOX.left) / precision);
      int numPointsY = int((latLonBBOX.bottom - latLonBBOX.top) / precision);
      numPointsX++;
      numPointsY++;

      size_t numPoints = numPointsX * numPointsY;

#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("numPointsX = %d, numPointsY = %d", numPointsX, numPointsY);
#endif

      f8point *gridP = new f8point[numPoints];

      for (int y = 0; y < numPointsY; y++) {
        for (int x = 0; x < numPointsX; x++) {
          double gx = latLonBBOX.left + precision * double(x);
          double gy = latLonBBOX.top + precision * double(y);
          size_t p = x + y * numPointsX;
          gridP[p].x = gx;
          gridP[p].y = gy;
          if (useProjection) {
            imageWarper.reprojpoint_inv(gridP[p]);
          }
          CoordinatesXYtoScreenXY(gridP[p], srvParam->geoParams);
        }
      }

#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Drawing horizontal lines");
#endif

      bool drawText = false;
      const char *fontLoc = NULL;
      float fontSize = 6.0;
      if (srvParam->cfg->WMS[0]->GridFont.size() == 1) {

        fontLoc = srvParam->cfg->WMS[0]->GridFont[0]->attr.location.c_str();
        fontSize = parseFloat(srvParam->cfg->WMS[0]->GridFont[0]->attr.size.c_str());
        drawText = true;
      }

      int s = int(gridSize / precision);
      if (s <= 0) s = 1;
      std::string message;
      for (int y = 0; y < numPointsY; y = y + s) {
        bool drawnTextLeft = false;
        bool drawnTextRight = false;
        for (int x = 0; x < numPointsX - 1; x++) {
          size_t p = x + y * numPointsX;
          if (p < numPoints) {
            drawImage.line(gridP[p].x, gridP[p].y, gridP[p + 1].x, gridP[p + 1].y, lineWidth, lineColor);
            if (drawnTextRight == false) {
              if (gridP[p].x > srvParam->geoParams.width && gridP[p].y > 0) {
                drawnTextRight = true;
                double gy = latLonBBOX.top + precision * double(y);
                message = CT::printf("%2.1f", gy);
                int ty = int(gridP[p].y);
                int tx = int(gridP[p].x);
                if (ty < 8) {
                  ty = 8;
                }
                if (tx > srvParam->geoParams.width - 30) tx = srvParam->geoParams.width - 1;
                tx -= 17;

                if (drawText) drawImage.drawText(tx, ty - 2, fontLoc, fontSize, 0, message.c_str(), textColor);
              }
            }
            if (drawnTextLeft == false) {
              if (gridP[p].x > 0 && gridP[p].y > 0) {
                drawnTextLeft = true;
                double gy = latLonBBOX.top + precision * double(y);
                message = CT::printf("%2.1f", gy);
                int ty = int(gridP[p].y);
                int tx = int(gridP[p].x);
                if (ty < 8) {
                  ty = 0;
                }
                if (tx < 15) tx = 0;
                tx += 2;
                if (drawText) drawImage.drawText(tx, ty - 2, fontLoc, fontSize, 0, message.c_str(), textColor);
              }
            }
          }
        }
      }

#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Drawing vertical lines");
#endif
      for (int x = 0; x < numPointsX; x = x + s) {
        bool drawnTextTop = false;
        bool drawnTextBottom = false;
        for (int y = numPointsY - 2; y >= 0; y--) {
          size_t p = x + y * numPointsX;
          if (p < numPoints) {
            drawImage.line(gridP[p].x, gridP[p].y, gridP[p + numPointsX].x, gridP[p + numPointsX].y, lineWidth, lineColor);

            if (drawnTextBottom == false) {
              if (gridP[p].x > 0 && gridP[p].y > srvParam->geoParams.height) {
                drawnTextBottom = true;
                double gx = latLonBBOX.left + precision * double(x);
                message = CT::printf("%2.1f", gx);
                int ty = int(gridP[p].y);
                if (ty < 15) ty = 0;
                if (ty > srvParam->geoParams.height) {
                  ty = srvParam->geoParams.height;
                }
                ty -= 2;
                int tx = int((gridP[p]).x + 2);
                if (drawText) drawImage.drawText(tx, ty, fontLoc, fontSize, 0, message.c_str(), textColor);
              }
            }

            if (drawnTextTop == false) {
              if (gridP[p].x > 0 && gridP[p].y > 0) {
                drawnTextTop = true;
                double gx = latLonBBOX.left + precision * double(x);
                message = CT::printf("%2.1f", gx);
                int ty = int(gridP[p].y);
                if (ty < 15) ty = 0;
                ty += 7;
                int tx = int(gridP[p].x) + 2; // if(tx<8){tx=8;ty+=4;}if(ty<15)tx=1;
                if (drawText) drawImage.drawText(tx, ty, fontLoc, fontSize, 0, message.c_str(), textColor);
              }
            }
          }
        }
      }

#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Delete gridp");
#endif

      delete[] gridP;
    }
  }
  return status;
}

CColor getColorForPlot(int plotNr, int nrOfPlots) {

  CColor color = CColor(255, 255, 255, 255);
  if (nrOfPlots < 6) {
    if (plotNr == 0) {
      color = CColor(0, 0, 255, 255);
    }
    if (plotNr == 1) {
      color = CColor(0, 255, 0, 255);
    }
    if (plotNr == 2) {
      color = CColor(255, 0, 0, 255);
    }
    if (plotNr == 3) {
      color = CColor(255, 128, 0, 255);
    }
    if (plotNr == 4) {
      color = CColor(0, 255, 128, 255);
    }
    if (plotNr == 5) {
      color = CColor(255, 0, 128, 255);
    }
    if (plotNr == 6) {
      color = CColor(0, 0, 128, 255);
    }
    if (plotNr == 7) {
      color = CColor(128, 0, 0, 255);
    }
    if (plotNr == 8) {
      color = CColor(0, 128, 0, 255);
    }
    if (plotNr == 9) {
      color = CColor(0, 128, 0, 255);
    }
    if (plotNr == 10) {
      color = CColor(0, 128, 128, 255);
    }
    if (plotNr == 11) {
      color = CColor(128, 128, 0, 255);
    }
  } else {
    color = CColor(0, 255, 0, 255);
  }
  return color;
}

int CImageDataWriter::end() {

  if (writerStatus == uninitialized) {
    CDBError("Not initialized");
    return 1;
  }
  if (writerStatus == finished) {
    CDBError("Already finished");
    return 1;
  }
  writerStatus = finished;
  if (srvParam->requestType == REQUEST_WMS_GETFEATUREINFO) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("end, number of GF results: %lu", getFeatureInfoResultList.size());
#endif
    enum ResultFormats { textplain, texthtml, textxml, applicationvndogcgml, imagepng, json, imagepng_eprofile };
    ResultFormats resultFormat = texthtml;

    if (srvParam->InfoFormat.equals("text/plain")) resultFormat = textplain;
    if (srvParam->InfoFormat.equals("text/xml")) resultFormat = textxml;
    if (srvParam->InfoFormat.equals("image/png")) resultFormat = imagepng;

    if (srvParam->InfoFormat.equals("application/vnd.ogc.gml")) resultFormat = applicationvndogcgml;

    if (isProfileData) {
      resultFormat = imagepng_eprofile;

      if (srvParam->InfoFormat.equals("image/png")) {
        printf("%s%s%c%c\n", "Content-Type:image/png", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
        drawImage.printImagePng8(true);
      } else {
        printf("%s%s%c%c\n", "Content-Type:application/json", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
        printf("%s", eProfileJson.c_str());
      }

      return 0;
    }

    if (srvParam->InfoFormat.indexOf("application/json") != -1) {

      try {
        if (gfiStructure.get("root") != NULL) {
          std::string data = gfiStructure.getList("root").toJSON(CXMLPARSER_JSONMODE_STANDARD);
          if (srvParam->JSONP.length() == 0) {
            printf("%s%s%c%c\n", "Content-Type: application/json", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
          } else {

            printf("%s%s%c%c", "Content-Type: application/javascript", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
            printf("\n%s(", srvParam->JSONP.c_str());
          }
          puts(data.c_str());
          if (srvParam->JSONP.length() != 0) {
            printf(");");
          }
          resetErrors();
          writerStatus = finished;
          return 0;
        }
      } catch (int e) {
      }
      resultFormat = json;
    }

    /* Text plain and text html */
    if (resultFormat == textplain || resultFormat == texthtml) {
      std::string resultHTML;
      if (resultFormat == textplain) {
        resultHTML = CT::printf("%s%s%c%c\n", "Content-Type: text/plain", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
      } else {
        resultHTML = CT::printf("%s%s%c%c\n", "Content-Type: text/html", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
      }

      if (resultFormat == texthtml) CT::printfconcat(resultHTML, "<html>\n");

      if (getFeatureInfoResultList.size() == 0) {
        CT::printfconcat(resultHTML, "Query returned no results");
      } else {
        GetFeatureInfoResult *g = &(getFeatureInfoResultList.at(0));
        if (resultFormat == texthtml) {
          CT::printfconcat(resultHTML, "Pointer Coordinates: (lon=%0.2f; lat=%0.2f)<br>\n", g->lon_coordinate, g->lat_coordinate);

        } else {
          CT::printfconcat(resultHTML, "Coordinates - (lon=%0.2f; lat=%0.2f)\n", g->lon_coordinate, g->lat_coordinate);
        }

        for (size_t j = 0; j < getFeatureInfoResultList.size(); j++) {

          GetFeatureInfoResult *g = &(getFeatureInfoResultList.at(j));

          if (resultFormat == texthtml) {
            CT::printfconcat(resultHTML, "<hr/><b>%s (%s)</b><br/>\n", g->layerTitle.c_str(), g->layerName.c_str());
          } else {
            CT::printfconcat(resultHTML, "%s (%s)\n", g->layerTitle.c_str(), g->layerName.c_str());
          }

          std::string currentTimeString = "";

          if (resultFormat == texthtml) {
            CT::printfconcat(resultHTML, "<table>");
          }
          for (size_t elNR = 0; elNR < g->elements.size(); elNR++) {
            GFIElement *e = &(g->elements[elNR]);

            if (resultFormat == texthtml) {
              CT::printfconcat(resultHTML, "<tr>");
            }
            if (resultFormat == texthtml) {
              CT::printfconcat(resultHTML, "<td>&nbsp;</td>");
            } else {
              CT::printfconcat(resultHTML, "  ");
            }
            if (g->elements.size() > 1) {
              if (resultFormat == texthtml) {
                CT::printfconcat(resultHTML, "<td>-</td>");
              } else {
                CT::printfconcat(resultHTML, "- ");
              }
            }
            if (e->value.length() > 0) {
              if (resultFormat == texthtml) {
                if (e->long_name == "pngdata") {
                  std::string newValue = "";
                  size_t color = std::stoi(e->value);
                  newValue = CT::printf("RGB: (%lu, %lu, %lu) / #%s / %s", color % 256, (color >> 8) % 256, (color >> 16) % 256, CT::getHex24(std::stoi(e->value)).c_str(), e->value.c_str());

                  CT::printfconcat(resultHTML, "<td>%s</td><td><b >%s</b><div style=\"background-color:#%s;width:100%%;height:30px;\"/></td>", e->long_name.c_str(), newValue.c_str(),
                                   CT::getHex24(std::stoi(e->value)).c_str());
                } else {

                  CT::printfconcat(resultHTML, "<td>%s</td><td><b>", e->long_name.c_str());
                  if (CT::isNumeric(e->value)) {
                    double value = std::stod(e->value);
                    if (fabs(value) > 0.1) {
                      CT::printfconcat(resultHTML, "%0.2f", value);
                    } else {
                      CT::printfconcat(resultHTML, "%g", value);
                    }
                  } else {
                    CT::printfconcat(resultHTML, "%s", e->value.c_str());
                  }
                  CT::printfconcat(resultHTML, "</b></td>");
                }

              } else {
                CT::printfconcat(resultHTML, "  %s %s", e->long_name.c_str(), e->value.c_str());
              }
              if (e->units.length() > 0) {
                if (e->value != "nodata" && e->value != "") {
                  if (resultFormat == texthtml) {
                    CT::printfconcat(resultHTML, "<td> %s</td>", e->units.c_str());
                  } else {
                    CT::printfconcat(resultHTML, " %s", e->units.c_str());
                  }
                }
              }
            }
            if (resultFormat == texthtml)
              CT::printfconcat(resultHTML, "</tr>\n");
            else
              CT::printfconcat(resultHTML, "\n");
          }
          if (resultFormat == texthtml) {
            CT::printfconcat(resultHTML, "</table>");
          }
        }
      }

      if (resultFormat == texthtml)
        CT::printfconcat(resultHTML, "</html>\n");
      else
        CT::printfconcat(resultHTML, "\n");
      resetErrors();

      printf("%s", resultHTML.c_str());
    } /*End of text html */

    /* Text XML */
    if (resultFormat == applicationvndogcgml) {
      CDBDebug("CREATING GML");
      std::string resultXML = CT::printf("%s%s%c%c\n", "Content-Type: text/xml", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
      CT::printfconcat(resultXML, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
      CT::printfconcat(resultXML, "  <FeatureCollection\n");
      CT::printfconcat(resultXML, "          xmlns:gml=\"http://www.opengis.net/gml\"\n");
      CT::printfconcat(resultXML, "          xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
      CT::printfconcat(resultXML, "          xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n");

      if (getFeatureInfoResultList.size() == 0) {
        CDBError("Query returned no results");
        return 1;
      } else {
        for (size_t j = 0; j < getFeatureInfoResultList.size(); j++) {
          GetFeatureInfoResult *g = &(getFeatureInfoResultList.at(j));
          auto layerName = CT::replace(CT::replace(CT::replace(g->layerName, ":", "-"), "/", "_"), " ", "_");
          CT::printfconcat(resultXML, "  <gml:featureMember>\n"); //, layerName.c_str());
          CDBDebug("GFI[%lu of %lu] %lu\n", j, getFeatureInfoResultList.size(), g->elements.size());
          for (size_t elNR = 0; elNR < g->elements.size(); elNR++) {
            GFIElement *e = &(g->elements[elNR]);
            std::string featureName = CT::replace(e->feature_name, " ", "_");
            CT::printfconcat(resultXML, "    <%s_feature>\n", featureName.c_str());

            CT::printfconcat(resultXML, "          <gml:pos>%f,%f</gml:pos>\n", g->lon_coordinate, g->lat_coordinate);
            CT::printfconcat(resultXML, "      <gml:location>\n");

            CT::printfconcat(resultXML, "        <gml:Point srsName=\"EPSG:4326\">\n");
            CT::printfconcat(resultXML, "          <gml:pos>%f,%f</gml:pos>\n", g->lon_coordinate, g->lat_coordinate);
            CT::printfconcat(resultXML, "        </gml:Point>\n");

            if (!srvParam->geoParams.crs.equals("EPSG:4326")) {
              CT::printfconcat(resultXML, "        <gml:Point srsName=\"%s\">\n", srvParam->geoParams.crs.c_str());
              CT::printfconcat(resultXML, "          <gml:pos>%f %f</gml:pos>\n", g->x_imageCoordinate, g->y_imageCoordinate);
              CT::printfconcat(resultXML, "        </gml:Point>\n");
            }

            CT::printfconcat(resultXML, "        <gml:Point srsName=\"%s\">\n", "raster:coordinates");
            CT::printfconcat(resultXML, "          <gml:pos>%f %f</gml:pos>\n", g->x_rasterCoordinate, g->y_rasterCoordinate);
            CT::printfconcat(resultXML, "        </gml:Point>\n");

            CT::printfconcat(resultXML, "        <gml:Point srsName=\"%s\">\n", "raster:xyindices");
            CT::printfconcat(resultXML, "          <gml:pos>%d %d</gml:pos>\n", g->x_rasterIndex, g->y_rasterIndex);
            CT::printfconcat(resultXML, "        </gml:Point>\n");

            CT::printfconcat(resultXML, "      </gml:location>\n");
            CT::printfconcat(resultXML, "      <FeatureName>%s</FeatureName>\n", featureName.c_str());
            CT::printfconcat(resultXML, "      <StandardName>%s</StandardName>\n", e->standard_name.c_str());
            CT::printfconcat(resultXML, "      <LongName>%s</LongName>\n", e->long_name.c_str());
            CT::printfconcat(resultXML, "      <VarName>%s</VarName>\n", e->var_name.c_str());
            CT::printfconcat(resultXML, "      <Value units=\"%s\">%s</Value>\n", e->units.c_str(), e->value.c_str());
            for (size_t d = 0; d < e->cdfDims.size(); d++) {
              CT::printfconcat(resultXML, "      <Dimension name=\"%s\" index=\"%lu\">%s</Dimension>\n", e->dataSource->requiredDims[d].name.c_str(), e->cdfDims.at(d).index,
                               e->cdfDims.at(d).value.c_str());
            }

            CT::printfconcat(resultXML, "    </%s_feature>\n", featureName.c_str());
          }
          CT::printfconcat(resultXML, "  </gml:featureMember>\n"); //, layerName.c_str());
        }
      }
      CT::printfconcat(resultXML, " </FeatureCollection>\n");
      resetErrors();
      printf("%s", resultXML.c_str());
    } /* End of applicationvndogcgml */

    if (resultFormat == textxml) {
      CDBDebug("CREATING XML");
      std::string resultXML = CT::printf("%s%c%c\n", "Content-Type:text/xml", 13, 10);
      CT::printfconcat(resultXML, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
      CT::printfconcat(resultXML, " <GMLOutput\n");
      CT::printfconcat(resultXML, "          xmlns:gml=\"http://www.opengis.net/gml\"\n");
      CT::printfconcat(resultXML, "          xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
      CT::printfconcat(resultXML, "          xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n");

      if (getFeatureInfoResultList.size() == 0) {
        CDBError("Query returned no results");
        return 1;
      } else {
        for (size_t j = 0; j < getFeatureInfoResultList.size(); j++) {
          GetFeatureInfoResult *g = &(getFeatureInfoResultList[j]);
          auto layerName = CT::replace(CT::replace(CT::replace(g->layerName, ":", "-"), "/", "_"), " ", "_");

          CT::printfconcat(resultXML, "  <%s_layer>\n", layerName.c_str());
          CDBDebug("GFI[%lu of %lu] %lu\n", j, getFeatureInfoResultList.size(), g->elements.size());
          for (size_t elNR = 0; elNR < g->elements.size(); elNR++) {
            GFIElement *e = &(g->elements[elNR]);
            std::string featureName = CT::replace(e->feature_name, " ", "_");
            CT::printfconcat(resultXML, "    <%s_feature>\n", featureName.c_str());
            CT::printfconcat(resultXML, "      <gml:location>\n");

            CT::printfconcat(resultXML, "        <gml:Point srsName=\"EPSG:4326\">\n");
            CT::printfconcat(resultXML, "          <gml:pos>%f,%f</gml:pos>\n", g->lon_coordinate, g->lat_coordinate);
            CT::printfconcat(resultXML, "        </gml:Point>\n");

            if (!srvParam->geoParams.crs.equals("EPSG:4326")) {
              CT::printfconcat(resultXML, "        <gml:Point srsName=\"%s\">\n", srvParam->geoParams.crs.c_str());
              CT::printfconcat(resultXML, "          <gml:pos>%f %f</gml:pos>\n", g->x_imageCoordinate, g->y_imageCoordinate);
              CT::printfconcat(resultXML, "        </gml:Point>\n");
            }
            CT::printfconcat(resultXML, "        <gml:Point srsName=\"%s\">\n", "raster:coordinates");
            CT::printfconcat(resultXML, "          <gml:pos>%f %f</gml:pos>\n", g->x_rasterCoordinate, g->y_rasterCoordinate);
            CT::printfconcat(resultXML, "        </gml:Point>\n");

            CT::printfconcat(resultXML, "        <gml:Point srsName=\"%s\">\n", "raster:xyindices");
            CT::printfconcat(resultXML, "          <gml:pos>%d %d</gml:pos>\n", g->x_rasterIndex, g->y_rasterIndex);
            CT::printfconcat(resultXML, "        </gml:Point>\n");

            CT::printfconcat(resultXML, "      </gml:location>\n");
            CT::printfconcat(resultXML, "      <FeatureName>%s</FeatureName>\n", featureName.c_str());
            CT::printfconcat(resultXML, "      <StandardName>%s</StandardName>\n", e->standard_name.c_str());
            CT::printfconcat(resultXML, "      <LongName>%s</LongName>\n", e->long_name.c_str());
            CT::printfconcat(resultXML, "      <VarName>%s</VarName>\n", e->var_name.c_str());
            CT::printfconcat(resultXML, "      <Value units=\"%s\">%s</Value>\n", e->units.c_str(), e->value.c_str());
            for (size_t d = 0; d < e->cdfDims.size(); d++) {
              CT::printfconcat(resultXML, "      <Dimension name=\"%s\" index=\"%lu\">%s</Dimension>\n", e->dataSource->requiredDims[d].name.c_str(), e->cdfDims.at(d).index,
                               e->cdfDims.at(d).value.c_str());
            }

            CT::printfconcat(resultXML, "    </%s_feature>\n", featureName.c_str());
          }
          CT::printfconcat(resultXML, "  </%s_layer>\n", layerName.c_str());
        }
      }
      CT::printfconcat(resultXML, " </GMLOutput>\n");
      resetErrors();
      printf("%s", resultXML.c_str());

    } /* End of text/xml */

    if (resultFormat == json) {
      std::string resultJSON;
      if (srvParam->JSONP.length() == 0) {
        CDBDebug("CREATING JSON");
        resultJSON = CT::printf("%s%s%c%c\n", "Content-Type: application/json", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
      } else {
        CDBDebug("CREATING JSONP %s", srvParam->JSONP.c_str());
        resultJSON = CT::printf("%s%s%c%c\n", "Content-Type: application/javascript", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
      }

      CXMLParser::XMLElement rootElement;

      rootElement.setName("root");

      for (size_t j = 0; j < getFeatureInfoResultList.size(); j++) {
        // CDBDebug("gfi len: %d of %d (%d el)\n", j, getFeatureInfoResultList.size(), getFeatureInfoResultList[j]->elements.size());
        GetFeatureInfoResult *g = &(getFeatureInfoResultList[j]);

        // Find out number of different features in getFeatureInfoResultList[j]
        std::vector<std::string> features;

        for (size_t elNr = 0; elNr < g->elements.size(); elNr++) {
          GFIElement &element = g->elements[elNr];
          bool featureNameFound = false;
          for (size_t jj = 0; jj < features.size(); jj++) {
            if (features[jj] == element.feature_name) {
              featureNameFound = true;
              break;
            }
          }
          if (!featureNameFound) {
            features.push_back(element.feature_name.c_str());
          } else {
            break;
          }
        }
        int nrFeatures = features.size();

        // Find available dimensions

        GFIElement *e = &(g->elements[0]);

        int nrDims = e->cdfDims.size();
        std::vector<int> dimLookup(nrDims); // position of each dimension in cdfDims.dimensions
        // CDBDebug("nrDims = %d",nrDims);
        int timeDimIndex = -1;
        int endIndex = nrDims - 1;
        for (int j = 0; j < nrDims; j++) {
          dimLookup[j] = j;
          if (timeDimIndex == -1 && isOGCTimeDim(e->cdfDims.at(j))) {
            timeDimIndex = j;
          }
        }

        if (timeDimIndex != -1) {
          if (timeDimIndex != endIndex) {
            int a = dimLookup[timeDimIndex];
            int b = dimLookup[endIndex];
            dimLookup[timeDimIndex] = b;
            dimLookup[endIndex] = a;
          }
        }

        for (int feat = 0; feat < nrFeatures; feat++) {
          GFIElement *e = &(g->elements[feat]);
          auto &paramElement = rootElement.add("param");
          paramElement.add("name", e->var_name);
          paramElement.add("layername", g->layerName);
          paramElement.add("variablename", e->var_name);
          paramElement.add("standard_name", e->standard_name);
          paramElement.add("feature_name", e->feature_name);
          paramElement.add("units", e->units);
          auto &point = paramElement.add("point");
          point.add("SRS", "EPSG:4326");
          point.add("coords", CT::printf("%f,%f", g->lon_coordinate, g->lat_coordinate));
          for (size_t d = 0; d < e->cdfDims.size() && int(d) < nrDims; d++) {
            paramElement.add("dims", e->dataSource->requiredDims[dimLookup[d]].name);
          }

          // Build datamap
          std::map<std::string, std::string> dataMap;
          for (size_t elNR = feat; elNR < g->elements.size(); elNR += nrFeatures) {
            GFIElement *e = &(g->elements[elNR]);
            std::string dimString;
            for (size_t d = 0; d < e->cdfDims.size(); d++) {
              CT::printfconcat(dimString, "%s,", e->cdfDims.at(dimLookup[d]).value.c_str());
            }
            dataMap[dimString] = e->value;
          }

          auto &data = paramElement.add("data");
          for (auto dataMapIter = dataMap.begin(); dataMapIter != dataMap.end(); ++dataMapIter) {
            auto nestedData = &data;
            // Loop trough the dim values and make a nested structure
            for (const auto &dimValue: CT::split(dataMapIter->first, ",")) {
              auto it = std::find_if(nestedData->xmlElements.begin(), nestedData->xmlElements.end(), [&dimValue](CXMLParser::XMLElement &element) { return element.name == dimValue; });
              nestedData = it == nestedData->xmlElements.end() ? &(nestedData->add(dimValue)) : &(*it); // Check if we have this already or if we have to make a new
            }
            nestedData->value = dataMapIter->second; // Assign value
          }
        }
      }

      resetErrors();
      if (srvParam->JSONP.length() == 0) {
        resultJSON += rootElement.getList("param").toJSON(CXMLPARSER_JSONMODE_STANDARD).c_str();
      } else {
        resultJSON += srvParam->JSONP.c_str();
        resultJSON += "(";
        resultJSON += rootElement.getList("param").toJSON(CXMLPARSER_JSONMODE_STANDARD).c_str();
        resultJSON += ");";
      }
      printf("%s", resultJSON.c_str());

    } /* End of json */

    getFeatureInfoResultList.clear();
  } /* End of getfeatureInfo */
  if (srvParam->requestType != REQUEST_WMS_GETMAP && srvParam->requestType != REQUEST_WMS_GETLEGENDGRAPHIC) return 0;

  // Output WMS getmap results
  if (errorsOccured()) {

    CREPORT_ERROR_NODOC(std::string("Error occured during image data writing"), CReportMessage::Categories::GENERAL);
    return 1;
  }

#ifdef MEASURETIME
  StopWatch_Stop("Drawing finished, start printing image");
#endif

  // Static image
  // CDBDebug("srvParam->imageFormat = %d",srvParam->imageFormat);
  int status = 1;

  std::string cacheControl = srvParam->getResponseHeaders(srvParam->getCacheControlOption());
  if (srvParam->imageFormat == IMAGEFORMAT_IMAGEPNG8) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Creating 8 bit png with alpha");
#endif
    printf("%s%s%c%c\n", "Content-Type:image/png\r\nContent-Encoding:png8a", cacheControl.c_str(), 13, 10);
    status = drawImage.printImagePng8(true);
  } else if (srvParam->imageFormat == IMAGEFORMAT_IMAGEPNG8_NOALPHA) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Creating 8 bit png without alpha");
#endif
    printf("%s%s%c%c\n", "Content-Type:image/png\r\nContent-Encoding:png8", cacheControl.c_str(), 13, 10);
    status = drawImage.printImagePng8(false);
  } else if (srvParam->imageFormat == IMAGEFORMAT_IMAGEPNG24) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Creating 24 bit png");
#endif
    printf("%s%s%c%c\n", "Content-Type:image/png", cacheControl.c_str(), 13, 10);
    status = drawImage.printImagePng24();
  } else if (srvParam->imageFormat == IMAGEFORMAT_IMAGEPNG32) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Creating 32 bit png");
#endif
    printf("%s%s%c%c\n", "Content-Type:image/png", cacheControl.c_str(), 13, 10);
    status = drawImage.printImagePng32();
  } else if (srvParam->imageFormat == IMAGEFORMAT_IMAGEWEBP) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("Creating webp");
#endif
    printf("%s%s%c%c\n", "Content-Type:image/webp\r\nContent-Encoding:webp", cacheControl.c_str(), 13, 10);

    int webPQuality = srvParam->imageQuality;
    if (!srvParam->Format.empty()) {
      /* Support setting quality via wms format parameter, e.g. format=image/webp;90& */
      auto s = srvParam->Format.split(";");
      if (s.size() > 1) {
        int q = s[1].toInt();
        if (q >= 0 && q <= 100) {
          webPQuality = q;
        }
      }
    }
    CDBDebug("Creating 32 bit webp quality = %d", webPQuality);
    status = drawImage.printImageWebP32(webPQuality);
  } else {
    // CDBDebug("LegendGraphic PNG");
    printf("%s%s%c%c\n", "Content-Type:image/png", cacheControl.c_str(), 13, 10);
    status = drawImage.printImagePng8(true);
  }

#ifdef MEASURETIME
  StopWatch_Stop("Image printed");
#endif
  if (status != 0) {
    CDBError("Errors occured during image printing");
  }
  return status;
}
float CImageDataWriter::getValueForColorIndex(CDataSource *dataSource, int index) {

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (styleConfiguration->legendScale != styleConfiguration->legendScale || styleConfiguration->legendScale == INFINITY || styleConfiguration->legendScale == NAN ||
      styleConfiguration->legendScale == 0.0 || styleConfiguration->legendScale == -INFINITY || styleConfiguration->legendOffset != styleConfiguration->legendOffset ||
      styleConfiguration->legendOffset == INFINITY || styleConfiguration->legendOffset == NAN || styleConfiguration->legendOffset == -INFINITY) {
    styleConfiguration->legendScale = 240.0;
    styleConfiguration->legendOffset = 0;
  }

  float v = index;
  v -= styleConfiguration->legendOffset;
  v /= styleConfiguration->legendScale;
  if (styleConfiguration->legendLog != 0) {
    v = pow(styleConfiguration->legendLog, v);
  }
  return v;
}
int CImageDataWriter::getColorIndexForValue(CDataSource *dataSource, float value) {
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  float val = value;
  if (styleConfiguration->legendLog != 0) val = log10(val + .000001) / log10(styleConfiguration->legendLog);
  val *= styleConfiguration->legendScale;
  val += styleConfiguration->legendOffset;
  if (val >= 239)
    val = 239;
  else if (val < 0)
    val = 0;
  return int(val);
}

CColor CImageDataWriter::getPixelColorForValue(CDataSource *dataSource, float val) {
  bool isNodata = false;

  CColor color;
  if (dataSource->getFirstAvailableDataObject()->hasNodataValue) {
    if (val == float(dataSource->getFirstAvailableDataObject()->dfNodataValue)) isNodata = true;
    if (!(val == val)) isNodata = true;
  }
  if (!isNodata) {
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    for (size_t j = 0; j < styleConfiguration->shadeIntervals.size(); j++) {
      const auto &shadeInterval = styleConfiguration->shadeIntervals[j];
      if (shadeInterval.attr.min.empty() == false && shadeInterval.attr.max.empty() == false) {
        if ((val >= atof(shadeInterval.attr.min.c_str())) && (val < atof(shadeInterval.attr.max.c_str()))) {
          return CColor(shadeInterval.attr.fillcolor.c_str());
        }
      }
    }
  }
  return color;
}

int CImageDataWriter::createLegend(CDataSource *dataSource, CDrawImage *legendImage) { return CCreateLegend::createLegend(dataSource, legendImage); }

int CImageDataWriter::createLegend(CDataSource *dataSource, CDrawImage *legendImage, bool rotate) { return CCreateLegend::createLegend(dataSource, legendImage, rotate); }

int CImageDataWriter::drawText(int x, int y, const char *fontlocation, float size, float angle, const char *text, unsigned char colorIndex) {
  drawImage.drawText(x, y, fontlocation, size, angle, text, colorIndex);
  return 0;
}

int CImageDataWriter::createScaleBar(GeoParameters &geoParams, CDrawImage *scaleBarImage, float scaling) { return CCreateScaleBar::createScaleBar(scaleBarImage, geoParams, scaling); }
