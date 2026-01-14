#include <cmath>
static inline double roundedLinearTransform(double dfx, double &dfSourceW, double &dfSourceExtW, double &dfSourceOrigX, double &dfDestOrigX, double &dfDestExtW, double &dfDestW) {
  return floor((((((dfx) / (dfSourceW)) * dfSourceExtW + dfSourceOrigX) - dfDestOrigX) / dfDestExtW) * dfDestW + 0.5);
}