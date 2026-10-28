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

class CDataSource;
class CDrawImage;

typedef unsigned int RenderMethod;
std::string getRenderMethodAsString(RenderMethod renderMethod);
RenderMethod getRenderMethodFromString(std::string renderMethodString);

// Whether this style uses a keyword of the removed CImgWarpBilinear renderer (bilinear/shaded/contour/
// vector/barb/thin), unless "generic" is also explicitly listed - an explicit "generic" always wins.
inline bool isLegacyRenderMethod(RenderMethod renderMethod) {
  return (renderMethod & (RM_BILINEAR | RM_SHADED | RM_CONTOUR | RM_VECTOR | RM_BARB | RM_THIN)) != 0 && (renderMethod & RM_GENERIC) == 0;
}
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
  std::string legendName;
  std::string styleCompositionName;
  std::string styleName;
  std::string styleTitle;
  std::string styleAbstract;
  std::vector<CServerConfig::XMLE_ContourLine> contourLines;
  std::vector<CServerConfig::XMLE_RenderSettings> renderSettings;
  std::vector<CServerConfig::XMLE_SmoothingFilter> smoothingFilterVector;
  std::vector<CServerConfig::XMLE_ShadeInterval> shadeIntervals;
  std::vector<CServerConfig::XMLE_SymbolInterval> symbolIntervals;
  std::vector<CServerConfig::XMLE_FeatureInterval> featureIntervals;
  std::vector<CServerConfig::XMLE_Point> pointIntervals;
  std::vector<CServerConfig::XMLE_Vector> vectorIntervals;
  std::vector<CServerConfig::XMLE_DataPostProc> dataPostProcessors;
  std::vector<CServerConfig::XMLE_Stippling> stipplingList;
  std::vector<CServerConfig::XMLE_FilterPoints> filterPointList;
  std::vector<CServerConfig::XMLE_Thinning> thinningList;
  CServerConfig::XMLE_Legend legend;
  CServerConfig::XMLE_LegendGraphic legendGraphic;

  // Empty until createPalette() is called; an index lookup then resolves transparent, not opaque black.
  std::vector<CColor> colors;

  // Parallel to colors: true if the <palette> entry had a negative alpha, meaning overwrite the
  // pixel (CDrawImage::setPixelTrueColorOverWrite) instead of blending; magnitude -(alpha+1) is in colors[index].a.
  std::vector<bool> overwrite;

  /**
   * Outputs styleConfiguration as string
   */
  std::string dump();

  /**
   * Completes in the styleConfig object based on datasource which contains the stylename, legendname and rendermethod
   */
  int makeStyleConfig(CDataSource *dataSource);

  /**
   * Sets the legend scale and offset based on min/max
   */
  void stretchLegend(double min, double max);

  // Builds the index-color palette (colorRange/interval/svg); transparent everywhere if legend is nullptr.
  int createPalette(CServerConfig::XMLE_Legend *legend);

  // Looks up colors[index]; transparent for an out-of-range index.
  CColor getCColorFromLegendPaletteIndex(int index) const;

  // Whether index should overwrite rather than blend - see `overwrite` above.
  bool isLegendPaletteIndexOverwrite(int index) const;

  // Draws via overwrite or blend as appropriate; overwrite still draws at alpha 0 (an erase).
  void drawPixelFromPaletteIndex(CDrawImage *drawImage, int x, int y, int colorIndex) const;
};
#endif
