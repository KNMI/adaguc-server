#include "CStyleConfiguration.h"
#include "CXMLParser.h"
#include "CDebugger.h"
#include "CDataSource.h"
#include "CTString.h"

static const bool CDATASOURCE_DEBUG = false;

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
  for (const auto &renderSetting: renderSettings) {
    CT::printfconcat(data, "renderSetting %d) = [%s] [%s]\n", a, renderSetting.attr.renderhint.c_str(), renderSetting.attr.interpolationmethod.c_str());
    a++;
  }
  a = 0;
  for (const auto &shadeInterval: shadeIntervals) {
    CT::printfconcat(data, "shadeInterval %d) =  [%s] [%s]\n", a++, shadeInterval.attr.label.c_str(), shadeInterval.attr.label.c_str());
  }
  a = 0;
  for (const auto &contourLine: contourLines) {
    CT::printfconcat(data, "contourLine %d) =  [%s] [%s] [%s]\n", a++, contourLine.attr.linecolor.c_str(), contourLine.attr.interval.c_str(), contourLine.attr.classes.c_str());
  }
  a = 0;
  for (const auto &symbolInterval: symbolIntervals) {
    CT::printfconcat(data, "symbolInterval %d) =  [%s] [%s]\n", a, symbolInterval.attr.min.c_str(), symbolInterval.attr.max.c_str());
    a++;
  }
  a = 0;
  for (const auto &featureInterval: featureIntervals) {
    CT::printfconcat(data, "featureInterval %d) =  [%s] [%s]\n", a, featureInterval.attr.fillcolor.c_str(), featureInterval.attr.label.c_str());
    a++;
  }

  return data;
}

void parseStyleInfo(CStyleConfiguration *styleConfig, CDataSource *dataSource, int styleIndex, int depth) {
  // Get info from style
  CServerConfig::XMLE_Style *style = &dataSource->cfg->Style[styleIndex];

  //  INCLUDE other styles
  for (const auto &includeStyle: style->IncludeStyle) {
    int extraStyle = dataSource->srvParams->getServerStyleIndexByName(includeStyle.attr.name);
    if (extraStyle >= 0) {
      parseStyleInfo(styleConfig, dataSource, extraStyle, depth + 1);
    }
  }

  if (style->Scale.size() > 0) styleConfig->legendScale = atof(style->Scale[0].elementValue.c_str());
  if (style->Offset.size() > 0) styleConfig->legendOffset = atof(style->Offset[0].elementValue.c_str());
  if (style->Log.size() > 0) styleConfig->legendLog = atof(style->Log[0].elementValue.c_str());

  if (style->ContourIntervalL.size() > 0) {
    styleConfig->contourIntervalL = atof(style->ContourIntervalL[0].elementValue.c_str());
    styleConfig->shadeInterval = styleConfig->contourIntervalL;
  }
  if (style->ContourIntervalH.size() > 0) styleConfig->contourIntervalH = atof(style->ContourIntervalH[0].elementValue.c_str());

  if (style->ShadeInterval.size() > 0) styleConfig->shadeInterval = atof(style->ShadeInterval[0].elementValue.c_str());
  if (style->SmoothingFilter.size() > 0) styleConfig->smoothingFilter = atoi(style->SmoothingFilter[0].elementValue.c_str());

  if (style->ValueRange.size() > 0) {
    styleConfig->hasLegendValueRange = true;
    styleConfig->legendLowerRange = atof(style->ValueRange[0].attr.min.c_str());
    styleConfig->legendUpperRange = atof(style->ValueRange[0].attr.max.c_str());
  }

  if (style->Min.size() > 0) {
    styleConfig->minValue = atof(style->Min[0].elementValue.c_str());
    styleConfig->minMaxSet = true;
  }
  if (style->Max.size() > 0) {

    styleConfig->maxValue = atof(style->Max[0].elementValue.c_str());
    styleConfig->minMaxSet = true;
  }

  styleConfig->contourLines.insert(styleConfig->contourLines.end(), style->ContourLine.begin(), style->ContourLine.end());
  styleConfig->renderSettings.insert(styleConfig->renderSettings.end(), style->RenderSettings.begin(), style->RenderSettings.end());
  styleConfig->smoothingFilterVector.insert(styleConfig->smoothingFilterVector.end(), style->SmoothingFilter.begin(), style->SmoothingFilter.end());
  for (const auto &shadeInterval: style->ShadeInterval) {
    styleConfig->shadeIntervals.push_back(shadeInterval);
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
    styleConfig->legend = style->Legend[0];
  }
  if (style->LegendGraphic.size() > 0) {
    styleConfig->legendGraphic = style->LegendGraphic[0];
  }

  if (style->Legend.size() > 0) {
    if (style->Legend[0].attr.tickinterval.empty() == false) {
      styleConfig->legendTickInterval = atof(style->Legend[0].attr.tickinterval.c_str());
    }
    if (style->Legend[0].attr.tickround.empty() == false) {
      styleConfig->legendTickRound = atof(style->Legend[0].attr.tickround.c_str());
    }

    if (style->Legend[0].attr.fixedclasses == "true") {
      styleConfig->legendHasFixedMinMax = true;
    } else if (style->Legend[0].attr.fixedclasses == "false") {
      styleConfig->legendHasFixedMinMax = false;
    }
    styleConfig->legendName = style->Legend[0].elementValue;
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
    CDBWarning("No styles configured, taking style [%s]", dataSource->cfg->Style[this->styleIndex].attr.name.c_str());
  }

  parseStyleInfo(this, dataSource, this->styleIndex, 0);

  // Legend settings can always be overriden in the layer itself!
  CServerConfig::XMLE_Layer *layer = dataSource->cfgLayer;
  if (layer->Scale.size() > 0) this->legendScale = atof(layer->Scale[0].elementValue.c_str());
  if (layer->Offset.size() > 0) this->legendOffset = atof(layer->Offset[0].elementValue.c_str());
  if (layer->Log.size() > 0) this->legendLog = atof(layer->Log[0].elementValue.c_str());

  if (layer->ContourIntervalL.size() > 0) this->contourIntervalL = atof(layer->ContourIntervalL[0].elementValue.c_str());
  if (layer->ContourIntervalH.size() > 0) this->contourIntervalH = atof(layer->ContourIntervalH[0].elementValue.c_str());
  if (this->shadeInterval == 0.0f) this->shadeInterval = this->contourIntervalL;
  if (layer->ShadeInterval.size() > 0) this->shadeInterval = atof(layer->ShadeInterval[0].elementValue.c_str());
  if (layer->SmoothingFilter.size() > 0) this->smoothingFilter = atoi(layer->SmoothingFilter[0].elementValue.c_str());

  if (layer->ValueRange.size() > 0) {
    this->hasLegendValueRange = true;
    this->legendLowerRange = atof(layer->ValueRange[0].attr.min.c_str());
    this->legendUpperRange = atof(layer->ValueRange[0].attr.max.c_str());
  }

  if (layer->Min.size() > 0) {
    this->minValue = atof(layer->Min[0].elementValue.c_str());
    this->minMaxSet = true;
  }
  if (layer->Max.size() > 0) {
    this->maxValue = atof(layer->Max[0].elementValue.c_str());
    this->minMaxSet = true;
  }

  if (layer->ContourLine.size() > 0) {
    this->contourLines.assign(layer->ContourLine.begin(), layer->ContourLine.end());
  }
  if (layer->ShadeInterval.size() > 0) {
    this->shadeIntervals.clear();
    for (const auto &shadeInterval: layer->ShadeInterval) {
      this->shadeIntervals.push_back(shadeInterval);
    }
  }
  if (layer->FeatureInterval.size() > 0) {
    this->featureIntervals.assign(layer->FeatureInterval.begin(), layer->FeatureInterval.end());
  }

  if (layer->Legend.size() > 0) {
    if (layer->Legend[0].attr.tickinterval.empty() == false) {
      this->legendTickInterval = atof(layer->Legend[0].attr.tickinterval.c_str());
    }
    if (layer->Legend[0].attr.tickround.empty() == false) {
      this->legendTickRound = atof(layer->Legend[0].attr.tickround.c_str());
    }
    if (layer->Legend[0].attr.fixedclasses == "true") {
      this->legendHasFixedMinMax = true;
    } else if (layer->Legend[0].attr.fixedclasses == "false") {
      this->legendHasFixedMinMax = false;
    }
    this->legendName = layer->Legend[0].elementValue;
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
    if (CDATASOURCE_DEBUG) {
      CDBDebug("Found min and max in layer configuration");
    }

    stretchLegend(this->minValue, this->maxValue);
    dataSource->stretchMinMax = false;
  }

  if (this->legendIndex == -1) {
    if (dataSource->cfg->Legend.size() > 0) {
      this->legendIndex = 0;
      this->legendName = dataSource->cfg->Legend[0].attr.name;
    } else {
      CDBError("Server has no legends configured. Cannot continue.");
      return 1;
    }
  }

  // Legacy ContourIntervalL/H auto-generate a thin/bold contour-line tier each, as CImgWarpBilinear did.
  if ((this->renderMethod & (RM_SHADED | RM_CONTOUR)) != 0) {
    if (this->contourIntervalH > 0) {
      CServerConfig::XMLE_ContourLine bigInterval;
      bigInterval.attr.width = "1.4";
      bigInterval.attr.linecolor = "#000000";
      bigInterval.attr.textcolor = "#000000";
      bigInterval.attr.interval = CT::printf("%0.12f", this->contourIntervalH);
      this->contourLines.push_back(bigInterval);
    }
    if (this->contourIntervalL > 0) {
      CServerConfig::XMLE_ContourLine smallInterval;
      smallInterval.attr.width = "0.35";
      smallInterval.attr.linecolor = "#000000";
      smallInterval.attr.textcolor = "#000000";
      smallInterval.attr.interval = CT::printf("%0.12f", this->contourIntervalL);
      this->contourLines.push_back(smallInterval);
    }
  }

  return 0;
}

void CStyleConfiguration::stretchLegend(double min, double max) {
  if (this->legendLog != 0.0f) {
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

// Every index ever looked up through getCColorFromLegendPaletteIndex() is clamped to 0-239 by its
// caller, so indices 240-255 are never read - only this 0-239 range needs finishing: any index
// still pure black here was never set by a <palette> entry, so treat it as "no color"
// (transparent) rather than opaque black.
static void markUnsetEntriesTransparent(std::vector<CColor> &colors) {
  for (int j = 0; j < 240; j++) {
    CColor &c = colors[j];
    if (c.r == 0 && c.g == 0 && c.b == 0) {
      c.a = 0;
    }
  }
}

// Linearly interpolates RGBA across the index range [fromIndex, toIndex], used by both the
// colorRange and svg legend types (interval has no interpolation - see fillPaletteRange below).
// The red channel of each written entry is bumped off zero so the auto-transparency rule above
// doesn't mistake an intentionally-set black for an unset entry; alpha is clamped to zero (not
// narrowed/wrapped) if interpolation drives it negative, which is why it's carried as a plain int
// here rather than boxed into CColor's unsigned-char alpha before this clamp runs.
//
// Deliberately float, not double: existing <Legend> configs have been rendered and golden-tested
// against this exact (single-precision) rounding for years, and switching to double measurably
// shifted individual palette entries by 1 unit wherever a delta/span isn't exactly representable
// (e.g. -64/6) - a real, if tiny, rendering regression against every existing golden image.
static void interpolatePaletteRange(std::vector<CColor> &colors, int fromIndex, int fromRed, int fromGreen, int fromBlue, int fromAlpha, int toIndex, int toRed, int toGreen, int toBlue,
                                     int toAlpha) {
  float span = toIndex - fromIndex;
  if (span < 0.5f) span = 1;
  float redStep = (toRed - fromRed) / span;
  float greenStep = (toGreen - fromGreen) / span;
  float blueStep = (toBlue - fromBlue) / span;
  float alphaStep = (toAlpha - fromAlpha) / span;

  for (int i = fromIndex; i <= toIndex; i++) {
    if (i < 0 || i >= 240) continue;
    float step = i - fromIndex;
    int red = int(redStep * step) + fromRed;
    int green = int(greenStep * step) + fromGreen;
    int blue = int(blueStep * step) + fromBlue;
    int alpha = int(alphaStep * step) + fromAlpha;
    unsigned char r = (unsigned char)red;
    if (r == 0) r = 1;
    colors[i] = CColor(r, (unsigned char)green, (unsigned char)blue, (unsigned char)(alpha < 0 ? 0 : alpha));
  }
}

// Flat-fills the index range [fromIndex, toIndex) with a single solid color - used by the
// "interval" legend type, where each <palette> entry is its own flat step rather than a gradient.
static void fillPaletteRange(std::vector<CColor> &colors, int fromIndex, int toIndex, int red, int green, int blue, int alpha) {
  for (int i = fromIndex; i < toIndex; i++) {
    if (i < 0 || i >= 240) continue;
    unsigned char r = (unsigned char)red;
    if (r == 0) r = 1; // avoid the auto-transparency rule above mistaking this for an unset entry
    colors[i] = CColor(r, (unsigned char)green, (unsigned char)blue, (unsigned char)(alpha < 0 ? 0 : alpha));
  }
}

int CStyleConfiguration::createPalette(CServerConfig::XMLE_Legend *legend) {
  colors.assign(256, CColor(0, 0, 0, 255));
  if (legend == NULL) {
    markUnsetEntriesTransparent(colors);
    return 0;
  }
  if (legend->attr.type == "colorRange") {
    for (size_t j = 0; j < legend->palette.size(); j++) {
      CServerConfig::XMLE_palette *pbegin = &legend->palette[j];
      CServerConfig::XMLE_palette *pnext = &legend->palette[j];
      if (j < legend->palette.size() - 1) {
        pnext = &legend->palette[j + 1];
      }

      if (pbegin->attr.index > 255) pbegin->attr.index = 255;
      if (pbegin->attr.index < 0) pbegin->attr.index = 0;
      if (pnext->attr.index > 255) pnext->attr.index = 255;
      if (pnext->attr.index < 0) pnext->attr.index = 0;

      interpolatePaletteRange(colors, pbegin->attr.index, pbegin->attr.red, pbegin->attr.green, pbegin->attr.blue, pbegin->attr.alpha, pnext->attr.index, pnext->attr.red, pnext->attr.green,
                               pnext->attr.blue, pnext->attr.alpha);
    }
    markUnsetEntriesTransparent(colors);
    return 0;
  }
  if (legend->attr.type == "interval") {
    for (size_t j = 0; j < legend->palette.size(); j++) {
      auto &attr = legend->palette[j].attr;
      if (attr.index != -1) {
        int stopIndex = 240;
        if (j < legend->palette.size() - 1) stopIndex = legend->palette[j + 1].attr.index;
        fillPaletteRange(colors, attr.index, stopIndex, attr.red, attr.green, attr.blue, attr.alpha);
      } else {
        fillPaletteRange(colors, attr.min, attr.max + 1, attr.red, attr.green, attr.blue, attr.alpha);
      }
    }
    markUnsetEntriesTransparent(colors);
    return 0;
  }
  if (legend->attr.type == "svg") {
    if (legend->attr.file.empty()) {
      CDBError("Legend type file has no file attribute specified");
      return 1;
    }
    CDBDebug("Reading file %s", legend->attr.file.c_str());

    CXMLParserElement element;

    try {
      element.parseFile(legend->attr.file.c_str());
      auto stops = element.getThrows("svg")->getThrows("g")->getThrows("defs")->getThrows("linearGradient")->getList("stop");
      unsigned char prevRed = 0, prevGreen = 0, prevBlue = 0, prevAlpha = 0;
      int prevOffset = 0;
      for (size_t j = 0; j < stops.size(); j++) {
        int offset = (int)(std::stof(stops.at(j).getAttrValue("offset")) * 2.4);
        std::string color = stops.at(j).getAttrValue("stop-color").c_str() + 4;
        color.resize(color.length() - 1);
        auto colorParts = CT::split(color, ",");
        if (colorParts.size() != 3) {
          CDBError("Number of specified colors is unequal to three");
          return 1;
        }
        unsigned char red = atoi(colorParts[0].c_str());
        unsigned char green = atoi(colorParts[1].c_str());
        unsigned char blue = atoi(colorParts[2].c_str());
        unsigned char alpha = (unsigned char)(std::stof(stops.at(j).getAttrValue("stop-opacity")) * 255);
        if (offset > 255)
          offset = 255;
        else if (offset < 0)
          offset = 0;

        if (j > 0) {
          interpolatePaletteRange(colors, prevOffset, prevRed, prevGreen, prevBlue, prevAlpha, offset, red, green, blue, alpha);
        }
        prevRed = red;
        prevGreen = green;
        prevBlue = blue;
        prevAlpha = alpha;
        prevOffset = offset;
      }

    } catch (int e) {
      std::string message = CXMLParser::getErrorMessage(e);
      CDBError("%s\n", message.c_str());
      return 1;
    }
    markUnsetEntriesTransparent(colors);
    return 0;
  }
  return 1;
}

CColor CStyleConfiguration::getCColorFromLegendPaletteIndex(int colorIndex) const {
  // Transparent, not CColor()'s default opaque black: callers gate drawing on alpha>0, and an
  // out-of-range index - which includes every index when createPalette() was never called, since
  // colors is then empty - must mean "nothing to draw" rather than "draw opaque black".
  if (colorIndex >= 0 && colorIndex < (int)colors.size()) {
    return colors[colorIndex];
  }
  return CColor(0, 0, 0, 0);
}
