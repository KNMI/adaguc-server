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

class CDataSource;

class CStyleConfiguration {
public:
  typedef unsigned int RenderMethod;
  static RenderMethod getRenderMethodFromString(CT::string *renderMethodString);
  static void getRenderMethodAsString(CT::string *renderMethodString, RenderMethod renderMethod);
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

  CStyleConfiguration();
  /**
   * Resets styleConfiguration back to default
   */
  void reset();

  /**
   * Outputs styleConfiguration as string
   */
  CT::string c_str();

  /**
   * Fills in the styleConfig object based on datasource,stylename, legendname and rendermethod
   */
  int makeStyleConfig(CDataSource *dataSource);
};
#endif
