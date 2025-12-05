#include "CStyleConfiguration.h"
#include "CDataSource.h"

RenderMethod getRenderMethodFromString(CT::string renderMethodString) {
  RenderMethod renderMethod = RM_UNDEFINED;
  if (renderMethodString.indexOf("nearest") != -1) renderMethod |= RM_NEAREST;
  if (renderMethodString.indexOf("generic") != -1)
    renderMethod |= RM_GENERIC;
  else if (renderMethodString.indexOf("bilinear") != -1)
    renderMethod |= RM_BILINEAR;
  if (renderMethodString.indexOf("hillshaded") != -1)
    renderMethod |= RM_HILLSHADED;
  else if (renderMethodString.indexOf("shaded") != -1)
    renderMethod |= RM_SHADED;
  if (renderMethodString.indexOf("contour") != -1) renderMethod |= RM_CONTOUR;
  if (renderMethodString.indexOf("linearinterpolation") != -1) renderMethod |= RM_POINT_LINEARINTERPOLATION;
  if (renderMethodString.indexOf("point") != -1) renderMethod |= RM_POINT;
  if (renderMethodString.indexOf("vector") != -1) renderMethod |= RM_VECTOR;
  if (renderMethodString.indexOf("barb") != -1) renderMethod |= RM_BARB;
  if (renderMethodString.indexOf("thin") != -1) renderMethod |= RM_THIN;
  if (renderMethodString.indexOf("rgba") != -1) renderMethod |= RM_RGBA;
  if (renderMethodString.indexOf("stippling") != -1) renderMethod |= RM_STIPPLING;
  if (renderMethodString.indexOf("polyline") != -1) renderMethod |= RM_POLYLINE;
  if (renderMethodString.indexOf("polygon") != -1) renderMethod |= RM_POLYGON;

  return renderMethod;
}

CT::string getRenderMethodAsString(RenderMethod renderMethod) {
  CT::string renderMethodString;
  if (renderMethod == RM_UNDEFINED) {
    renderMethodString.copy("undefined");
    return renderMethodString;
  }
  renderMethodString.copy("");
  if (renderMethod & RM_NEAREST) renderMethodString.concat("nearest");
  if (renderMethod & RM_BILINEAR) renderMethodString.concat("bilinear");
  if (renderMethod & RM_SHADED) renderMethodString.concat("shaded");
  if (renderMethod & RM_CONTOUR) renderMethodString.concat("contour");
  if (renderMethod & RM_POINT) renderMethodString.concat("point");
  if (renderMethod & RM_VECTOR) renderMethodString.concat("vector");
  if (renderMethod & RM_BARB) renderMethodString.concat("barb");
  if (renderMethod & RM_THIN) renderMethodString.concat("thin");
  if (renderMethod & RM_RGBA) renderMethodString.concat("rgba");
  if (renderMethod & RM_STIPPLING) renderMethodString.concat("stippling");
  if (renderMethod & RM_POLYLINE) renderMethodString.concat("polyline");
  if (renderMethod & RM_POLYGON) renderMethodString.concat("polygon");
  if (renderMethod & RM_POINT_LINEARINTERPOLATION) renderMethodString.concat("linearinterpolation");
  if (renderMethod & RM_HILLSHADED) renderMethodString.concat("hillshaded");
  if (renderMethod & RM_GENERIC) renderMethodString.concat("generic");
  return renderMethodString;
}

CT::string CStyleConfiguration::dump() {
  CT::string data;
  data.print("name = %s\n", styleCompositionName.c_str());
  data.printconcat("minMaxSet = %d\n", minMaxSet);
  data.printconcat("hasLegendValueRange = %d\n", hasLegendValueRange);
  data.printconcat("hasError = %d\n", hasError);
  data.printconcat("legendHasFixedMinMax = %d\n", legendHasFixedMinMax);
  data.printconcat("smoothingFilter = %d\n", smoothingFilter);
  data.printconcat("legendIndex = %d\n", legendIndex);
  data.printconcat("styleIndex = %d\n", styleIndex);
  data.printconcat("shadeInterval = %f\n", shadeInterval);
  data.printconcat("contourIntervalL = %f\n", contourIntervalL);
  data.printconcat("contourIntervalH = %f\n", contourIntervalH);
  data.printconcat("legendScale = %f\n", legendScale);
  data.printconcat("legendOffset = %f\n", legendOffset);
  data.printconcat("legendLog = %f\n", legendLog);
  data.printconcat("legendLowerRange = %f\n", legendLowerRange);
  data.printconcat("legendUpperRange; = %f\n", legendUpperRange);
  data.printconcat("legendTickInterval = %f\n", legendTickInterval);
  data.printconcat("legendTickRound = %f\n", legendTickRound);
  data.printconcat("minValue = %f\n", minValue);
  data.printconcat("maxValue = %f\n", maxValue);
  data.printconcat("renderMethod = %s\n", getRenderMethodAsString(renderMethod).c_str());
  data.printconcat("legendName %s\n", legendName.c_str());
  data.printconcat("styleCompositionName %s\n", styleCompositionName.c_str());
  data.printconcat("styleTitle %s\n", styleTitle.c_str());
  data.printconcat("styleAbstract %s\n", styleAbstract.c_str());
  int a = 0;
  for (auto renderSetting : renderSettings) {
    data.printconcat("renderSetting %d) = [%s] [%s]\n", a, renderSetting->attr.renderhint.c_str(), renderSetting->attr.interpolationmethod.c_str());
    a++;
  }
  a = 0;
  for (auto shadeInterval : shadeIntervals) {
    data.printconcat("shadeInterval %d) =  [%s] [%s]\n", a++, shadeInterval->attr.label.c_str(), shadeInterval->attr.label.c_str());
  }
  a = 0;
  for (auto contourLine : contourLines) {
    data.printconcat("contourLine %d) =  [%s] [%s] [%s]\n", a++, contourLine->attr.linecolor.c_str(), contourLine->attr.interval.c_str(), contourLine->attr.classes.c_str());
  }
  a = 0;
  for (auto symbolInterval : symbolIntervals) {
    data.printconcat("symbolInterval %d) =  [%s] [%s]\n", a, symbolInterval->attr.min.c_str(), symbolInterval->attr.max.c_str());
    a++;
  }
  a = 0;
  for (auto featureInterval : featureIntervals) {
    data.printconcat("featureInterval %d) =  [%s] [%s]\n", a, featureInterval->attr.fillcolor.c_str(), featureInterval->attr.label.c_str());
    a++;
  }

  return data;
}

void parseStyleInfo(CStyleConfiguration *styleConfig, CDataSource *dataSource, int styleIndex, int depth) {
  // Get info from style
  CServerConfig::XMLE_Style *style = dataSource->cfg->Style[styleIndex];
  styleConfig->styleConfig = style;

  //  INCLUDE other styles
  for (auto includeStyle : style->IncludeStyle) {
    int extraStyle = dataSource->srvParams->getServerStyleIndexByName(includeStyle->attr.name);
    if (extraStyle >= 0) {
      // CDBDebug("Include style %d - %s", extraStyle, dataSource->cfg->Style[extraStyle]->attr.name.c_str());
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
  styleConfig->shadeIntervals.insert(styleConfig->shadeIntervals.end(), style->ShadeInterval.begin(), style->ShadeInterval.end());
  styleConfig->symbolIntervals.insert(styleConfig->symbolIntervals.end(), style->SymbolInterval.begin(), style->SymbolInterval.end());
  styleConfig->featureIntervals.insert(styleConfig->featureIntervals.end(), style->FeatureInterval.begin(), style->FeatureInterval.end());

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
    this->shadeIntervals.assign(layer->ShadeInterval.begin(), layer->ShadeInterval.end());
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

  int index = styleCompositionName.indexOf("/");
  if (index > 0) {
    CT::string renderMethodString = index > 0 ? styleCompositionName.substring(index, -1) : "";
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
    CDataSource::calculateScaleAndOffsetFromMinMax(this->legendScale, this->legendOffset, this->minValue, this->maxValue, this->legendLog);

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
