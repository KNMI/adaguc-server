#include "numericutils.h"
#include <cmath>

int fieldWidth(std::vector<CT::string> column) {
  int intWidth = maxIntWidth(column);
  int decWidth = maxDecimalWidth(column);
  int signWidth = negWidth(column);

  if (decWidth > 0) {
    return intWidth + signWidth + 1 + decWidth;
  } else {
    return intWidth + signWidth;
  }
}

int maxIntWidth(std::vector<CT::string> column) {
  // Note: Consider if there are negative numbers
  int width = 0;

  for (const CT::string &item : column) {
    int intVal = atoi(item.c_str());
    int numberOfDigits = intVal ? static_cast<int>(log10(abs(intVal))) + 1 : 1;
    if (numberOfDigits > width) {
      width = numberOfDigits;
    }
  }
  return width;
}

int negWidth(std::vector<CT::string> column) {
  int isNeg = 0;

  for (const CT::string &item : column) {
    if (atoi(item.c_str()) < 0) {
      isNeg = 1;
    }
  }
  return isNeg;
}

int maxDecimalWidth(std::vector<CT::string> column) {
  //
  int width = 0;
  for (const CT::string &item : column) {
    CT::string curr = item;
    int index = curr.lastIndexOf(".");
    int currWidth = curr.length() - index;

    if (currWidth > width) {
      width = currWidth;
    };
  }
  return width;
}

int fieldWidthAsPixels(std::vector<CT::string> column, int dotWidth, int numbericGlyphWidth) {
  int intWidth = maxIntWidth(column);
  int decWidth = maxDecimalWidth(column);

  if (decWidth > 0) {
    return intWidth * numbericGlyphWidth + 1 + decWidth;
  } else {
    return intWidth * numbericGlyphWidth;
  }
}
