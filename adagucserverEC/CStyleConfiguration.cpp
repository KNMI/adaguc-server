#include "CStyleConfiguration.h"
#include "CDataSource.h"

CStyleConfiguration::RenderMethod CStyleConfiguration::getRenderMethodFromString(CT::string *renderMethodString) {
  RenderMethod renderMethod = RM_UNDEFINED;
  if (renderMethodString->indexOf("nearest") != -1) renderMethod |= RM_NEAREST;
  if (renderMethodString->indexOf("generic") != -1)
    renderMethod |= RM_GENERIC;
  else if (renderMethodString->indexOf("bilinear") != -1)
    renderMethod |= RM_BILINEAR;
  if (renderMethodString->indexOf("hillshaded") != -1)
    renderMethod |= RM_HILLSHADED;
  else if (renderMethodString->indexOf("shaded") != -1)
    renderMethod |= RM_SHADED;
  if (renderMethodString->indexOf("contour") != -1) renderMethod |= RM_CONTOUR;
  if (renderMethodString->indexOf("linearinterpolation") != -1) renderMethod |= RM_POINT_LINEARINTERPOLATION;
  if (renderMethodString->indexOf("point") != -1) renderMethod |= RM_POINT;
  if (renderMethodString->indexOf("vector") != -1) renderMethod |= RM_VECTOR;
  if (renderMethodString->indexOf("barb") != -1) renderMethod |= RM_BARB;
  if (renderMethodString->indexOf("thin") != -1) renderMethod |= RM_THIN;
  if (renderMethodString->indexOf("avg_rgba") != -1) renderMethod |= RM_AVG_RGBA;
  if (renderMethodString->indexOf("rgba") != -1) renderMethod |= RM_RGBA;

  if (renderMethodString->indexOf("stippling") != -1) renderMethod |= RM_STIPPLING;
  if (renderMethodString->indexOf("polyline") != -1) renderMethod |= RM_POLYLINE;

  /* When RM_AVG_RGBA is requested, disable RM_RGBA, otherwise two RGBA rendereres come into action */
  if (renderMethod & RM_AVG_RGBA) {
    renderMethod &= ~RM_RGBA;
  }

  return renderMethod;
}

void CStyleConfiguration::getRenderMethodAsString(CT::string *renderMethodString, RenderMethod renderMethod) {

  if (renderMethod == RM_UNDEFINED) {
    renderMethodString->copy("undefined");
    return;
  }
  renderMethodString->copy("");
  if (renderMethod & RM_NEAREST) renderMethodString->concat("nearest");
  if (renderMethod & RM_BILINEAR) renderMethodString->concat("bilinear");
  if (renderMethod & RM_SHADED) renderMethodString->concat("shaded");
  if (renderMethod & RM_CONTOUR) renderMethodString->concat("contour");
  if (renderMethod & RM_POINT) renderMethodString->concat("point");
  if (renderMethod & RM_VECTOR) renderMethodString->concat("vector");
  if (renderMethod & RM_BARB) renderMethodString->concat("barb");
  if (renderMethod & RM_THIN) renderMethodString->concat("thin");
  if (renderMethod & RM_RGBA) renderMethodString->concat("rgba");
  if (renderMethod & RM_AVG_RGBA) renderMethodString->concat("avg_rgba");
  if (renderMethod & RM_STIPPLING) renderMethodString->concat("stippling");
  if (renderMethod & RM_POLYLINE) renderMethodString->concat("polyline");
  if (renderMethod & RM_POINT_LINEARINTERPOLATION) renderMethodString->concat("linearinterpolation");
  if (renderMethod & RM_HILLSHADED) renderMethodString->concat("hillshaded");
  if (renderMethod & RM_GENERIC) renderMethodString->concat("generic");
}

CT::string CStyleConfiguration::c_str() {
  CT::string data;
  data.print("name = %s\n", styleCompositionName.c_str());
  data.printconcat("shadeInterval = %f\n", shadeInterval);
  data.printconcat("contourIntervalL = %f\n", contourIntervalL);
  data.printconcat("contourIntervalH = %f\n", contourIntervalH);
  data.printconcat("legendScale = %f\n", legendScale);
  data.printconcat("legendOffset = %f\n", legendOffset);
  data.printconcat("legendLog = %f\n", legendLog);
  data.printconcat("hasLegendValueRange = %d\n", hasLegendValueRange);
  data.printconcat("legendLowerRange = %f\n", legendLowerRange);
  data.printconcat("legendUpperRange = %f\n", legendUpperRange);
  data.printconcat("smoothingFilter = %d\n", smoothingFilter);
  data.printconcat("legendTickRound = %f\n", legendTickRound);
  data.printconcat("legendTickInterval = %f\n", legendTickInterval);
  data.printconcat("legendIndex = %d\n", legendIndex);
  data.printconcat("styleIndex = %d\n", styleIndex);
  // TODO
  CT::string rMethodString;
  getRenderMethodAsString(&rMethodString, renderMethod);
  data.printconcat("renderMethod = %s", rMethodString.c_str());
  return data;
}

CStyleConfiguration::CStyleConfiguration() { reset(); }

void CStyleConfiguration::reset() {
  shadeInterval = 0;
  contourIntervalL = 0;
  contourIntervalH = 0;
  legendScale = 1;
  legendOffset = 0;
  legendLog = 0.0f;
  legendLowerRange = 0;
  legendUpperRange = 0;
  smoothingFilter = 0;
  hasLegendValueRange = false;
  hasError = false;
  legendHasFixedMinMax = false;
  legendTickInterval = 0;
  legendTickRound = 0.0;
  legendIndex = -1;
  styleIndex = -1;
  contourLines = NULL;
  shadeIntervals = NULL;
  featureIntervals = NULL;
  symbolIntervals = NULL;
  styleCompositionName = "";
  styleTitle = "";
  styleAbstract = "";
  styleConfig = NULL;
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

  float min = 0.0f;
  float max = 0.0f;
  this->minMaxSet = false;

  if (this->styleIndex != -1) {
    // Get info from style
    CServerConfig::XMLE_Style *style = dataSource->cfg->Style[this->styleIndex];
    this->styleConfig = style;
    if (style->Scale.size() > 0) this->legendScale = parseFloat(style->Scale[0]->value.c_str());
    if (style->Offset.size() > 0) this->legendOffset = parseFloat(style->Offset[0]->value.c_str());
    if (style->Log.size() > 0) this->legendLog = parseFloat(style->Log[0]->value.c_str());

    if (style->ContourIntervalL.size() > 0) this->contourIntervalL = parseFloat(style->ContourIntervalL[0]->value.c_str());
    if (style->ContourIntervalH.size() > 0) this->contourIntervalH = parseFloat(style->ContourIntervalH[0]->value.c_str());
    this->shadeInterval = this->contourIntervalL;
    if (style->ShadeInterval.size() > 0) this->shadeInterval = parseFloat(style->ShadeInterval[0]->value.c_str());
    if (style->SmoothingFilter.size() > 0) this->smoothingFilter = parseInt(style->SmoothingFilter[0]->value.c_str());

    if (style->ValueRange.size() > 0) {
      this->hasLegendValueRange = true;
      this->legendLowerRange = parseFloat(style->ValueRange[0]->attr.min.c_str());
      this->legendUpperRange = parseFloat(style->ValueRange[0]->attr.max.c_str());
    }

    if (style->Min.size() > 0) {
      min = parseFloat(style->Min[0]->value.c_str());
      this->minMaxSet = true;
    }
    if (style->Max.size() > 0) {
      max = parseFloat(style->Max[0]->value.c_str());
      this->minMaxSet = true;
    }

    this->contourLines = &style->ContourLine;
    this->shadeIntervals = &style->ShadeInterval;
    this->symbolIntervals = &style->SymbolInterval;
    this->featureIntervals = &style->FeatureInterval;

    if (style->Legend.size() > 0) {
      if (style->Legend[0]->attr.tickinterval.empty() == false) {
        this->legendTickInterval = parseDouble(style->Legend[0]->attr.tickinterval.c_str());
      }
      if (style->Legend[0]->attr.tickround.empty() == false) {
        this->legendTickRound = parseDouble(style->Legend[0]->attr.tickround.c_str());
      }
      if (style->Legend[0]->attr.fixedclasses.equals("true")) {
        this->legendHasFixedMinMax = true;
      }
    }
  }

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
    min = parseFloat(layer->Min[0]->value.c_str());
    this->minMaxSet = true;
  }
  if (layer->Max.size() > 0) {
    max = parseFloat(layer->Max[0]->value.c_str());
    this->minMaxSet = true;
  }

  if (layer->ContourLine.size() > 0) {
    this->contourLines = &layer->ContourLine;
  }
  if (layer->ShadeInterval.size() > 0) {
    this->shadeIntervals = &layer->ShadeInterval;
  }
  if (layer->FeatureInterval.size() > 0) {
    this->featureIntervals = &layer->FeatureInterval;
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
    }
  }

  // Min and max can again be overriden by WMS extension settings
  if (dataSource->srvParams->wmsExtensions.colorScaleRangeSet) {
    this->minMaxSet = true;
    min = dataSource->srvParams->wmsExtensions.colorScaleRangeMin;
    max = dataSource->srvParams->wmsExtensions.colorScaleRangeMax;
  }
  // Log can again be overriden by WMS extension settings
  if (dataSource->srvParams->wmsExtensions.logScale) {
    this->legendLog = 10;
  }

  if (dataSource->srvParams->wmsExtensions.numColorBandsSet) {
    float interval = (max - min) / dataSource->srvParams->wmsExtensions.numColorBands;
    this->shadeInterval = interval;
    this->contourIntervalL = interval;
    if (dataSource->srvParams->wmsExtensions.numColorBands > 0 && dataSource->srvParams->wmsExtensions.numColorBands < 300) {
      this->legendTickInterval = int((max - min) / double(dataSource->srvParams->wmsExtensions.numColorBands) + 0.5);
      // this->legendTickRound = this->legendTickInterval;//pow(10,(log10(this->legendTickInterval)-1));
      // if(this->legendTickRound > 0.1)this->legendTickRound =0.1;
      //
    }
  }

  // When min and max are given, calculate the scale and offset according to min and max.
  if (this->minMaxSet) {
#ifdef CDATASOURCE_DEBUG
    CDBDebug("Found min and max in layer configuration");
#endif
    CDataSource::calculateScaleAndOffsetFromMinMax(this->legendScale, this->legendOffset, min, max, this->legendLog);

    dataSource->stretchMinMax = false;
    // this->legendScale=240/(max-min);
    // this->legendOffset=min*(-this->legendScale);
  }

  // Some safety checks, we cannot create contourlines with negative values.
  /*if(this->contourIntervalL<=0.0f||this->contourIntervalH<=0.0f){
    if(this->renderMethod==contour||
      this->renderMethod==bilinearcontour||
      this->renderMethod==nearestcontour){
      this->renderMethod=nearest;
      }
  }*/
  CT::string styleDump = this->c_str();
  //   #ifdef CDATASOURCE_DEBUG
  //
  //   CDBDebug("styleDump:\n%s",styleDump.c_str());
  //   #endif
  return 0;
}
