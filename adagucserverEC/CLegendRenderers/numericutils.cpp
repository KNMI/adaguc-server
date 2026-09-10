#include "numericutils.h"
#include <cmath>

int fieldWidth(const std::vector<std::string> &column) {
  int intWidth = maxIntWidth(column);
  int decWidth = maxDecimalWidth(column);
  int signWidth = hasNeg(column);

  if (decWidth > 0) {
    return intWidth + signWidth + 1 + decWidth;
  } else {
    return intWidth + signWidth;
  }
}

int maxIntWidth(const std::vector<std::string> &column) {
  // Note: Consider if there are negative numbers
  int width = 0;

  for (const std::string &item: column) {
    int intVal = atoi(item.c_str());
    int numberOfDigits = intVal ? static_cast<int>(log10(abs(intVal))) + 1 : 1;
    if (numberOfDigits > width) {
      width = numberOfDigits;
    }
  }
  return width;
}

int hasNeg(const std::vector<std::string> &column) {
  int isNeg = 0;

  for (const std::string &item: column) {
    if (atoi(item.c_str()) < 0) {
      isNeg = 1;
    }
  }
  return isNeg;
}

int maxDecimalWidth(const std::vector<std::string> &column) {
  int maxDecimals = 0;

  for (const std::string &item: column) {
    int dotIndex = CT::indexOf(item, ".");
    if (dotIndex < 0) {
      continue; // dot not found
    }

    int decimals = item.length() - dotIndex - 1;
    if (decimals > maxDecimals) {
      maxDecimals = decimals;
    }
  }

  return maxDecimals;
}

int fieldWidthAsPixels(const std::vector<std::string> &column, int dashWidth, int, int numericGlyphWidth) {
  int intWidth = maxIntWidth(column);
  int decWidth = maxDecimalWidth(column);
  int hasDash = hasNeg(column);

  if (decWidth > 0) {
    return intWidth * numericGlyphWidth + hasDash * dashWidth + decWidth;
  } else {
    return intWidth * numericGlyphWidth + hasDash * dashWidth;
  }
}

std::vector<std::string> extractColumn(size_t drawIntervals, int minInterval, const std::vector<CServerConfig::XMLE_ShadeInterval> &shadeIntervals, bool isMin) {
  // We calculate the min column
  // Convert the min into an array of std::string
  std::vector<std::string> column;
  for (size_t j = 0; j < drawIntervals; j++) {
    size_t realj = minInterval + j;
    const CServerConfig::XMLE_ShadeInterval &s = (shadeIntervals)[realj];
    if (!std::isnan(s.attr.min) && !std::isnan(s.attr.max)) {
      if (isMin) {
        column.push_back(CT::printf("%g", s.attr.min));
      } else {
        column.push_back(CT::printf("%g", s.attr.max));
      }
    }
  }
  return column;
}
