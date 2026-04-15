#ifndef POINTSTYLES_H
#define POINTSTYLES_H

#include "CColor.h"

struct TextStyle {
  CColor textColor;
  double fontSize;

  CColor textOutlineColor;
  double textOutlineWidth;
};

struct LineStyle {
  CColor lineColor;
  double lineWidth;

  CColor lineOutlineColor;
  double lineOutlineWidth;
};

#endif