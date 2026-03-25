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

#include "CDataSource.h"
#include "CDBFileScanner.h"
#include "CConvertGeoJSON.h"
#include "utils/LayerUtils.h"

// #define CDATASOURCE_DEBUG

bool debugDataSource = false;
bool configWarningSet = false;

DataObject::DataObject() {
  hasStatusFlag = false;
  cdfVariable = NULL;
  cdfObject = NULL;
  std::vector<f8point> points;
}

DataObject *DataObject::clone() {
  DataObject *nd = new DataObject();
  nd->hasStatusFlag = hasStatusFlag;
  nd->hasNodataValue = hasNodataValue;
  nd->dfNodataValue = dfNodataValue;
  nd->cdfObject = cdfObject;
  nd->overruledUnits = overruledUnits;
  nd->variableName = variableName;
  nd->cdfVariable = nullptr;
  return nd;
}

CT::string DataObject::getUnits() {
  if (overruledUnits.empty() && cdfVariable != NULL) {
    try {
      return cdfVariable->getAttributeThrows("units")->toString();
    } catch (int e) {
    }
  }
  return overruledUnits;
}

CT::string DataObject::getStandardName() {
  CT::string standard_name = variableName;
  CDF::Attribute *standardNameAttr = cdfVariable->getAttributeNE("standard_name");
  if (standardNameAttr != nullptr) {
    standard_name = standardNameAttr->toString();
  }
  return standard_name;
}

void DataObject::setUnits(CT::string units) { overruledUnits = units; }

CDataSource::CDataSource() {
  stretchMinMax = false;
  isConfigured = false;
  threadNr = -1;
  dimsAreAutoConfigured = false;
  statistics = NULL;
  currentAnimationStep = 0;
  srvParams = NULL;
  cfgLayer = NULL;
  cfg = NULL;
  datasourceIndex = 0;
  formatConverterActive = false;
  useLonTransformation = -1;
  dOrigWidth = -1;
  lonTransformDone = false;
  swapXYDimensions = false;
  varX = NULL;
  varY = NULL;

  queryBBOX = false;
  queryLevel = 0; // -1 Means query all. 0 means query all non tiled versions. Positive number means a tiled version.
}

CDataSource::~CDataSource() {

  for (size_t d = 0; d < dataObjects.size(); d++) {
    if (dataSourceOwnsDataObject) {
      delete (CDFReader *)dataObjects[d]->cdfObject->getCDFReader();
      delete dataObjects[d]->cdfObject;
      dataObjects[d]->cdfObject = nullptr;
    }
    delete dataObjects[d];
    dataObjects[d] = NULL;
  }

  dataSourceOwnsDataObject = false;
  dataObjects.clear();

  for (size_t j = 0; j < timeSteps.size(); j++) {

    delete timeSteps[j];
    timeSteps[j] = NULL;
  }
  requiredDims.clear();
  if (statistics != NULL) {
    delete statistics;
  };
  statistics = NULL;

  if (featureSet.length() != 0) {
    CConvertGeoJSON::clearFeatureStore(featureSet);
  }
}

int CDataSource::setCFGLayer(CServerParams *_srvParams, CServerConfig::XMLE_Layer *_cfgLayer, int layerIndex) {
  srvParams = _srvParams;
  cfg = srvParams->cfg;
  cfgLayer = _cfgLayer;
  datasourceIndex = layerIndex;

  if (this->dataObjects.size() > 0) {
    CDBWarning("DataSource already has dataobjects, cannot setCFGLayer");
    return 1;
  }
  // Make DataObjects for each Variable defined in the Layer.
  for (size_t j = 0; j < cfgLayer->Variable.size(); j++) {
    DataObject *newDataObject = new DataObject();
    newDataObject->variableName.copy(cfgLayer->Variable[j]->value.c_str());
    this->dataObjects.push_back(newDataObject);
  }

  // Set the layername
  layerName = makeUniqueLayerName(cfgLayer);

  layerTitle = cfgLayer->Title.size() > 0 && !cfgLayer->Title[0]->value.empty() ? cfgLayer->Title[0]->value.c_str() : layerName.c_str();

  if (debugDataSource) {
    CDBDebug("LayerName=\"%s\"", layerName.c_str());
  }
  // Defaults to database
  dLayerType = CConfigReaderLayerTypeDataBase;
  if (cfgLayer->attr.type.equals("database")) {
    dLayerType = CConfigReaderLayerTypeDataBase;
  } else if (cfgLayer->attr.type.equals("grid")) {
    dLayerType = CConfigReaderLayerTypeGraticule;
  } else if (cfgLayer->attr.type.equals("autoscan")) {
    dLayerType = CConfigReaderLayerTypeUnknown;
  } else if (cfgLayer->attr.type.equals("baselayer")) {
    dLayerType = CConfigReaderLayerTypeBaseLayer;
  } else if (cfgLayer->attr.type.equals("liveupdate")) {
    dLayerType = CConfigReaderLayerTypeLiveUpdate;
  } else if (cfgLayer->attr.type.empty() == false) {
    if (strlen(cfgLayer->attr.type.c_str()) > 0) {
      dLayerType = CConfigReaderLayerTypeUnknown;
      CDBError("Unknown layer type for layer %s", layerName.c_str());
      return 1;
    }
  }
  // Deprecated
  if (cfgLayer->attr.type.equals("file")) {
    dLayerType = CConfigReaderLayerTypeDataBase; // CConfigReaderLayerTypeFile;
  }

  if (!_srvParams->internalAutoResourceLocation.empty()) {
    headerFilename = _srvParams->internalAutoResourceLocation.c_str();
  }

  isConfigured = true;
  return 0;
}

void CDataSource::addStep(const char *fileName) {
  TimeStep *timeStep = new TimeStep();
  timeSteps.push_back(timeStep);
  currentAnimationStep = timeSteps.size() - 1;
  timeStep->fileName.copy(fileName);
}

void CDataSource::setHeaderFilename(CT::string headerFilename) { this->headerFilename = headerFilename; }

void CDataSource::setGeo(GeoParameters &geo) {
  nativeProj4 = geo.crs;
  dWidth = geo.width;
  dHeight = geo.height;
  geo.bbox.toArray(dfBBOX);
}

const char *CDataSource::getFileName() {
  if (currentAnimationStep < 0) return NULL;
  if (currentAnimationStep >= (int)timeSteps.size()) return NULL;
  return timeSteps[currentAnimationStep]->fileName.c_str();
}

void CDataSource::setTimeStep(int timeStep) {
  if (timeStep < 0) return;
  if (timeStep > (int)timeSteps.size()) return;
  currentAnimationStep = timeStep;
}

int CDataSource::getCurrentTimeStep() { return currentAnimationStep; }

size_t CDataSource::getDimensionIndex(const char *name) {
  int idx = findCDFDimIdx(timeSteps[currentAnimationStep]->dims, name);
  return idx == -1 ? 0 : timeSteps[currentAnimationStep]->dims[idx].index;
}

size_t CDataSource::getDimensionIndex(int i) { return timeSteps[currentAnimationStep]->dims[i].index; }

CT::string CDataSource::getDimensionValue(int i) { return timeSteps[currentAnimationStep]->dims[i].value; }

int CDataSource::getNumTimeSteps() { return (int)timeSteps.size(); }

const char *CDataSource::getLayerName() { return layerName.c_str(); }
const char *CDataSource::getLayerTitle() { return layerTitle.c_str(); }

CCDFDims *CDataSource::getCDFDims() {
  if (currentAnimationStep >= int(timeSteps.size())) {
    CDBError("Invalid step asked");
    return NULL;
  }
  return &timeSteps[currentAnimationStep]->dims;
}

void CDataSource::readStatusFlags(CDF::Variable *var, std::vector<StatusFlag> &statusFlagList) {
  statusFlagList.clear();
  if (var != NULL) {
    CDF::Attribute *attr_flag_meanings = var->getAttributeNE("flag_meanings");
    // We might have status flag, check if all mandatory attributes are set!
    if (attr_flag_meanings != NULL) {
      CDF::Attribute *attr_flag_values = var->getAttributeNE("flag_values");
      if (attr_flag_values == NULL) {
        attr_flag_values = var->getAttributeNE("flag_masks");
      }
      if (attr_flag_values != NULL) {
        CT::string flag_meanings;
        flag_meanings = attr_flag_meanings->toString();
        auto flagStrings = flag_meanings.split(" ");
        size_t nrOfFlagMeanings = flagStrings.size();
        if (nrOfFlagMeanings > 0) {
          size_t nrOfFlagValues = attr_flag_values->length;
          // Check we have an equal number of flagmeanings and flagvalues
          if (nrOfFlagMeanings == nrOfFlagValues) {
            double *dfFlagValues = new double[nrOfFlagMeanings + 1];
            attr_flag_values->getData(dfFlagValues, attr_flag_values->length);
            for (size_t j = 0; j < nrOfFlagMeanings; j++) {
              statusFlagList.push_back({.meaning = flagStrings[j], .value = dfFlagValues[j]});
            }
            delete[] dfFlagValues;
          } else {
            CDBError("ReadStatusFlags: nrOfFlagMeanings!=nrOfFlagValues, %lu!=%lu", nrOfFlagMeanings, nrOfFlagValues);
          }
        } else {
          CDBError("ReadStatusFlags: flag_meanings: nrOfFlagMeanings = 0");
        }
      } else {
        CDBError("ReadStatusFlags: flag_meanings found, but no flag_values attribute found");
      }
    }
  }
}

std::string CDataSource::getFlagMeaning(std::vector<StatusFlag> &statusFlagList, double value) {
  for (size_t j = 0; j < statusFlagList.size(); j++) {
    if ((statusFlagList)[j].value == value) {
      return (statusFlagList)[j].meaning.c_str();
    }
  }
  return "no_flag_meaning";
}

std::string CDataSource::getFlagMeaningHumanReadable(std::vector<StatusFlag> &statusFlagList, double value) {
  std::string flagMeaning = getFlagMeaning(statusFlagList, value);
  return CT::replace(flagMeaning, "_", " ");
}

CT::string CDataSource::getDimensionValueForNameAndStep(const char *dimName, int dimStep) { return getCDFDimensionValue(timeSteps[dimStep]->dims, dimName); }

/**
 * Returns a stringlist with all available legends for this datasource and chosen style.
 * @param dataSource pointer to the datasource
 * @param style pointer to the style to find the legends for
 * @return stringlist with the list of available legends.
 */

std::vector<std::string> CDataSource::getLegendListForDataSource(CServerConfig::XMLE_Style *style) {
  if (debugDataSource) {
    CDBDebug("getLegendListForDataSource");
  }
  if (this->cfgLayer->Legend.size() > 0) {
    return CServerParams::getLegendNames(this->cfgLayer->Legend);
  } else {
    if (style != NULL) {
      return CServerParams::getLegendNames(style->Legend);
    }
  }
  //  CDBError("No legendlist for layer %s",this->layerName.c_str());
  return {};
}

/**
 * Returns a stringlist with all available rendermethods for this datasource and chosen style.
 * @param dataSource pointer to the datasource
 * @param style pointer to the style to find the rendermethods for
 * @return stringlist with the list of available rendermethods.
 */
std::vector<std::string> CDataSource::getRenderMethodListForDataSource(CServerConfig::XMLE_Style *style) {
  // List all the desired rendermethods
  CT::string renderMethodList;

  // rendermethods defined in the layers must prepend rendermethods defined in the style
  if (this->cfgLayer->RenderMethod.size() > 0) {

    for (size_t j = 0; j < this->cfgLayer->RenderMethod.size(); j++) {
      if (renderMethodList.length() > 0) renderMethodList.concat(",");
      renderMethodList.concat(this->cfgLayer->RenderMethod[j]->value.c_str());
    }
  }

  if (style != NULL) {
    if (style->RenderMethod.size() > 0) {
      for (size_t j = 0; j < style->RenderMethod.size(); j++) {
        if (renderMethodList.length() > 0) renderMethodList.concat(",");
        renderMethodList.concat(style->RenderMethod[j]->value.c_str());
      }
    } else {
      return {"generic"};
    }
  }

  // If still no list of rendermethods is found, use the default list
  if (renderMethodList.length() == 0) {
    renderMethodList = "nearest";
  }

  return CT::split(renderMethodList, ",");
}

/**
 * Returns a vector with all possible CStyleConfiguration for the datasource
 * @param dataSource pointer to the datasource to find the stylelist for
 * @return vector with all possible CStyleConfigurations
 */
const std::vector<CStyleConfiguration> &CDataSource::getStyleListForDataSource() {
  if (styleConfigurationList.size() > 0) {
    return styleConfigurationList;
  }
  if (debugDataSource) {
    CDBDebug("getStyleListForDataSource %s", this->layerName.c_str());
  }

  CServerConfig::XMLE_Configuration *serverCFG = this->cfg;

  std::vector<std::string> renderMethods;
  std::vector<std::string> legendList;

  // Auto configure styles, if no legends or styles are defined
  if (this->cfgLayer->Styles.size() == 0 && this->cfgLayer->Legend.size() == 0) {
    renderMethods = getRenderMethodListForDataSource(NULL);
    if (renderMethods.size() > 0) {
      CAutoConfigure::autoConfigureStyles(this);
    }
  }

  std::vector<std::string> styleNames = getStyleNames(this->cfgLayer->Styles);

  size_t start = 0;
  if (styleNames.size() > 1) start = 1;
  // Loop over the styles.
  try {
    for (size_t i = start; i < styleNames.size(); i++) {

      // Lookup the style index in the servers configuration

      int dStyleIndex = this->srvParams->getServerStyleIndexByName(styleNames[i]);

      if (debugDataSource) {
        CDBDebug("dStyleIndex = %d", dStyleIndex);
      }
      // TODO CHECK, why did we add this line?:
      if (dStyleIndex != -1) {

        CServerConfig::XMLE_Style *style = NULL;
        if (dStyleIndex != -1) style = serverCFG->Style[dStyleIndex];

        renderMethods = getRenderMethodListForDataSource(style);
        legendList = getLegendListForDataSource(style);

        if (legendList.size() == 0) {
          CDBDebug("No legends defined for layer %s, adding legend auto", this->layerName.c_str());
          legendList.push_back("rainbow");
        }

        bool isOnlyGeneric = renderMethods.size() == 0 || (renderMethods.size() == 1 && renderMethods[0] == "generic");
        if (isOnlyGeneric) {
          CStyleConfiguration styleConfig;
          styleConfig.styleCompositionName = styleNames[i];
          styleConfig.styleName = styleNames[i];
          styleConfig.styleTitle = styleNames[i];
          styleConfig.renderMethod = RM_GENERIC;
          styleConfig.styleIndex = dStyleIndex;
          styleConfig.legendIndex = this->srvParams->getServerLegendIndexByName(legendList[0]);
          if (styleConfig.legendIndex == -1) {
            CDBError("Legend %s not found", legendList[0].c_str());
          }
          int status = styleConfig.makeStyleConfig(this);
          if (status == -1) {
            styleConfig.hasError = true;
          }
          styleConfigurationList.push_back(styleConfig);
        } else {
          // Deprecated methods to have multiple legends and rendermethods in the same style

          CT::string styleName;
          for (size_t l = 0; l < legendList.size(); l++) {
            for (size_t r = 0; r < renderMethods.size(); r++) {
              if (renderMethods[r].length() > 0) {
                if (legendList.size() > 1) {
                  styleName.print("%s_%s/%s", styleNames[i].c_str(), legendList[l].c_str(), renderMethods[r].c_str());
                  CDBWarning("Deprecated to have multiple legends in one style");
                } else {
                  styleName.print("%s/%s", styleNames[i].c_str(), renderMethods[r].c_str());
                  // if (renderMethods.size() > 1) {
                  //   // CDBWarning("Deprecated to have multiple rendermethods ([%d]) in one style. There are %d styles configured.", renderMethods.size(), styleNames.size());
                  // }
                }

                styleConfigurationList.push_back(CStyleConfiguration());
                CStyleConfiguration &styleConfig = styleConfigurationList.back();
                styleConfig.styleCompositionName = styleName.c_str();
                styleConfig.styleName = styleNames[i];
                styleConfig.styleTitle = styleName.c_str();
                styleConfig.renderMethod = getRenderMethodFromString(renderMethods[r].c_str());
                styleConfig.styleIndex = dStyleIndex;
                styleConfig.legendIndex = this->srvParams->getServerLegendIndexByName(legendList[l]);
                if (styleConfig.legendIndex == -1) {
                  CDBError("Legend %s not found", legendList[l].c_str());
                }

                if (style != nullptr && style->NameMapping.size() > 0) {
                  if (configWarningSet == false) {
                    CDBLint("In dataset \"%s\" and style \"%s\": Deprecated to have NameMapping configs in the style. Use title and abstracts instead.",
                            CT::basename(this->srvParams->datasetLocation).c_str(), styleName.c_str());
                    configWarningSet = true;
                  }
                  for (size_t j = 0; j < style->NameMapping.size(); j++) {
                    if (renderMethods[r] == style->NameMapping[j]->attr.name.c_str()) {
                      styleConfig.styleTitle = style->NameMapping[j]->attr.title;
                      styleConfig.styleAbstract = style->NameMapping[j]->attr.abstract;
                      break;
                    }
                  }
                }
                int status = styleConfig.makeStyleConfig(this);
                if (status == -1) {
                  styleConfig.hasError = true;
                }
              }
            }
          }
        }
      }
    }
    // We have been through the loop, but the styleConfig has not been created. This is an error.
  } catch (int e) {
  }

  if (styleConfigurationList.size() == 0) {
    CStyleConfiguration styleConfig;
    styleConfig.styleTitle = "default";
    styleConfig.styleAbstract = "default";
    styleConfig.renderMethod = RM_NEAREST;
    styleConfig.styleCompositionName = "default";
    styleConfigurationList.push_back(styleConfig);
  }
  if (debugDataSource) {
    CDBDebug("/getStyleListForDataSource");
  }

  return styleConfigurationList;
}

/**
 * Returns a stringlist with all possible styles available for this style config object.
 * @param Style a pointer to XMLE_Style vector configured in a layer
 * @return Pointer to a new stringlist with all possible style names, must be deleted with delete. Is NULL on failure.
 */
std::vector<std::string> CDataSource::getStyleNames(std::vector<CServerConfig::XMLE_Styles *> Styles) {
  std::vector<std::string> stringList = {"default"};
  for (size_t j = 0; j < Styles.size(); j++) {
    if (Styles[j]->value.empty()) continue;
    std::vector<CT::string> l1 = Styles[j]->value.split(",");
    for (auto styleValue: l1) {
      if (styleValue.length() > 0) {
        stringList.push_back(styleValue);
      }
    }
  }
  return stringList;
}

CStyleConfiguration *CDataSource::getStyle() {
  if (currentStyleSet == true) {
    return &currentStyle;
  }
  const auto &styleList = getStyleListForDataSource();
  if (styleList.size() == 0) {
    CDBError("There are no styles available");
    return nullptr;
  }
  CT::string styleName = "default";
  CT::string styles(srvParams->Styles.c_str());

  // TODO CHECK CDBDebug("Server Styles=%s",srvParam->Styles.c_str());
  std::vector<CT::string> layerstyles = styles.split(",");
  int layerIndex = datasourceIndex;
  if (layerstyles.size() != 0) {
    // Make sure default layer index is within the right bounds.
    if (layerIndex < 0) layerIndex = 0;
    if (layerIndex > ((int)layerstyles.size()) - 1) layerIndex = layerstyles.size() - 1;
    styleName = layerstyles[layerIndex].c_str();
    if (styleName.length() == 0) {
      styleName = "default";
    }
  }

  // Copy default style
  currentStyle = styleList.at(0);

  auto it = std::find_if(styleList.begin(), styleList.end(), [&styleName](const CStyleConfiguration &a) { return styleName.equals(a.styleName); });
  if (it != styleList.end()) {
    currentStyle = (*it);
  } else {
    // If not found, check for the style without rendermethod instead using startsWith.
    it = std::find_if(styleList.begin(), styleList.end(), [&styleName](const CStyleConfiguration &a) { return CT::startsWith(a.styleCompositionName, styleName); });
    if (it != styleList.end()) {
      currentStyle = (*it);
    } else {
      // Try without rendermethod
      CT::string styleNameWithoutRenderMethod = styleName.substring(0, styleName.indexOf("/"));
      it = std::find_if(styleList.begin(), styleList.end(), [&styleNameWithoutRenderMethod](const CStyleConfiguration &a) { return styleNameWithoutRenderMethod.equals(a.styleName); });
      if (it != styleList.end()) {
        currentStyle = (*it);
        CDBDebug("Selected style %s", currentStyle.styleName.c_str());
      }
    }
  }

  if (currentStyle.styleIndex == -1) {
    int status = currentStyle.makeStyleConfig(this);
    if (status == -1) {
      currentStyle.hasError = true;
    }
  }
  if (currentStyle.legendIndex == -1) {
    std::vector<std::string> legendList = getLegendListForDataSource(NULL);
    if (legendList.size() > 0) {
      currentStyle.legendIndex = this->srvParams->getServerLegendIndexByName(legendList[0]);
    }
  }
  currentStyleSet = true;
  return &currentStyle;
}

int CDataSource::setStyle(const char *styleName) {
  const auto &styleList = getStyleListForDataSource();

  if (styleList.size() == 0) {
    CDBError("There are no styles available");
    return 1;
  }

  auto it = std::find_if(styleList.begin(), styleList.end(), [&styleName](const CStyleConfiguration &a) { return CT::startsWith(a.styleCompositionName, styleName); });
  if (it != styleList.end()) {
    currentStyle = (*it);
  } else {

    currentStyle = styleList.at(0);
    CDBWarning("Unable to find style %s. Available styles:", styleName);
    for (size_t j = 0; j < styleList.size(); j++) {
      CDBWarning("  -%s", styleList.at(j).styleCompositionName.c_str());
    }
  }

  if (currentStyle.styleIndex == -1) {
    int status = currentStyle.makeStyleConfig(this);
    if (status == -1) {
      currentStyle.hasError = true;
    }
  }
  if (currentStyle.legendIndex == -1) {
    std::vector<std::string> legendList = getLegendListForDataSource(NULL);
    if (legendList.size() > 0) {
      currentStyle.legendIndex = this->srvParams->getServerLegendIndexByName(legendList[0]);
    }
  }
  if (currentStyle.hasError) return 1;
  // Indicate that this style is set, so the getter does not re-figure out the style.
  currentStyleSet = true;
  return 0;
};

CDataSource *CDataSource::clone() {

  CDataSource *d = new CDataSource();
  d->dataSourceOwnsDataObject = false; // cdfObject stays with source datasource.
  d->currentStyle = currentStyle;
  d->datasourceIndex = datasourceIndex;
  d->currentAnimationStep = currentAnimationStep;
  d->styleConfigurationList = styleConfigurationList;

  /* Copy timesteps */
  for (size_t j = 0; j < timeSteps.size(); j++) {
    // CDBDebug("addStep for %s",fileName);
    TimeStep *timeStep = new TimeStep();
    d->timeSteps.push_back(timeStep);
    timeStep->fileName.copy(timeSteps[j]->fileName.c_str());
    timeStep->dims = timeSteps[j]->dims;
  }

  /* Copy dataObjects */
  for (size_t j = 0; j < dataObjects.size(); j++) {
    d->dataObjects.push_back(dataObjects[j]->clone());
  }

  d->stretchMinMax = stretchMinMax;

  /* Copy requireddims */
  d->requiredDims = requiredDims;

  for (size_t j = 0; j < 4; j++) {
    d->dfBBOX[j] = dfBBOX[j];
  }
  d->nativeViewPortBBOX = nativeViewPortBBOX;

  d->dfCellSizeX = dfCellSizeX;
  d->dfCellSizeY = dfCellSizeY;
  d->dWidth = dWidth;
  d->dHeight = dHeight;
  d->nativeEPSG = nativeEPSG;
  d->nativeProj4 = nativeProj4;
  d->isConfigured = isConfigured;
  d->formatConverterActive = formatConverterActive;
  d->dimXIndex = dimXIndex;
  d->dimYIndex = dimYIndex;
  d->stride2DMap = stride2DMap;
  d->useLonTransformation = useLonTransformation;
  d->origBBOXLeft = origBBOXLeft;
  d->origBBOXRight = origBBOXRight;
  d->dOrigWidth = dOrigWidth;
  d->lonTransformDone = lonTransformDone;
  d->swapXYDimensions = swapXYDimensions;
  d->varX = varX;
  d->varY = varY;
  d->dNetCDFNumDims = dNetCDFNumDims;
  d->dLayerType = dLayerType;
  d->layerName = layerName;
  d->queryBBOX = queryBBOX;
  d->queryLevel = queryLevel;
  d->srvParams = srvParams;
  d->cfgLayer = cfgLayer;
  d->cfg = cfg;
  d->featureSet = featureSet;

  return d;
}

double CDataSource::getScaling() {
  auto styleConfiguration = this->getStyle();
  if (styleConfiguration != nullptr) {
    for (auto renderSetting: styleConfiguration->renderSettings) {
      if (!renderSetting->attr.scalewidth.empty()) {
        double scaleWidth = renderSetting->attr.scalewidth.toDouble();
        double imageWidth = (double)this->srvParams->geoParams.width;
        return imageWidth / scaleWidth;
      }
    }
  }
  return 1;
}

double CDataSource::getContourScaling() {
  auto styleConfiguration = this->getStyle();
  if (styleConfiguration != nullptr) {
    for (auto renderSetting: styleConfiguration->renderSettings) {
      if (!renderSetting->attr.scalecontours.empty()) {
        double scalecontours = renderSetting->attr.scalecontours.toDouble();
        return scalecontours;
      }
    }
  }
  return 1;
}

DataObject *CDataSource::getDataObjectByName(std::string name) { return getDataObjectByName(name.c_str()); }
DataObject *CDataSource::getDataObjectByName(const char *name) {
  for (auto it = dataObjects.begin(); it != dataObjects.end(); ++it) {
    DataObject *dataObject = *it;

    if (dataObject->dataObjectName.equals(name)) {
      return dataObject;
    }

    if (dataObject->variableName.equals(name)) {
      return dataObject;
    }

    if (dataObject->cdfVariable->name.equals(name)) {
      return dataObject;
    }
  }
  return nullptr;
}

DataObject *CDataSource::getFirstAvailableDataObject() {
  for (size_t o = 0; o < this->getNumDataObjects(); o++) {
    if (this->getDataObject(o)->filterFromOutput) {
      continue;
    }
    return this->getDataObject(o);
  }
  CDBWarning("No dataobjects available");
  return nullptr;
}

DataObject *CDataSource::getDataObject(int j) {

  if (int(dataObjects.size()) <= j) {
    CDBError("No Data object witn nr %d (total %d) for animation step %lu (total steps %lu)", j, currentAnimationStep, dataObjects.size(), timeSteps.size());
    throw(CEXCEPTION_NULLPOINTER);
  }

  DataObject *d = dataObjects[j];
  // CDBDebug("getDataObject %d %d",currentAnimationStep,j);
  return d;
}

int CDataSource::attachCDFObject(CDFObject *cdfObject, bool dataSourceOwnsDataObject) {
  if (cdfObject == NULL) {
    CDBError("cdfObject==NULL");
    return 1;
  }
  if (isConfigured == false) {
    CDBError("Datasource %s is not configured", cfgLayer->Name[0]->value.c_str());
    return 1;
  }
  if (getNumDataObjects() <= 0) {
    CDBError("No variables found for datasource %s", cfgLayer->Name[0]->value.c_str());
    return 1;
  }
  for (size_t varNr = 0; varNr < getNumDataObjects(); varNr++) {
    if (getDataObject(varNr)->cdfVariable != NULL && getDataObject(varNr)->cdfVariable->hasCustomReader()) {
      continue;
    }

    getDataObject(varNr)->cdfObject = cdfObject;
    getDataObject(varNr)->cdfVariable = cdfObject->getVariableNE(getDataObject(varNr)->variableName.c_str());
    if (getDataObject(varNr)->cdfVariable == NULL) {
      CDBError("attachCDFObject: variable nr %lu \"%s\" does not exist", varNr, getDataObject(varNr)->variableName.c_str());
      return 1;
    }
  }
  // Shorthand to variable configuration in the layer.
  for (auto *cfgVar: cfgLayer->Variable) {
    CDF::Variable *var = cdfObject->getVar(cfgVar->value);
    if (var != nullptr) {

      // Set long_name
      if (!cfgVar->attr.long_name.empty()) {
        var->setAttributeText("long_name", cfgVar->attr.long_name);
      }

      // Set units
      if (!cfgVar->attr.units.empty()) {
        var->setAttributeText("units", cfgVar->attr.units);
      }

      // Set standard_name
      if (!cfgVar->attr.standard_name.empty()) {
        var->setAttributeText("standard_name", cfgVar->attr.standard_name);
      }
    }
  }
  this->dataSourceOwnsDataObject = dataSourceOwnsDataObject;
  return 0;
}
void CDataSource::detachCDFObject() {
  for (size_t j = 0; j < getNumDataObjects(); j++) {
    getDataObject(j)->cdfVariable = NULL;
    getDataObject(j)->cdfObject = NULL;
  }
}

int CDataSource::readVariableDataForCDFDims(CDF::Variable *variableToRead, CDFType dataTypeToReturnData) {
  if (variableToRead == nullptr) {
    CDBError("Variable is not defined");
    return 1;
  }
  size_t numDimensionsForVariableToRead = variableToRead->dimensionlinks.size();
  std::vector<size_t> start(numDimensionsForVariableToRead);
  std::vector<size_t> count(numDimensionsForVariableToRead);
  std::vector<ptrdiff_t> stride(numDimensionsForVariableToRead);
  auto *cdfDims = this->getCDFDims();
  for (size_t dimNr = 0; dimNr < numDimensionsForVariableToRead; dimNr += 1) {
    auto *dimensionLink = variableToRead->dimensionlinks[dimNr];
    size_t startCountIndex = dimNr;
    start[startCountIndex] = 0;
    stride[startCountIndex] = 1;
    count[startCountIndex] = dimensionLink->getSize();
    int cdfDimIndex = findCDFDimIdx(*cdfDims, dimensionLink->name.c_str());
    if (cdfDimIndex >= 0) {

      start[startCountIndex] = cdfDims->at(cdfDimIndex).index;
      count[startCountIndex] = 1;
    }
  }
  return variableToRead->readData(dataTypeToReturnData, start.data(), count.data(), stride.data(), true);
}

std::string CDataSource::getDataSetName() { return std::string(this->srvParams->datasetLocation.c_str()); }

GeoParameters CDataSource::makeGeoParams() {
  GeoParameters geoParams;
  geoParams.width = this->dWidth;
  geoParams.height = this->dHeight;
  geoParams.bbox = this->dfBBOX;
  geoParams.cellsizeX = this->dfCellSizeX;
  geoParams.cellsizeY = this->dfCellSizeY;
  geoParams.crs = this->nativeProj4;
  return geoParams;
}

CDataSource::CDataSource(CServerParams *srvParams, CServerConfig::XMLE_Layer *cfgLayer, int layerIndex) { setCFGLayer(srvParams, cfgLayer, layerIndex); }