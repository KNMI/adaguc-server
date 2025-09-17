#ifndef numericutils_H
#define numericutils_H

#include <vector>
#include "CTypes.h"
#include <CServerConfig_CPPXSD.h>

// Field width takes into consideration:
// - If there is a negative sign
// - If there are decimals
// Field width = (max int part width) (for integers)
// Field width = (max int part width) + 1 + (max decimal width) (for decimals)
int fieldWidth(std::vector<CT::string> column);

int maxIntWidth(std::vector<CT::string> column);

int hasNeg(std::vector<CT::string> column);

int maxDecimalWidth(std::vector<CT::string> column);

int fieldWidthAsPixels(std::vector<CT::string> column, int dashWidth, int dotWidth, int numbericGlyphWidth);

std::vector<CT::string> extractColumn(size_t drawIntervals, int minInterval, std::vector<CServerConfig::XMLE_ShadeInterval *> &shadeIntervals, bool isMin);

#endif