#ifndef numericutils_H
#define numericutils_H

#include <vector>
#include "CTString.h"
#include <CServerConfig_CPPXSD.h>

// Field width takes into consideration:
// - If there is a negative sign
// - If there are decimals
// Field width = (max int part width) (for integers)
// Field width = (max int part width) + 1 + (max decimal width) (for decimals)
int fieldWidth(std::vector<std::string> column);

int maxIntWidth(std::vector<std::string> column);

int hasNeg(std::vector<std::string> column);

int maxDecimalWidth(std::vector<std::string> column);

int fieldWidthAsPixels(std::vector<std::string> column, int dashWidth, int dotWidth, int numbericGlyphWidth);

std::vector<std::string> extractColumn(size_t drawIntervals, int minInterval, const std::vector<CServerConfig::XMLE_ShadeInterval>& shadeIntervals, bool isMin);

#endif