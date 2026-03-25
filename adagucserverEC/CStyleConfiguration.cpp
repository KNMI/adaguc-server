#include "CStyleConfiguration.h"
#include "CDataSource.h"

RenderMethod getRenderMethodFromString(std::string renderMethodString) {
  RenderMethod renderMethod = RM_UNDEFINED;
  if (CT::indexOf(renderMethodString, "nearest") != -1) renderMethod |= RM_NEAREST;
  if (CT::indexOf(renderMethodString, "generic") != -1)
    renderMethod |= RM_GENERIC;
  else if (CT::indexOf(renderMethodString, "bilinear") != -1)
    renderMethod |= RM_BILINEAR;
  if (CT::indexOf(renderMethodString, "hillshaded") != -1)
    renderMethod |= RM_HILLSHADED;
  else if (CT::indexOf(renderMethodString, "shaded") != -1)
    renderMethod |= RM_SHADED;
  if (CT::indexOf(renderMethodString, "contour") != -1) renderMethod |= RM_CONTOUR;
  if (CT::indexOf(renderMethodString, "linearinterpolation") != -1) renderMethod |= RM_POINT_LINEARINTERPOLATION;
  if (CT::indexOf(renderMethodString, "point") != -1) renderMethod |= RM_POINT;
  if (CT::indexOf(renderMethodString, "vector") != -1) renderMethod |= RM_VECTOR;
  if (CT::indexOf(renderMethodString, "barb") != -1) renderMethod |= RM_BARB;
  if (CT::indexOf(renderMethodString, "thin") != -1) renderMethod |= RM_THIN;
  if (CT::indexOf(renderMethodString, "rgba") != -1) renderMethod |= RM_RGBA;
  if (CT::indexOf(renderMethodString, "stippling") != -1) renderMethod |= RM_STIPPLING;
  if (CT::indexOf(renderMethodString, "polyline") != -1) renderMethod |= RM_POLYLINE;
  if (CT::indexOf(renderMethodString, "polygon") != -1) renderMethod |= RM_POLYGON;

  return renderMethod;
}

std::string getRenderMethodAsString(RenderMethod renderMethod) {
  std::string renderMethodString;
  if (renderMethod == RM_UNDEFINED) {
    renderMethodString = "undefined";
    return renderMethodString;
  }
  renderMethodString = "";
  if (renderMethod & RM_NEAREST) renderMethodString += "nearest";
  if (renderMethod & RM_BILINEAR) renderMethodString += "bilinear";
  if (renderMethod & RM_SHADED) renderMethodString += "shaded";
  if (renderMethod & RM_CONTOUR) renderMethodString += "contour";
  if (renderMethod & RM_POINT) renderMethodString += "point";
  if (renderMethod & RM_VECTOR) renderMethodString += "vector";
  if (renderMethod & RM_BARB) renderMethodString += "barb";
  if (renderMethod & RM_THIN) renderMethodString += "thin";
  if (renderMethod & RM_RGBA) renderMethodString += "rgba";
  if (renderMethod & RM_STIPPLING) renderMethodString += "stippling";
  if (renderMethod & RM_POLYLINE) renderMethodString += "polyline";
  if (renderMethod & RM_POLYGON) renderMethodString += "polygon";
  if (renderMethod & RM_POINT_LINEARINTERPOLATION) renderMethodString += "linearinterpolation";
  if (renderMethod & RM_HILLSHADED) renderMethodString += "hillshaded";
  if (renderMethod & RM_GENERIC) renderMethodString += "generic";
  return renderMethodString;
}

std::string CStyleConfiguration::dump() {
  std::string data = CT::printf("name = %s\n", styleCompositionName.c_str());
  CT::printfconcat(data, "minMaxSet = %d\n", minMaxSet);
  CT::printfconcat(data, "hasLegendValueRange = %d\n", hasLegendValueRange);
  CT::printfconcat(data, "hasError = %d\n", hasError);
  CT::printfconcat(data, "legendHasFixedMinMax = %d\n", legendHasFixedMinMax);
  CT::printfconcat(data, "smoothingFilter = %d\n", smoothingFilter);
  CT::printfconcat(data, "legendIndex = %d\n", legendIndex);
  CT::printfconcat(data, "styleIndex = %d\n", styleIndex);
  CT::printfconcat(data, "shadeInterval = %f\n", shadeInterval);
  CT::printfconcat(data, "contourIntervalL = %f\n", contourIntervalL);
  CT::printfconcat(data, "contourIntervalH = %f\n", contourIntervalH);
  CT::printfconcat(data, "legendScale = %f\n", legendScale);
  CT::printfconcat(data, "legendOffset = %f\n", legendOffset);
  CT::printfconcat(data, "legendLog = %f\n", legendLog);
  CT::printfconcat(data, "legendLowerRange = %f\n", legendLowerRange);
  CT::printfconcat(data, "legendUpperRange; = %f\n", legendUpperRange);
  CT::printfconcat(data, "legendTickInterval = %f\n", legendTickInterval);
  CT::printfconcat(data, "legendTickRound = %f\n", legendTickRound);
  CT::printfconcat(data, "minValue = %f\n", minValue);
  CT::printfconcat(data, "maxValue = %f\n", maxValue);
  CT::printfconcat(data, "renderMethod = %s\n", getRenderMethodAsString(renderMethod).c_str());
  CT::printfconcat(data, "legendName %s\n", legendName.c_str());
  CT::printfconcat(data, "styleCompositionName %s\n", styleCompositionName.c_str());
  CT::printfconcat(data, "styleTitle %s\n", styleTitle.c_str());
  CT::printfconcat(data, "styleAbstract %s\n", styleAbstract.c_str());
  int a = 0;
  for (auto renderSetting: renderSettings) {
    CT::printfconcat(data, "renderSetting %d) = [%s] [%s]\n", a, renderSetting->attr.renderhint.c_str(), renderSetting->attr.interpolationmethod.c_str());
    a++;
  }
  a = 0;
  for (const auto &shadeInterval: shadeIntervals) {
    CT::printfconcat(data, "shadeInterval %d) =  [%s] [%s]\n", a++, shadeInterval.attr.label.c_str(), shadeInterval.attr.label.c_str());
  }
  a = 0;
  for (auto contourLine: contourLines) {
    CT::printfconcat(data, "contourLine %d) =  [%s] [%s] [%s]\n", a++, contourLine->attr.linecolor.c_str(), contourLine->attr.interval.c_str(), contourLine->attr.classes.c_str());
  }
  a = 0;
  for (auto symbolInterval: symbolIntervals) {
    CT::printfconcat(data, "symbolInterval %d) =  [%s] [%s]\n", a, symbolInterval->attr.min.c_str(), symbolInterval->attr.max.c_str());
    a++;
  }
  a = 0;
  for (auto featureInterval: featureIntervals) {
    CT::printfconcat(data, "featureInterval %d) =  [%s] [%s]\n", a, featureInterval->attr.fillcolor.c_str(), featureInterval->attr.label.c_str());
    a++;
  }

  return data;
}

void parseStyleInfo(CStyleConfiguration *styleConfig, CDataSource *dataSource, int styleIndex, int depth) {
  // Get info from style
  CServerConfig::XMLE_Style *style = dataSource->cfg->Style[styleIndex];

  //  INCLUDE other styles
  for (auto includeStyle: style->IncludeStyle) {
    int extraStyle = dataSource->srvParams->getServerStyleIndexByName(includeStyle->attr.name);
    if (extraStyle >= 0) {
      parseStyleInfo(styleConfig, dataSource, extraStyle, depth + 1);
    }
  }

  if (style->Scale.size() > 0) styleConfig->legendScale = parseFloat(style->Scale[0]->value.c_str());
  if (style->Offset.size() > 0) styleConfig->legendOffset = parseFloat(style->Offset[0]->value.c_str());
  if (style->Log.size() > 0) styleConfig->legendLog = parseFloat(style->Log[0]->value.c_str());

  if (style->ContourIntervalL.size() > 0) {
    styleConfig->contourIntervalL = parseFloat(style->ContourIntervalL[0]->value.c_str());
    styleConfig->shadeInterval = styleConfig->contourIntervalL;
  }
  if (style->ContourIntervalH.size() > 0) styleConfig->contourIntervalH = parseFloat(style->ContourIntervalH[0]->value.c_str());

  if (style->ShadeInterval.size() > 0) styleConfig->shadeInterval = parseFloat(style->ShadeInterval[0]->value.c_str());
  if (style->SmoothingFilter.size() > 0) styleConfig->smoothingFilter = parseInt(style->SmoothingFilter[0]->value.c_str());

  if (style->ValueRange.size() > 0) {
    styleConfig->hasLegendValueRange = true;
    styleConfig->legendLowerRange = parseFloat(style->ValueRange[0]->attr.min.c_str());
    styleConfig->legendUpperRange = parseFloat(style->ValueRange[0]->attr.max.c_str());
  }

  if (style->Min.size() > 0) {
    styleConfig->minValue = style->Min[0]->value.toDouble();
    styleConfig->minMaxSet = true;
  }
  if (style->Max.size() > 0) {
    styleConfig->maxValue = style->Max[0]->value.toDouble();
    styleConfig->minMaxSet = true;
  }

  styleConfig->contourLines.insert(styleConfig->contourLines.end(), style->ContourLine.begin(), style->ContourLine.end());
  styleConfig->renderSettings.insert(styleConfig->renderSettings.end(), style->RenderSettings.begin(), style->RenderSettings.end());
  styleConfig->smoothingFilterVector.insert(styleConfig->smoothingFilterVector.end(), style->SmoothingFilter.begin(), style->SmoothingFilter.end());
  for (const auto shadeInterval: style->ShadeInterval) {
    styleConfig->shadeIntervals.push_back(*shadeInterval);
  }
  styleConfig->symbolIntervals.insert(styleConfig->symbolIntervals.end(), style->SymbolInterval.begin(), style->SymbolInterval.end());
  styleConfig->featureIntervals.insert(styleConfig->featureIntervals.end(), style->FeatureInterval.begin(), style->FeatureInterval.end());
  styleConfig->pointIntervals.insert(styleConfig->pointIntervals.end(), style->Point.begin(), style->Point.end());
  styleConfig->vectorIntervals.insert(styleConfig->vectorIntervals.end(), style->Vector.begin(), style->Vector.end());
  styleConfig->dataPostProcessors.insert(styleConfig->dataPostProcessors.end(), style->DataPostProc.begin(), style->DataPostProc.end());
  styleConfig->stipplingList.insert(styleConfig->stipplingList.end(), style->Stippling.begin(), style->Stippling.end());
  styleConfig->filterPointList.insert(styleConfig->filterPointList.end(), style->FilterPoints.begin(), style->FilterPoints.end());
  styleConfig->thinningList.insert(styleConfig->thinningList.end(), style->Thinning.begin(), style->Thinning.end());

  if (style->Legend.size() > 0) {
    styleConfig->legend = (*style->Legend[0]);
  }
  if (style->LegendGraphic.size() > 0) {
    styleConfig->legendGraphic = (*style->LegendGraphic[0]);
  }

  if (style->Legend.size() > 0) {
    if (style->Legend[0]->attr.tickinterval.empty() == false) {
      styleConfig->legendTickInterval = parseDouble(style->Legend[0]->attr.tickinterval.c_str());
    }
    if (style->Legend[0]->attr.tickround.empty() == false) {
      styleConfig->legendTickRound = parseDouble(style->Legend[0]->attr.tickround.c_str());
    }

    if (style->Legend[0]->attr.fixedclasses.equals("true")) {
      styleConfig->legendHasFixedMinMax = true;
    } else if (style->Legend[0]->attr.fixedclasses.equals("false")) {
      styleConfig->legendHasFixedMinMax = false;
    }
    styleConfig->legendName = style->Legend[0]->value;
  }

  if (depth == 0) {
    if (!style->attr.abstract.empty()) {
      styleConfig->styleAbstract = style->attr.abstract;
    }
    if (!style->attr.title.empty()) {
      styleConfig->styleTitle = style->attr.title;
    }
  }
}

int CStyleConfiguration::makeStyleConfig(CDataSource *dataSource) {

  this->shadeInterval = 0.0f;
  this->contourIntervalL = 0.0f;
  this->contourIntervalH = 0.0f;
  this->legendScale = 0.0f;
  this->legendOffset = 0.0f;
  this->legendLog = 0.0f;
  this->legendLowerRange = 0.0f;
  this->legendUpperRange = 0.0f;
  this->smoothingFilter = 0;
  this->hasLegendValueRange = false;
  this->minValue = 0.0f;
  this->maxValue = 0.0f;
  this->minMaxSet = false;
  // this->renderMethod = RM_UNDEFINED;
  if (dataSource->cfg->Style.size() == 0) {
    CDBError("Server configuration has no styles at all.");
    return 1;
  }

  if (this->styleIndex == -1) {

    int styleIndex = dataSource->srvParams->getServerStyleIndexByName("auto");
    if (styleIndex < 0) {
      styleIndex = 0;
    }
    this->styleIndex = styleIndex;
    CDBWarning("No styles configured, taking style [%s]", dataSource->cfg->Style[this->styleIndex]->attr.name.c_str());
  }

  parseStyleInfo(this, dataSource, this->styleIndex, 0);

  // Legend settings can always be overriden in the layer itself!
  CServerConfig::XMLE_Layer *layer = dataSource->cfgLayer;
  if (layer->Scale.size() > 0) this->legendScale = parseFloat(layer->Scale[0]->value.c_str());
  if (layer->Offset.size() > 0) this->legendOffset = parseFloat(layer->Offset[0]->value.c_str());
  if (layer->Log.size() > 0) this->legendLog = parseFloat(layer->Log[0]->value.c_str());

  if (layer->ContourIntervalL.size() > 0) this->contourIntervalL = parseFloat(layer->ContourIntervalL[0]->value.c_str());
  if (layer->ContourIntervalH.size() > 0) this->contourIntervalH = parseFloat(layer->ContourIntervalH[0]->value.c_str());
  if (this->shadeInterval == 0.0f) this->shadeInterval = this->contourIntervalL;
  if (layer->ShadeInterval.size() > 0) this->shadeInterval = parseFloat(layer->ShadeInterval[0]->value.c_str());
  if (layer->SmoothingFilter.size() > 0) this->smoothingFilter = parseInt(layer->SmoothingFilter[0]->value.c_str());

  if (layer->ValueRange.size() > 0) {
    this->hasLegendValueRange = true;
    this->legendLowerRange = parseFloat(layer->ValueRange[0]->attr.min.c_str());
    this->legendUpperRange = parseFloat(layer->ValueRange[0]->attr.max.c_str());
  }

  if (layer->Min.size() > 0) {
    this->minValue = layer->Min[0]->value.toDouble();
    this->minMaxSet = true;
  }
  if (layer->Max.size() > 0) {
    this->maxValue = layer->Max[0]->value.toDouble();
    this->minMaxSet = true;
  }

  if (layer->ContourLine.size() > 0) {
    this->contourLines.assign(layer->ContourLine.begin(), layer->ContourLine.end());
  }
  if (layer->ShadeInterval.size() > 0) {
    this->shadeIntervals.clear();
    for (const auto shadeInterval: layer->ShadeInterval) {
      this->shadeIntervals.push_back(*shadeInterval);
    }
  }
  if (layer->FeatureInterval.size() > 0) {
    this->featureIntervals.assign(layer->FeatureInterval.begin(), layer->FeatureInterval.end());
  }

  if (layer->Legend.size() > 0) {
    if (layer->Legend[0]->attr.tickinterval.empty() == false) {
      this->legendTickInterval = parseDouble(layer->Legend[0]->attr.tickinterval.c_str());
    }
    if (layer->Legend[0]->attr.tickround.empty() == false) {
      this->legendTickRound = parseDouble(layer->Legend[0]->attr.tickround.c_str());
    }
    if (layer->Legend[0]->attr.fixedclasses.equals("true")) {
      this->legendHasFixedMinMax = true;
    } else if (layer->Legend[0]->attr.fixedclasses.equals("false")) {
      this->legendHasFixedMinMax = false;
    }
    this->legendName = layer->Legend[0]->value;
  }

  this->legendIndex = dataSource->srvParams->getServerLegendIndexByName(this->legendName);
  // Min and max can again be overriden by WMS extension settings
  if (dataSource->srvParams->wmsExtensions.colorScaleRangeSet) {
    this->minMaxSet = true;
    this->minValue = dataSource->srvParams->wmsExtensions.colorScaleRangeMin;
    this->maxValue = dataSource->srvParams->wmsExtensions.colorScaleRangeMax;
  }
  // Log can again be overriden by WMS extension settings
  if (dataSource->srvParams->wmsExtensions.logScale) {
    this->legendLog = 10;
  }

  int index = CT::indexOf(styleCompositionName, "/");
  if (index > 0) {
    std::string renderMethodString = index > 0 ? CT::substring(styleCompositionName, index, -1) : "";
    this->renderMethod = getRenderMethodFromString(renderMethodString);
  }
  if (this->renderMethod == RM_UNDEFINED) {
    this->renderMethod = RM_GENERIC;
  }

  if (dataSource->srvParams->wmsExtensions.numColorBandsSet) {
    float interval = (this->maxValue - this->minValue) / dataSource->srvParams->wmsExtensions.numColorBands;
    this->shadeInterval = interval;
    this->contourIntervalL = interval;
    if (dataSource->srvParams->wmsExtensions.numColorBands > 0 && dataSource->srvParams->wmsExtensions.numColorBands < 300) {
      this->legendTickInterval = int((this->maxValue - this->minValue) / double(dataSource->srvParams->wmsExtensions.numColorBands) + 0.5);
    }
  }

  // When min and max are given, calculate the scale and offset according to min and max.
  if (this->minMaxSet) {
#ifdef CDATASOURCE_DEBUG
    CDBDebug("Found min and max in layer configuration");
#endif

    stretchLegend(this->minValue, this->maxValue);
    dataSource->stretchMinMax = false;
  }

  if (this->legendIndex == -1) {
    if (dataSource->cfg->Legend.size() > 0) {
      this->legendIndex = 0;
      this->legendName = dataSource->cfg->Legend[0]->attr.name;
    } else {
      CDBError("Server has no legends configured. Cannot continue.");
      return 1;
    }
  }

  return 0;
}

void CStyleConfiguration::stretchLegend(double min, double max) {
  if (this->legendLog != 0.0f) {
    // CDBDebug("LOG = %f",log);
    min = log10(min) / log10(this->legendLog);
    max = log10(max) / log10(this->legendLog);
  }

  // Make sure that there is always a range in between the min and max.
  if (max == min) max = min + 0.1;
  // Calculate legendOffset legendScale
  float ls = 240 / (max - min);
  float lo = -(min * ls);
  this->legendScale = ls;
  this->legendOffset = lo;

  // Check for infinities
  if (this->legendScale != this->legendScale || this->legendScale == INFINITY || std::isnan(this->legendScale) || this->legendScale == -INFINITY) {
    this->legendScale = 240;
  }
  if (this->legendOffset != this->legendOffset || this->legendOffset == INFINITY || std::isnan(this->legendOffset) || this->legendOffset == -INFINITY) {
    this->legendOffset = 0;
  }
}
