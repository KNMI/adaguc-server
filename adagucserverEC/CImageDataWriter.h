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

#ifndef CImageDataWriter_H
#define CImageDataWriter_H
#include <string>
#include <map>
#include <vector>
#include "Definitions.h"
#include "CStopWatch.h"
#include "CIBaseDataWriterInterface.h"
#include "CImgWarpNearestNeighbour.h"
#include "CImgWarpNearestRGBA.h"
#include "CImgWarpBilinear.h"
#include "CImgWarpBoolean.h"
#include "CImgRenderers/CImgRenderPoints.h"
#include "CImgRenderStippling.h"
#include "CImgRenderPolylines.h"
#include "CStyleConfiguration.h"
#include "CMyCURL.h"
#include "CXMLParser.h"
#include "CDebugger.h"

class CImageDataWriter : public CBaseDataWriterInterface {
public:
  class ProjCacheInfo {
  public:
    double CoordX, CoordY;
    double nativeCoordX, nativeCoordY;
    double lonX, lonY;
    int imx, imy;
    int dWidth, dHeight;
    double dX, dY;
    bool isOutsideBBOX;
  };

  class IndexRange {
  public:
    int min;
    int max;
    IndexRange(int min, int max) {
      this->min = min;
      this->max = max;
    }
    IndexRange();
  };
  std::vector<CImageDataWriter::IndexRange> getIndexRangesForRegex(CT::string match, CT::string *attributeValues, int n);
  static std::map<std::string, CImageDataWriter::ProjCacheInfo> projCacheMap;
  static std::map<std::string, CImageDataWriter::ProjCacheInfo>::iterator projCacheIter;
  static ProjCacheInfo GetProjInfo(CT::string ckey, CDrawImage *drawImage, CDataSource *dataSource, CImageWarper *imageWarper, CServerParams *srvParam, int dX, int dY);

private:
  // CImageWarper imageWarper;
  // CDataSource *currentDataSource;
  // int requestType;
  // int status;
  int animation;
  int nrImagesAdded;
  CT::string eProfileJson;

public:
public:
  class GetFeatureInfoResult {
  public:
    ~GetFeatureInfoResult() {
      for (size_t j = 0; j < elements.size(); j++) delete elements[j];
    }

    int x_imagePixel, y_imagePixel;
    double x_imageCoordinate, y_imageCoordinate;
    double x_rasterCoordinate, y_rasterCoordinate;
    int x_rasterIndex, y_rasterIndex;
    double lat_coordinate, lon_coordinate;

    CT::string layerName;
    CT::string layerTitle;
    int dataSourceIndex;

    class Element {
    public:
      CT::string value;
      CT::string units;
      CT::string standard_name;
      CT::string feature_name;
      CT::string long_name;
      CT::string var_name;
      // CT::string time;
      CDF::Variable *variable;
      CDataSource *dataSource;
      CCDFDims cdfDims;
    };
    std::vector<Element *> elements;
  };

private:
  static int getTextForValue(CT::string *tv, float v, CStyleConfiguration *styleConfiguration);
  std::vector<GetFeatureInfoResult *> getFeatureInfoResultList;
  CXMLParser::XMLElement gfiStructure;
  DEF_ERRORFUNCTION();

  int warpImage(CDataSource *sourceImage, CDrawImage *drawImage);

  CServerParams *srvParam;

  enum ImageDataWriterStatus { uninitialized, initialized, finished };
  ImageDataWriterStatus writerStatus;
  // float shadeInterval,contourIntervalL,contourIntervalH;

  // int smoothingFilter;
  // RenderMethodEnum renderMethod;
public:
  static double convertValue(CDFType type, void *data, size_t p);
  static int getColorIndexForValue(CDataSource *dataSource, float value);
  static float getValueForColorIndex(CDataSource *dataSource, int index);
  static CColor getPixelColorForValue(CDataSource *dataSource, float val);

private:
  void setValue(CDFType type, void *data, size_t ptr, double pixel);
  int _setTransparencyAndBGColor(CServerParams *srvParam, CDrawImage *drawImage);

  int drawCascadedWMS(CDataSource *dataSource, const char *service, const char *layers, const char *styles, bool transparent, const char *bgcolor);

  bool isProfileData;

  /* Loops over the points, calculates the closest points, then calculates if point is within specified range in pixels */
  void getFeatureInfoGetPointDataResults(CDataSource *dataSource, CImageDataWriter::GetFeatureInfoResult *getFeatureInfoResult, int dataObjectNrInDataSource, GetFeatureInfoResult::Element *element,
                                         int maxPixelDistance);

public:
  CDrawImage drawImage;

  CImageDataWriter();
  ~CImageDataWriter() {
    for (size_t j = 0; j < getFeatureInfoResultList.size(); j++) {
      delete getFeatureInfoResultList[j];
      getFeatureInfoResultList[j] = NULL;
    }
    getFeatureInfoResultList.clear();
    // delete currentStyleConfiguration;currentStyleConfiguration = NULL;
  }

  static int createLegend(CDataSource *sourceImage, CDrawImage *legendImage);
  static int createLegend(CDataSource *sourceImage, CDrawImage *legendImage, bool rotate);

  static int createScaleBar(CGeoParams *geoParams, CDrawImage *scaleBarImage, float scaling);

  int getFeatureInfo(std::vector<CDataSource *> &dataSources, int dataSourceIndex, int dX, int dY);
  int createAnimation();
  void setDate(const char *date);
  int calculateData(std::vector<CDataSource *> &dataSources);

  // Virtual functions
  int init(CServerParams *srvParam, CDataSource *dataSource, int nrOfBands);
  int addData(std::vector<CDataSource *> &dataSources);
  int end();
  int drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, unsigned char colorIndex);
};

#endif
