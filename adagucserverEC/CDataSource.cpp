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
const char *CDataSource::className = "CDataSource";

// #define CDATASOURCE_DEBUG

CDataSource::DataObject::DataObject() {
  hasStatusFlag = false;
  appliedScaleOffset = false;
  hasScaleOffset = false;
  cdfVariable = NULL;
  cdfObject = NULL;
  dfadd_offset = 0;
  dfscale_factor = 1;
  std::vector<f8point> points;
}

CDataSource::DataObject *CDataSource::DataObject::clone() {
  CDataSource::DataObject *nd = new CDataSource::DataObject();
  nd->hasStatusFlag = hasStatusFlag;
  nd->hasNodataValue = hasNodataValue;
  nd->appliedScaleOffset = appliedScaleOffset;
  nd->hasScaleOffset = hasScaleOffset;
  nd->dfNodataValue = dfNodataValue;
  nd->dfscale_factor = dfscale_factor;
  nd->dfadd_offset = dfadd_offset;
  nd->cdfObject = cdfObject;
  nd->overruledUnits = overruledUnits;
  nd->variableName = variableName;
  nd->cdfVariable = nullptr;
  return nd;
}

CT::string CDataSource::DataObject::getUnits() {
  if (overruledUnits.empty() && cdfVariable != NULL) {
    try {
      return cdfVariable->getAttribute("units")->getDataAsString();
    } catch (int e) {
    }
  }
  return overruledUnits;
}

CT::string CDataSource::DataObject::getStandardName() {
  CT::string standard_name = variableName;
  CDF::Attribute *standardNameAttr = cdfVariable->getAttributeNE("standard_name");
  if (standardNameAttr != nullptr) {
    standard_name = standardNameAttr->toString();
  }
  return standard_name;
}

void CDataSource::DataObject::setUnits(CT::string units) { overruledUnits = units; }

double CDataSource::Statistics::getMinimum() { return min; }
double CDataSource::Statistics::getMaximum() { return max; }

double CDataSource::Statistics::getStdDev() { return stddev; }

double CDataSource::Statistics::getAverage() { return avg; }

void CDataSource::Statistics::setMinimum(double min) { this->min = min; }
void CDataSource::Statistics::setMaximum(double max) { this->max = max; }

MinMax getMinMax(double *data, bool hasFillValue, double fillValue, size_t numElements) {
  MinMax minMax;
  bool firstSet = false;
  for (size_t j = 0; j < numElements; j++) {
    double v = data[j];
    if (v == v) {
      if ((v != fillValue) || (!hasFillValue)) {
        if (firstSet == false) {
          firstSet = true;
          minMax.min = v;
          minMax.max = v;
          minMax.isSet = true;
        }
        if (v < minMax.min) minMax.min = v;
        if (v > minMax.max) minMax.max = v;
      }
    }
  }
  if (minMax.isSet == false) {
    throw __LINE__;
  }
  return minMax;
}

MinMax getMinMax(float *data, bool hasFillValue, double fillValue, size_t numElements) {
  MinMax minMax;
  bool firstSet = false;
  for (size_t j = 0; j < numElements; j++) {
    float v = data[j];
    if (v == v) {
      if ((v != fillValue) || (!hasFillValue)) {
        if (firstSet == false) {
          firstSet = true;
          minMax.min = v;
          minMax.max = v;
          minMax.isSet = true;
        }
        if (v < minMax.min) minMax.min = v;
        if (v > minMax.max) minMax.max = v;
      }
    }
  }
  if (minMax.isSet == false) {
    throw __LINE__ + 100;
  }
  return minMax;
}

MinMax getMinMax(CDF::Variable *var) {
  MinMax minMax;
  if (var != NULL) {
    if (var->getType() == CDF_FLOAT) {

      float *data = (float *)var->data;

      float scaleFactor = 1, addOffset = 0, fillValue = 0;
      bool hasFillValue = false;

      try {
        var->getAttribute("scale_factor")->getData(&scaleFactor, 1);
      } catch (int e) {
      }
      try {
        var->getAttribute("add_offset")->getData(&addOffset, 1);
      } catch (int e) {
      }
      try {
        var->getAttribute("_FillValue")->getData(&fillValue, 1);
        hasFillValue = true;
      } catch (int e) {
      }

      size_t lsize = var->getSize();

      // Apply scale and offset
      if (scaleFactor != 1 || addOffset != 0) {
        for (size_t j = 0; j < lsize; j++) {
          data[j] = data[j] * scaleFactor + addOffset;
        }
        fillValue = fillValue * scaleFactor + addOffset;
      }

      minMax = getMinMax(data, hasFillValue, fillValue, lsize);
    } else if (var->getType() == CDF_DOUBLE) {

      double *data = (double *)var->data;

      double scaleFactor = 1, addOffset = 0, fillValue = 0;
      bool hasFillValue = false;

      try {
        var->getAttribute("scale_factor")->getData(&scaleFactor, 1);
      } catch (int e) {
      }
      try {
        var->getAttribute("add_offset")->getData(&addOffset, 1);
      } catch (int e) {
      }
      try {
        var->getAttribute("_FillValue")->getData(&fillValue, 1);
        hasFillValue = true;
      } catch (int e) {
      }

      size_t lsize = var->getSize();

      // Apply scale and offset
      if (scaleFactor != 1 || addOffset != 0) {
        for (size_t j = 0; j < lsize; j++) {
          data[j] = data[j] * scaleFactor + addOffset;
        }
        fillValue = fillValue * scaleFactor + addOffset;
      }

      minMax = getMinMax(data, hasFillValue, fillValue, lsize);
    }

  } else {
    throw __LINE__;
  }
  return minMax;
}

int CDataSource::Statistics::calculate(CDataSource *dataSource) {
  // Get Min and Max
  // CDBDebug("calculate stat ");
  CDataSource::DataObject *dataObject = dataSource->getFirstAvailableDataObject();
  if (dataObject->cdfVariable->data != NULL) {
    size_t size = dataObject->cdfVariable->getSize(); // dataSource->dWidth*dataSource->dHeight;

    if (dataObject->cdfVariable->getType() == CDF_CHAR) calcMinMax<char>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_BYTE) calcMinMax<char>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_UBYTE) calcMinMax<unsigned char>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_SHORT) calcMinMax<short>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_USHORT) calcMinMax<unsigned short>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_INT) calcMinMax<int>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_UINT) calcMinMax<unsigned int>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_FLOAT) calcMinMax<float>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_DOUBLE) calcMinMax<double>(size, dataSource->getDataObjectsVector());
  }
  return 0;
}

template <class T> void CDataSource::Statistics::calcMinMax(size_t size, std::vector<DataObject *> *dataObject) {
#ifdef MEASURETIME
  StopWatch_Stop("Start min/max calculation");
#endif
  if (dataObject->size() == 1) {
    T *data = (T *)(*dataObject)[0]->cdfVariable->data;
    CDFType type = (*dataObject)[0]->cdfVariable->getType();
    double dfNodataValue = (*dataObject)[0]->dfNodataValue;
    bool hasNodataValue = (*dataObject)[0]->hasNodataValue;
    calculate(size, data, type, dfNodataValue, hasNodataValue);
  }

  // Wind vector min max calculation
  if (dataObject->size() == 2) {
    T *dataU = (T *)(*dataObject)[0]->cdfVariable->data;
    T *dataV = (T *)(*dataObject)[1]->cdfVariable->data;
    T _min = (T)0.0f, _max = (T)0.0f;
    int firstDone = 0;
    T s = 0;
    for (size_t p = 0; p < size; p++) {

      T u = dataU[p];
      T v = dataV[p];

      if (((((T)v) != (T)(*dataObject)[0]->dfNodataValue || (!(*dataObject)[0]->hasNodataValue)) && v == v) &&
          ((((T)u) != (T)(*dataObject)[0]->dfNodataValue || (!(*dataObject)[0]->hasNodataValue)) && u == u)) {
        s = (T)hypot(u, v);
        if (firstDone == 0) {
          _min = s;
          _max = s;
          firstDone = 1;
        } else {

          if (s < _min) _min = s;
          if (s > _max) _max = s;
        }
      }
    }
    min = (double)_min;
    max = (double)_max;
  }
#ifdef MEASURETIME
  StopWatch_Stop("Finished min/max calculation");
#endif
}

CDataSource::CDataSource() {
  stretchMinMax = false;
  stretchMinMaxDone = false;
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

  _styles = NULL;
  _currentStyle = NULL;

  queryBBOX = false;
  queryLevel = 0; // -1 Means query all. 0 means query all non tiled versions. Positive number means a tiled version.
  featureSet = NULL;
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
  for (size_t j = 0; j < requiredDims.size(); j++) delete requiredDims[j];
  if (statistics != NULL) {
    delete statistics;
  };
  statistics = NULL;

  if (_styles != NULL) {
    delete _styles;
    _styles = NULL;
  }

  if (featureSet.length() != 0) {
    CConvertGeoJSON::clearFeatureStore(featureSet);
    featureSet = NULL;
  }
}

int CDataSource::setCFGLayer(CServerParams *_srvParams, CServerConfig::XMLE_Configuration *_cfg, CServerConfig::XMLE_Layer *_cfgLayer, const char *_layerName, int layerIndex) {
  srvParams = _srvParams;
  cfg = _cfg;
  cfgLayer = _cfgLayer;
  datasourceIndex = layerIndex;

  // Make DataObjects for each Variable defined in the Layer.
  for (size_t j = 0; j < cfgLayer->Variable.size(); j++) {
    DataObject *newDataObject = new DataObject();
    newDataObject->variableName.copy(cfgLayer->Variable[j]->value.c_str());
    this->dataObjects.push_back(newDataObject);
  }

  // Set the layername
  layerName = makeUniqueLayerName(cfgLayer, _layerName);

  layerTitle = cfgLayer->Title.size() > 0 && !cfgLayer->Title[0]->value.empty() ? cfgLayer->Title[0]->value.c_str() : layerName.c_str();

#ifdef CDATASOURCE_DEBUG
  CDBDebug("LayerName=\"%s\"", layerName.c_str());
#endif
  // Defaults to database
  dLayerType = CConfigReaderLayerTypeDataBase;
  if (cfgLayer->attr.type.equals("database")) {
    dLayerType = CConfigReaderLayerTypeDataBase;
  } else if (cfgLayer->attr.type.equals("styled")) {
    dLayerType = CConfigReaderLayerTypeStyled;
  } else if (cfgLayer->attr.type.equals("cascaded")) {
    dLayerType = CConfigReaderLayerTypeCascaded;
  } else if (cfgLayer->attr.type.equals("image")) {
    dLayerType = CConfigReaderLayerTypeCascaded;
  } else if (cfgLayer->attr.type.equals("grid")) {
    dLayerType = CConfigReaderLayerTypeCascaded;
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

size_t CDataSource::getDimensionIndex(const char *name) { return timeSteps[currentAnimationStep]->dims.getDimensionIndex(name); }

size_t CDataSource::getDimensionIndex(int i) { return timeSteps[currentAnimationStep]->dims.getDimensionIndex(i); }

CT::string CDataSource::getDimensionValue(int i) { return timeSteps[currentAnimationStep]->dims.getDimensionValue(i); }

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

void CDataSource::readStatusFlags(CDF::Variable *var, std::vector<CDataSource::StatusFlag> *statusFlagList) {
  statusFlagList->clear();
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
        attr_flag_meanings->getDataAsString(&flag_meanings);
        auto flagStrings = flag_meanings.splitToStack(" ");
        size_t nrOfFlagMeanings = flagStrings.size();
        if (nrOfFlagMeanings > 0) {
          size_t nrOfFlagValues = attr_flag_values->length;
          // Check we have an equal number of flagmeanings and flagvalues
          if (nrOfFlagMeanings == nrOfFlagValues) {
            double dfFlagValues[nrOfFlagMeanings + 1];
            attr_flag_values->getData(dfFlagValues, attr_flag_values->length);
            for (size_t j = 0; j < nrOfFlagMeanings; j++) {
              statusFlagList->push_back({.meaning = flagStrings[j], .value = dfFlagValues[j]});
            }
          } else {
            CDBError("ReadStatusFlags: nrOfFlagMeanings!=nrOfFlagValues, %d!=%d", nrOfFlagMeanings, nrOfFlagValues);
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

const char *CDataSource::getFlagMeaning(std::vector<CDataSource::StatusFlag> *statusFlagList, double value) {
  for (size_t j = 0; j < statusFlagList->size(); j++) {
    if ((*statusFlagList)[j].value == value) {
      return (*statusFlagList)[j].meaning.c_str();
    }
  }
  return "no_flag_meaning";
}

void CDataSource::getFlagMeaningHumanReadable(CT::string *flagMeaning, std::vector<CDataSource::StatusFlag> *statusFlagList, double value) {
  flagMeaning->copy(getFlagMeaning(statusFlagList, value));
  flagMeaning->replaceSelf("_", " ");
}

CT::string CDataSource::getDimensionValueForNameAndStep(const char *dimName, int dimStep) { return timeSteps[dimStep]->dims.getDimensionValue(dimName); }

/**
 * Returns a stringlist with all available legends for this datasource and chosen style.
 * @param dataSource pointer to the datasource
 * @param style pointer to the style to find the legends for
 * @return stringlist with the list of available legends.
 */

std::vector<CT::string> CDataSource::getLegendListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style *style) {
#ifdef CDATASOURCE_DEBUG
  CDBDebug("getLegendListForDataSource");
#endif
  if (dataSource->cfgLayer->Legend.size() > 0) {
    return CServerParams::getLegendNames(dataSource->cfgLayer->Legend);
  } else {
    if (style != NULL) {
      return CServerParams::getLegendNames(style->Legend);
    }
  }
  //  CDBError("No legendlist for layer %s",dataSource->layerName.c_str());
  return {};
}

/**
 * Returns a stringlist with all available rendermethods for this datasource and chosen style.
 * @param dataSource pointer to the datasource
 * @param style pointer to the style to find the rendermethods for
 * @return stringlist with the list of available rendermethods.
 */
std::vector<CT::string> CDataSource::getRenderMethodListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style *style) {
  // List all the desired rendermethods
  CT::string renderMethodList;

  // rendermethods defined in the layers must prepend rendermethods defined in the style
  if (dataSource->cfgLayer->RenderMethod.size() > 0) {

    for (size_t j = 0; j < dataSource->cfgLayer->RenderMethod.size(); j++) {
      if (renderMethodList.length() > 0) renderMethodList.concat(",");
      renderMethodList.concat(dataSource->cfgLayer->RenderMethod[j]->value.c_str());
    }
  }

  if (style != NULL) {
    if (style->RenderMethod.size() > 0) {
      for (size_t j = 0; j < style->RenderMethod.size(); j++) {
        if (renderMethodList.length() > 0) renderMethodList.concat(",");
        renderMethodList.concat(style->RenderMethod[j]->value.c_str());
      }
    }
  }

  // If still no list of rendermethods is found, use the default list
  if (renderMethodList.length() == 0) {
    renderMethodList.copy("nearest");
  }

  return renderMethodList.splitToStack(",");
}

/**
 * Returns a vector with all possible CStyleConfiguration for the datasource
 * @param dataSource pointer to the datasource to find the stylelist for
 * @return vector with all possible CStyleConfigurations
 */
CT::PointerList<CStyleConfiguration *> *CDataSource::getStyleListForDataSource(CDataSource *dataSource) {

#ifdef CDATASOURCE_DEBUG
  CDBDebug("getStyleListForDataSource %s", dataSource->layerName.c_str());
#endif

  CT::PointerList<CStyleConfiguration *> *styleConfigurationList = new CT::PointerList<CStyleConfiguration *>();

  CServerConfig::XMLE_Configuration *serverCFG = dataSource->cfg;

  std::vector<CT::string> renderMethods;
  std::vector<CT::string> legendList;

  // Auto configure styles, if no legends or styles are defined
  if (dataSource->cfgLayer->Styles.size() == 0 && dataSource->cfgLayer->Legend.size() == 0) {
    renderMethods = getRenderMethodListForDataSource(dataSource, NULL);
    if (renderMethods.size() > 0) {
      CAutoConfigure::autoConfigureStyles(dataSource);
    }
  }

  std::vector<CT::string> styleNames = getStyleNames(dataSource->cfgLayer->Styles);

  size_t start = 0;
  if (styleNames.size() > 1) start = 1;
  // Loop over the styles.
  try {
    // CDBDebug("There are %d styles to check",styleNames->size());
    for (size_t i = start; i < styleNames.size(); i++) {

      // Lookup the style index in the servers configuration

      int dStyleIndex = dataSource->srvParams->getServerStyleIndexByName(styleNames[i]);

#ifdef CDATASOURCE_DEBUG
      CDBDebug("dStyleIndex = %d", dStyleIndex);
#endif
      // TODO CHECK, why did we add this line?:
      if (dStyleIndex != -1) {

        CServerConfig::XMLE_Style *style = NULL;
        if (dStyleIndex != -1) style = serverCFG->Style[dStyleIndex];

        renderMethods = getRenderMethodListForDataSource(dataSource, style);
        legendList = getLegendListForDataSource(dataSource, style);

        if (legendList.size() == 0) {
          CDBDebug("No legends defined for layer %s, adding legend auto", dataSource->layerName.c_str());
          legendList.push_back("rainbow");
        }

        CT::string styleName;
        for (size_t l = 0; l < legendList.size(); l++) {
          for (size_t r = 0; r < renderMethods.size(); r++) {
            if (renderMethods[r].length() > 0) {
              int dLegendIndex = dataSource->srvParams->getServerLegendIndexByName(legendList[l]);
              if (legendList.size() > 1) {
                styleName.print("%s_%s/%s", styleNames[i].c_str(), legendList[l].c_str(), renderMethods[r].c_str());
              } else {
                styleName.print("%s/%s", styleNames[i].c_str(), renderMethods[r].c_str());
              }

#ifdef CDATASOURCE_DEBUG
              CDBDebug("Matching '%s' == '%s'", styleName->c_str(), styleToSearchString.c_str());
#endif
              CStyleConfiguration *styleConfig = new CStyleConfiguration();
              styleConfigurationList->push_back(styleConfig);
              styleConfig->styleCompositionName = styleName.c_str();
              styleConfig->styleName = styleNames[i];
              styleConfig->styleTitle = styleName.c_str();
              //  We found the correspondign legend/style and rendermethod corresponding with the requested stylename!
              //  Now fill in the CStyleConfiguration Object.

              styleConfig->renderMethod = getRenderMethodFromString(renderMethods[r].c_str());
              styleConfig->styleIndex = dStyleIndex;

              styleConfig->legendIndex = dLegendIndex;

              if (dLegendIndex == -1) {
                CDBError("Legend %s not found", legendList[l].c_str());
              }

              if (style != NULL) {
                for (size_t j = 0; j < style->NameMapping.size(); j++) {
                  if (renderMethods[r].equals(style->NameMapping[j]->attr.name.c_str())) {
                    styleConfig->styleTitle.copy(style->NameMapping[j]->attr.title.c_str());
                    styleConfig->styleAbstract.copy(style->NameMapping[j]->attr.abstract.c_str());
                    break;
                  }
                }
              }
#ifdef CDATASOURCE_DEBUG
              CDBDebug("Pushing %s with legendIndex %d and styleIndex %d", styleName.c_str(), dLegendIndex, dStyleIndex);
#endif
              int status = styleConfig->makeStyleConfig(dataSource);
              if (status == -1) {
                styleConfig->hasError = true;
              }
            }
          }
        }
      }
    }
    // We have been through the loop, but the styleConfig has not been created. This is an error.
  } catch (int e) {
  }

  if (styleConfigurationList->size() == 0) {
    CStyleConfiguration *styleConfig = new CStyleConfiguration();
    styleConfig->styleTitle.copy("default");
    styleConfig->styleAbstract.copy("default");
    styleConfig->renderMethod = RM_NEAREST;
    styleConfig->styleCompositionName = "default";
    styleConfigurationList->push_back(styleConfig);
  }
#ifdef CDATASOURCE_DEBUG
  CDBDebug("/getStyleListForDataSource");
#endif

  return styleConfigurationList;
}

/**
 * Returns a stringlist with all possible styles available for this style config object.
 * @param Style a pointer to XMLE_Style vector configured in a layer
 * @return Pointer to a new stringlist with all possible style names, must be deleted with delete. Is NULL on failure.
 */
std::vector<CT::string> CDataSource::getStyleNames(std::vector<CServerConfig::XMLE_Styles *> Styles) {
  std::vector<CT::string> stringList = {"default"};
  for (size_t j = 0; j < Styles.size(); j++) {
    if (Styles[j]->value.empty()) continue;
    CT::StackList<CT::string> l1 = Styles[j]->value.splitToStack(",");
    for (auto styleValue : l1) {
      if (styleValue.length() > 0) {
        stringList.push_back(styleValue);
      }
    }
  }
  return stringList;
}

void CDataSource::calculateScaleAndOffsetFromMinMax(float &scale, float &offset, float min, float max, float log) {
  if (log != 0.0f) {
    // CDBDebug("LOG = %f",log);
    min = log10(min) / log10(log);
    max = log10(max) / log10(log);
  }

  scale = 240 / (max - min);
  offset = min * (-scale);
}

CStyleConfiguration *CDataSource::getStyle() {
  if (_currentStyle == NULL) {
    if (_styles == NULL) {
      _styles = getStyleListForDataSource(this);
    }
    if (_styles->size() == 0) {
      CDBError("There are no styles available");
      return NULL;
    }
    CT::string styleName = "default";
    CT::string styles(srvParams->Styles.c_str());

    // TODO CHECK CDBDebug("Server Styles=%s",srvParam->Styles.c_str());
    CT::StackList<CT::string> layerstyles = styles.splitToStack(",");
    int layerIndex = datasourceIndex;
    if (layerstyles.size() != 0) {
      // Make sure default layer index is within the right bounds.
      if (layerIndex < 0) layerIndex = 0;
      if (layerIndex > ((int)layerstyles.size()) - 1) layerIndex = layerstyles.size() - 1;
      styleName = layerstyles[layerIndex].c_str();
      if (styleName.length() == 0) {
        styleName.copy("default");
      }
    }

    _currentStyle = _styles->get(0);

    for (size_t j = 0; j < _styles->size(); j++) {
      if (_styles->get(j)->styleCompositionName.equals(styleName)) {
        _currentStyle = _styles->get(j);
        break;
      }
    }
    // If not found, check for the style without rendermethod instead using startsWith.
    auto it = std::find_if(_styles->begin(), _styles->end(), [&styleName](CStyleConfiguration *a) { return a->styleCompositionName.startsWith(styleName); });
    if (it != _styles->end()) {
      _currentStyle = (*it);
      CDBDebug("Selected style %s", _currentStyle->styleName.c_str());
    } else {
      CT::string styleNameWithoutRenderMethod = styleName.substring(0, styleName.indexOf("/"));
      it = std::find_if(_styles->begin(), _styles->end(), [&styleNameWithoutRenderMethod](CStyleConfiguration *a) { return styleNameWithoutRenderMethod.equals(a->styleName); });
      if (it != _styles->end()) {
        _currentStyle = (*it);
        CDBDebug("Selected style %s", _currentStyle->styleName.c_str());
      }
    }

    if (_currentStyle->styleIndex == -1) {
      int status = _currentStyle->makeStyleConfig(this);
      if (status == -1) {
        _currentStyle->hasError = true;
      }
    }
    if (_currentStyle->legendIndex == -1) {
      std::vector<CT::string> legendList = getLegendListForDataSource(this, NULL);
      if (legendList.size() > 0) {
        _currentStyle->legendIndex = this->srvParams->getServerLegendIndexByName(legendList[0]);
      }
    }
  }

  return _currentStyle;
}

int CDataSource::setStyle(const char *styleName) {
  if (_styles == NULL) {
    _styles = getStyleListForDataSource(this);
  }
  if (_styles->size() == 0) {
    CDBError("There are no styles available");
    return 1;
  }

  _currentStyle = _styles->get(0);
  bool foundStyle = false;
  for (size_t j = 0; j < _styles->size(); j++) {
    if (_styles->get(j)->styleCompositionName.equals(styleName)) {

      _currentStyle = _styles->get(j);
      foundStyle = true;
      break;
    }
  }

  if (foundStyle == false) {
    CDBWarning("Unable to find style %s. Available styles:", styleName);
    for (size_t j = 0; j < _styles->size(); j++) {
      CDBWarning("  -%s", _styles->get(j)->styleCompositionName.c_str());
    }
  }

  if (_currentStyle->styleIndex == -1) {
    int status = _currentStyle->makeStyleConfig(this);
    if (status == -1) {
      _currentStyle->hasError = true;
    }
  }
  if (_currentStyle->legendIndex == -1) {
    std::vector<CT::string> legendList = getLegendListForDataSource(this, NULL);
    if (legendList.size() > 0) {
      _currentStyle->legendIndex = this->srvParams->getServerLegendIndexByName(legendList[0]);
    }
  }
  if (_currentStyle->hasError) return 1;
  return 0;
};

CDataSource *CDataSource::clone() {

  CDataSource *d = new CDataSource();
  d->dataSourceOwnsDataObject = false; // cdfObject stays with source datasource.
  d->_currentStyle = _currentStyle;
  d->datasourceIndex = datasourceIndex;
  d->currentAnimationStep = currentAnimationStep;

  /* Copy timesteps */
  for (size_t j = 0; j < timeSteps.size(); j++) {
    // CDBDebug("addStep for %s",fileName);
    TimeStep *timeStep = new TimeStep();
    d->timeSteps.push_back(timeStep);
    timeStep->fileName.copy(timeSteps[j]->fileName.c_str());
    timeStep->dims.copy(&timeSteps[j]->dims);
  }

  /* Copy dataObjects */
  for (size_t j = 0; j < dataObjects.size(); j++) {
    d->dataObjects.push_back(dataObjects[j]->clone());
  }

  d->stretchMinMax = stretchMinMax;
  d->stretchMinMaxDone = stretchMinMaxDone;

  /* Copy requireddims */
  for (size_t j = 0; j < requiredDims.size(); j++) {
    COGCDims *ogcDim = new COGCDims();
    d->requiredDims.push_back(ogcDim);
    ogcDim->name = requiredDims[j]->name;
    ogcDim->value = requiredDims[j]->value;
    ogcDim->queryValue = requiredDims[j]->queryValue;
    ogcDim->netCDFDimName = requiredDims[j]->netCDFDimName;
    ogcDim->hidden = requiredDims[j]->hidden;
    for (size_t i = 0; i < requiredDims[j]->uniqueValues.size(); i++) {
      ogcDim->uniqueValues.push_back(requiredDims[j]->uniqueValues[i].c_str());
    }
    ogcDim->isATimeDimension = requiredDims[j]->isATimeDimension;
  }

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
  if (this->getStyle() != NULL && this->getStyle()->styleConfig != NULL) {
    if (this->getStyle()->styleConfig->RenderSettings.size() > 0) {
      if (!this->getStyle()->styleConfig->RenderSettings[0]->attr.scalewidth.empty()) {
        double scaleWidth = this->getStyle()->styleConfig->RenderSettings[0]->attr.scalewidth.toDouble();
        double imageWidth = (double)this->srvParams->Geo->dWidth;
        return imageWidth / scaleWidth;
      }
    }
  }
  return 1;
}

double CDataSource::getContourScaling() {
  if (this->getStyle() != NULL && this->getStyle()->styleConfig != NULL) {
    if (this->getStyle()->styleConfig->RenderSettings.size() > 0) {
      if (!this->getStyle()->styleConfig->RenderSettings[0]->attr.scalecontours.empty()) {
        double scalecontours = this->getStyle()->styleConfig->RenderSettings[0]->attr.scalecontours.toDouble();
        return scalecontours;
      }
    }
  }
  return 1;
}

CDataSource::DataObject *CDataSource::getDataObjectByName(const char *name) {
  for (auto it = dataObjects.begin(); it != dataObjects.end(); ++it) {
    CDataSource::DataObject *dataObject = *it;

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

CDataSource::DataObject *CDataSource::getFirstAvailableDataObject() {
  for (size_t o = 0; o < this->getNumDataObjects(); o++) {
    if (this->getDataObject(o)->filterFromOutput) {
      continue;
    }
    return this->getDataObject(o);
  }
  CDBWarning("No dataobjects available");
  return nullptr;
}

CDataSource::DataObject *CDataSource::getDataObject(int j) {

  if (int(dataObjects.size()) <= j) {
    CDBError("No Data object witn nr %d (total %d) for animation step %d (total steps %d)", j, currentAnimationStep, dataObjects.size(), timeSteps.size());
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
      CDBError("attachCDFObject: variable nr %d \"%s\" does not exist", varNr, getDataObject(varNr)->variableName.c_str());
      return 1;
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
  size_t start[numDimensionsForVariableToRead];
  size_t count[numDimensionsForVariableToRead];
  ptrdiff_t stride[numDimensionsForVariableToRead];
  auto *cdfDims = this->getCDFDims();
  for (size_t dimNr = 0; dimNr < numDimensionsForVariableToRead; dimNr += 1) {
    auto *dimensionLink = variableToRead->dimensionlinks[dimNr];
    size_t startCountIndex = dimNr;
    start[startCountIndex] = 0;
    stride[startCountIndex] = 1;
    count[startCountIndex] = dimensionLink->getSize();
    int cdfDimIndex = cdfDims->getArrayIndexForName(dimensionLink->name.c_str());
    if (cdfDimIndex >= 0) {
#ifdef CDATASOURCE_DEBUG
      CDBDebug("Start %d/%d:%s %d:%s ==> %d", startCountIndex, dimNr, dimensionLink->name.c_str(), cdfDimIndex, cdfDims->getDimensionName(cdfDimIndex), cdfDims->getDimensionIndex(cdfDimIndex));
#endif
      start[startCountIndex] = cdfDims->getDimensionIndex(cdfDimIndex);
      count[startCountIndex] = 1;
    }
  }
  return variableToRead->readData(dataTypeToReturnData, start, count, stride, true);
}

std::string CDataSource::getDataSetName() { return std::string(this->srvParams->datasetLocation.c_str()); }
