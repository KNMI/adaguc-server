#include "numericutils.h"
#include <cmath>

int fieldWidth(std::vector<CT::string> column) {
  int intWidth = maxIntWidth(column);
  int decWidth = maxDecimalWidth(column);
  int signWidth = hasNeg(column);

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

int hasNeg(std::vector<CT::string> column) {
  int isNeg = 0;

  for (const CT::string &item : column) {
    if (atoi(item.c_str()) < 0) {
      isNeg = 1;
    }
  }
  return isNeg;
}

int maxDecimalWidth(std::vector<CT::string> column) { //
  int width = 0;
  for (int i = 1; i < column.size(); i++) { // CT::string &item : column) {
    // CT::string curr(item.c_str());
    int index = column[i].indexOf(".");

    int currWidth = column[i].length() - index;

    if (currWidth > width) {
      CDBDebug("Decimal width of %d reached with number %s", currWidth, column[i].c_str());
      CDBDebug("Total length = %d,  index of dot = %d", currWidth, index);
      if (index < 0) {
        break; // dot not found
      }
      width = currWidth;
    };
  }
  return width - 1;
}

int fieldWidthAsPixels(std::vector<CT::string> column, int dashWidth, int dotWidth, int numbericGlyphWidth) {
  int intWidth = maxIntWidth(column);
  int decWidth = maxDecimalWidth(column);
  int hasDash = hasNeg(column);

  if (decWidth > 0) {
    return intWidth * numbericGlyphWidth + hasDash * dashWidth + decWidth;
  } else {
    return intWidth * numbericGlyphWidth + hasDash * dashWidth;
  }
}

std::vector<CT::string> extractColumn(size_t drawIntervals, int minInterval, std::vector<CServerConfig::XMLE_ShadeInterval *> *shadeIntervals, bool isMin) {
  // We calculate the min column
  // Convert the min into an array of CT::string
  std::vector<CT::string> column;
  for (size_t j = 0; j < drawIntervals; j++) {
    size_t realj = minInterval + j;
    CServerConfig::XMLE_ShadeInterval *s = (*shadeIntervals)[realj];
    if (!s->attr.min.empty() && !s->attr.max.empty()) {
      if (isMin) {
        if ((int)(std::abs(parseFloat(s->attr.min.c_str()))) % 5 != 0) {
          continue;
        }
        column.push_back(s->attr.min.c_str());
      } else {
        if ((int)(std::abs(parseFloat(s->attr.max.c_str()))) % 5 != 0) {
          continue;
        }
        column.push_back(s->attr.max.c_str());
      }
    }
  }
  return column;
}