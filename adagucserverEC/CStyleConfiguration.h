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

typedef unsigned int RenderMethod;
CT::string getRenderMethodAsString(RenderMethod renderMethod);
RenderMethod getRenderMethodFromString(const char *_renderMethodString);
class CStyleConfiguration {
private:
  void parseStyleInfo(CDataSource *dataSource, int styleIndex, int depth = 0);
  void _reset();

public:
  bool minMaxSet;
  bool hasLegendValueRange;
  bool hasError;
  bool legendHasFixedMinMax; // True to fix the classes in the legend, False to determine automatically which values occur.
  int smoothingFilter;
  int legendIndex;
  int styleIndex;
  float shadeInterval;
  float contourIntervalL;
  float contourIntervalH;
  float legendScale;
  float legendOffset;
  float legendLog;
  float legendLowerRange;
  float legendUpperRange; // Values in which values are visible (ValueRange)
  double legendTickInterval;
  double legendTickRound;
  double minValue;
  double maxValue;
  RenderMethod renderMethod;
  CT::string legendName;
  CT::string styleCompositionName;
  CT::string styleTitle;
  CT::string styleAbstract;

  std::vector<CServerConfig::XMLE_ContourLine *> contourLines;
  std::vector<CServerConfig::XMLE_RenderSettings *> renderSettings;
  std::vector<CServerConfig::XMLE_ShadeInterval *> shadeIntervals;
  std::vector<CServerConfig::XMLE_SymbolInterval *> *symbolIntervals;
  std::vector<CServerConfig::XMLE_FeatureInterval *> *featureIntervals;

  CServerConfig::XMLE_Style *styleConfig;

  CStyleConfiguration();

  /**
   * Outputs styleConfiguration as string
   */
  CT::string dump();

  /**
   * Fills in the styleConfig object based on datasource,stylename, legendname and rendermethod
   */
  int makeStyleConfig(CDataSource *dataSource);
};
#endif
