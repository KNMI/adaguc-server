#ifndef DRAW_CONTOURDEFINITION_H
#define DRAW_CONTOURDEFINITION_H

class ContourDefinition {
public:
  ContourDefinition() {
    lineWidth = 1;
    linecolor.r = 0;
    linecolor.g = 0;
    linecolor.b = 0;
    linecolor.a = 255;
    textcolor.r = 0;
    textcolor.g = 0;
    textcolor.b = 0;
    textcolor.a = 0;
    textstrokecolor.r = 0;
    textstrokecolor.g = 0;
    textstrokecolor.b = 0;
    textstrokecolor.a = 0;
    continuousInterval = 0;
    textFormat = "%f";
    fontSize = 0;        // Zero means take default.
    textStrokeWidth = 0; // Zero means take default.
  }

  std::vector<float> definedIntervals;
  float continuousInterval;
  float lineWidth;
  float fontSize;
  float textStrokeWidth;
  CT::string textFormat;
  CColor linecolor;
  CColor textcolor;
  CColor textstrokecolor;
  CT::string dashing;

  ContourDefinition(float lineWidth, CColor linecolor, CColor textcolor, CColor textstrokecolor, const char *_definedIntervals, const char *_textFormat, float fontSize, float textStrokeWidth,
                    CT::string dashing) {
    this->lineWidth = lineWidth;
    this->linecolor = linecolor;
    this->textcolor = textcolor;
    this->textstrokecolor = textstrokecolor;
    this->fontSize = fontSize;
    this->textStrokeWidth = textStrokeWidth;
    this->dashing = dashing;
    this->continuousInterval = 0;

    if (_definedIntervals != NULL) {
      CT::string defIntervalString = _definedIntervals;
      CT::string *defIntervalList = defIntervalString.splitToArray(",");
      for (size_t j = 0; j < defIntervalList->count; j++) {
        definedIntervals.push_back(defIntervalList[j].toFloat());
      }
      delete[] defIntervalList;
    }

    if (_textFormat != NULL) {
      if (strlen(_textFormat) > 1) {
        this->textFormat = _textFormat;
        return;
      }
    }
  }

  ContourDefinition(float lineWidth, CColor linecolor, CColor textcolor, CColor textstrokecolor, float continuousInterval, const char *_textFormat, float fontSize, float textStrokeWidth,
                    CT::string dashing) {

    this->lineWidth = lineWidth;
    this->linecolor = linecolor;
    this->textcolor = textcolor;
    this->textstrokecolor = textstrokecolor;
    this->fontSize = fontSize;
    this->textStrokeWidth = textStrokeWidth;
    this->continuousInterval = continuousInterval;
    this->dashing = dashing;

    if (_textFormat != NULL) {
      this->textFormat = _textFormat;
      return;
    }

    float fracPart = continuousInterval - int(continuousInterval);
    int textRounding = -int(log10(fracPart) - 0.9999999f);
    if (textRounding <= 0) textFormat = "%2.0f";
    if (textRounding == 1) textFormat = "%2.1f";
    if (textRounding == 2) textFormat = "%2.2f";
    if (textRounding == 3) textFormat = "%2.3f";
    if (textRounding == 4) textFormat = "%2.4f";
    if (textRounding == 5) textFormat = "%2.5f";
    if (textRounding >= 6) textFormat = "%f";
  }
};
#endif