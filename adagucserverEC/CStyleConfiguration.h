#ifndef CSTYLECONFIGURATION_H
#define CSTYLECONFIGURATION_H

// Possible rendermethods
#define RM_UNDEFINED 0
#define RM_NEAREST 1
#define RM_BILINEAR 2
#define RM_SHADED 4
#define RM_CONTOUR 8
#define RM_POINT 16
#define RM_VECTOR 32
#define RM_BARB 64
#define RM_THIN 128
#define RM_RGBA 256
#define RM_STIPPLING 512
#define RM_AVG_RGBA 1024
#define RM_POLYLINE 2048
#define RM_POINT_LINEARINTERPOLATION 4096
#define RM_HILLSHADED 8192
#define RM_GENERIC 16384

#include "CServerConfig_CPPXSD.h"
#include "CXMLParser.h"
#include "CDebugger.h"
class CStyleConfiguration {
public:
  typedef unsigned int RenderMethod;

  static RenderMethod getRenderMethodFromString(CT::string *renderMethodString);
  static void getRenderMethodAsString(CT::string *renderMethodString, RenderMethod renderMethod);
  void reset() {
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
  CStyleConfiguration() { reset(); }
  float shadeInterval, contourIntervalL, contourIntervalH;
  float legendScale, legendOffset, legendLog;
  float legendLowerRange, legendUpperRange; // Values in which values are visible (ValueRange)
  int smoothingFilter;
  bool hasLegendValueRange;
  bool hasError;
  bool legendHasFixedMinMax; // True to fix the classes in the legend, False to determine automatically which values occur.
  double legendTickInterval;
  double legendTickRound;
  bool minMaxSet;
  int legendIndex;
  int styleIndex;
  RenderMethod renderMethod;
  std::vector<CServerConfig::XMLE_ContourLine *> *contourLines;
  std::vector<CServerConfig::XMLE_ShadeInterval *> *shadeIntervals;
  std::vector<CServerConfig::XMLE_SymbolInterval *> *symbolIntervals;
  std::vector<CServerConfig::XMLE_FeatureInterval *> *featureIntervals;
  CServerConfig::XMLE_Style *styleConfig;
  CT::string styleCompositionName;
  CT::string styleTitle;
  CT::string styleAbstract;

  CT::string c_str() {
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
};
#endif
