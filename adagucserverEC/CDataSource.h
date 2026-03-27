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

#ifndef CDATASOURCE_H
#define CDATASOURCE_H

#include <cstddef>
#include "CCDFObject.h"
#include "CStyleConfiguration.h"
#include "Types/CPointTypes.h"
#include "COGCDims.h"
#include "Types/GeoParameters.h"
#include "CServerParams.h"
#include "utils/minMax.h"
#include "utils/KeyValuePair.h"

// Forward declaration
struct CStyleConfiguration;

struct StatusFlag {
  std::string meaning;
  double value;
};

struct TimeStep {
  std::string fileName; // Filename of the file to load
  CCDFDims dims;        // Dimension index in the corresponding name and file
};
struct DataObject {
  bool hasStatusFlag = false;
  bool hasNodataValue = false;
  bool noFurtherProcessing = false;
  bool filterFromOutput = false; // When set to true, this dataobject is not returned in the GetFeatureInfo response.
  double dfNodataValue;
  std::string dataObjectName;
  std::string variableName;
  std::string overruledUnits;
  std::vector<StatusFlag> statusFlagList;
  std::vector<PointDVWithLatLon> points;
  std::map<int, CFeature> features;
  CDF::Variable *cdfVariable = nullptr; // Not owned!
  CDFObject *cdfObject = nullptr;       // Not owned!
};

std::string dObjGetStdName(const DataObject &dataOject);
std::string dObjgetVariableName(const DataObject &dataOject);
std::string dObjgetUnits(const DataObject &dataOject);
void dObjsetUnits(const DataObject &dataOject, std::string units);

/**
 * This class represents data to be used further in the server. Specific  metadata and data is filled in by CDataReader
 * This class is used for both image drawing (WMS) and data output (WCS)
 */
class CDataSource {
public:
  bool debug = false;
  CT::string headerFilename;

  bool dimsAreAutoConfigured = -1;

private:
  bool currentStyleSet = false;
  CStyleConfiguration currentStyle;
  std::vector<CStyleConfiguration> styleConfigurationList;
  std::vector<std::string> getLegendListForDataSource(CServerConfig::XMLE_Style *style);
  std::vector<std::string> getStyleNames(std::vector<CServerConfig::XMLE_Styles *> Styles);
  std::vector<std::string> getRenderMethodListForDataSource(CServerConfig::XMLE_Style *style);

public:
  CStyleConfiguration *getStyle();

  int datasourceIndex = 0;
  size_t currentAnimationStep = 0;
  int threadNr = -1;
  /**
   * The amount of steps in this datasource
   */
  std::vector<TimeStep> timeSteps;

  /**
   * Returns the value for a certain dimension and step
   */
  std::string getDimensionValueForNameAndStep(const char *dimName, int dimStep);

  std::vector<DataObject> dataObjects;

  bool stretchMinMax = false;

  /**
   * The required dimensions for this datasource
   */
  std::vector<COGCDims> requiredDims;
  Statistics *statistics = nullptr; // is NULL when not available
  // The actual dataset data (can have multiple variables)

  // source image parameters
  double dfBBOX[4], dfCellSizeX, dfCellSizeY;
  int dWidth, dHeight;
  CT::string nativeEPSG;
  CT::string nativeProj4;

  // TODO KVP and metaDataItems can be moved out to GDAL datawriter

  std::vector<KeyValuePair> metaDataItems;

  // Configured?
  bool isConfigured = false;

  // Used for vectors and points
  bool formatConverterActive = false;

  // Some projections require the scaling of the axis with a certain number, like converting km to meter
  bool didAxisScalingConversion = false;

  // The index of the X and Y dimension in the variable dimensionlist (not the id's from the netcdf file)
  int dimXIndex;
  int dimYIndex;

  // The striding of the read 2D map
  int stride2DMap;

  // Lon transformation is used to swap datasets from 0-360 degrees to -180 till 180 degrees
  // Swap data from >180 degrees to domain of -180 till 180 in case olonTransformDone lat lon source data
  int useLonTransformation = -1;
  double origBBOXLeft, origBBOXRight;
  int dOrigWidth = -1;
  bool lonTransformDone = false;

  // Sometimes X and Y need to be swapped, this boolean indicates whether it should or not.
  bool swapXYDimensions = false;

  // X and Y variables of the 2D field
  CDF::Variable *varX = nullptr;
  CDF::Variable *varY = nullptr;

  // Numver of dims
  int dNetCDFNumDims;
  int dLayerType;
  CT::string layerName;
  CT::string layerTitle;

  //
  bool queryBBOX = false; // True: query on viewport
  f8box nativeViewPortBBOX;
  int queryLevel = 0;

  // Current value index of the dim
  // int dOGCDimValues[MAX_DIMS];

  CServerParams *srvParams = nullptr;

  // Link to the XML configuration
  CServerConfig::XMLE_Layer *cfgLayer = nullptr;
  CServerConfig::XMLE_Configuration *cfg = nullptr;

  CT::string featureSet;
  CDataSource();
  CDataSource(CServerParams *srvParams, CServerConfig::XMLE_Layer *cfgLayer, int layerIndex = -1);
  ~CDataSource();

  // TODO
  static void readStatusFlags(CDF::Variable *var, std::vector<StatusFlag> &statusFlagList);
  static std::string getFlagMeaning(std::vector<StatusFlag> &statusFlagList, double value);
  static std::string getFlagMeaningHumanReadable(std::vector<StatusFlag> &statusFlagList, double value);

  int setCFGLayer(CServerParams *_srvParams, CServerConfig::XMLE_Layer *_cfgLayer, int layerIndex);
  void addStep(const char *fileName);
  std::string getFileName();
  void setHeaderFilename(CT::string headerFileName);
  void setGeo(GeoParameters &geo);
  GeoParameters getGeo();

  DataObject *getDataObjectByName(const char *name);
  DataObject *getDataObjectByName(std::string name);
  DataObject *getDataObject(int j);
  DataObject *getFirstAvailableDataObject();

  size_t getNumDataObjects() { return dataObjects.size(); }
  void eraseDataObject(int j) { dataObjects.erase(dataObjects.begin() + j); }
  void setTimeStep(size_t timeStep);
  int getCurrentTimeStep();
  size_t getDimensionIndex(const char *name);
  size_t getDimensionIndex(int i);
  std::string getDimensionValue(int i);
  CCDFDims *getCDFDims();
  int getNumTimeSteps();
  const char *getLayerName();
  const char *getLayerTitle();

  int attachCDFObject(CDFObject *cdfObject);
  void detachCDFObject();
  CDataSource *clone();
  const std::vector<CStyleConfiguration> &getStyleListForDataSource();

  /**
   * Sets the style by name, can be a character string.
   * @param styleName The name of the style
   * returns zero on success.
   */
  int setStyle(const char *styleName);

  /**
   * Returns the amount of need image scaling for elements like title and legend. This can be the case if the scalewidth property in the RenderSettings is set
   */
  double getScaling();

  /**
   * Returns the amount of need image map scaling for elements lice contours. This can be the case if the scalecontours property in the RenderSettings is set
   */
  double getContourScaling();

  /**
   * Reads a variable with requested type according dimensions indices set in the DataSource CDFDims object
   * @param CDF::Variable *variableToRead: The variable to read. Can be the one from the datasource itself
   * @param CDFType dataTypeToReturnData Type to read
   * @return 0 on succes, 1 on failure
   */
  int readVariableDataForCDFDims(CDF::Variable *variableToRead, CDFType dataTypeToReturnData);

  std::string getDataSetName();

  GeoParameters makeGeoParams();
};

#endif
