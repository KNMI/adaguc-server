#ifndef DRAW_SHADEDEFINITION_H
#define DRAW_SHADEDEFINITION_H

class ShadeDefinition {
public:
  float min, max;
  bool foundColor;
  bool hasBGColor;
  CColor fillColor;
  CColor bgColor;
  ShadeDefinition(float min, float max, CColor fillColor, bool foundColor, CColor bgColor, bool hasBGColor) {
    this->min = min;
    this->max = max;
    this->fillColor = fillColor;
    this->foundColor = foundColor;
    this->bgColor = bgColor;
    this->hasBGColor = hasBGColor;
  }
};

#endif