#include <cmath>
#include "CGenericDataWarperTools.h"

double roundedLinearTransform(double dfx, double dfSourceW, double dfSourceExtW, double dfSourceOrigX, double dfDestOrigX, double dfDestExtW, double dfDestW) {
  return std::floor((dfx * dfSourceExtW / dfSourceW + dfSourceOrigX - dfDestOrigX) * dfDestW / dfDestExtW + 0.5);
}