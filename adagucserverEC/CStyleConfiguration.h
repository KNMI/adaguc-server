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
#define RM_POLYLINE 2048
#define RM_POINT_LINEARINTERPOLATION 4096
#define RM_HILLSHADED 8192
#define RM_GENERIC 16384
#define RM_POLYGON 32768

#include "CServerConfig_CPPXSD.h"
#include "CXMLParser.h"
#include "CDebugger.h"

class CDataSource;

typedef unsigned int RenderMethod;
CT::string getRenderMethodAsString(RenderMethod renderMethod);
RenderMethod getRenderMethodFromString(CT::string renderMethodString);
struct CStyleConfiguration {
  bool minMaxSet = false;
  bool hasLegendValueRange = false;
  bool hasError = false;
  bool legendHasFixedMinMax = true; // True to fix the classes in the legend, False to determine automatically which values occur.
  int smoothingFilter = 0;
  int legendIndex = -1;
  int styleIndex = -1;
  float shadeInterval = 0;
  float contourIntervalL = 0;
  float contourIntervalH = 0;
  float legendScale = 0;
  float legendOffset = 0;
  float legendLog = 0.0f;
  float legendLowerRange = 0;
  float legendUpperRange = 0; // Values in which values are visible (ValueRange)
  double legendTickInterval = 0;
  double legendTickRound = 0.0;
  double minValue = 0;
  double maxValue = 0;
  RenderMethod renderMethod = RM_GENERIC;
  CT::string legendName;
  CT::string styleCompositionName;
  CT::string styleName;
  CT::string styleTitle;
  CT::string styleAbstract;
  std::vector<CServerConfig::XMLE_ContourLine *> contourLines;
  std::vector<CServerConfig::XMLE_RenderSettings *> renderSettings;
  std::vector<CServerConfig::XMLE_SmoothingFilter *> smoothingFilterVector;
  std::vector<CServerConfig::XMLE_ShadeInterval> shadeIntervals;
  std::vector<CServerConfig::XMLE_SymbolInterval *> symbolIntervals;
  std::vector<CServerConfig::XMLE_FeatureInterval *> featureIntervals;
  std::vector<CServerConfig::XMLE_Point *> pointIntervals;
  std::vector<CServerConfig::XMLE_Vector *> vectorIntervals;
  std::vector<CServerConfig::XMLE_DataPostProc *> dataPostProcessors;
  std::vector<CServerConfig::XMLE_Stippling *> stipplingList;
  std::vector<CServerConfig::XMLE_FilterPoints *> filterPointList;
  std::vector<CServerConfig::XMLE_Thinning *> thinningList;
  CServerConfig::XMLE_Legend legend;
  CServerConfig::XMLE_LegendGraphic legendGraphic;

  /**
   * Outputs styleConfiguration as string
   */
  CT::string dump();

  /**
   * Completes in the styleConfig object based on datasource which contains the stylename, legendname and rendermethod
   */
  int makeStyleConfig(CDataSource *dataSource);
};
#endif
