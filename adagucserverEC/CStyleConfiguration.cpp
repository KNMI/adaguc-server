#include "CStyleConfiguration.h"
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
