#include "CDebugger.h"
#include "CGenericDataWarperTools.h"
#include <assert.h>

DEF_ERRORMAIN()

int main() {
  double dfSourceW = 1000;
  double dfSourceExtW = 360;
  double dfSourceOrigX = 0;
  double dfDestOrigX = 0;
  double dfDestExtW = 360;
  double dfDestW = 1000;
  for (double input = -4000; input < 4000; input += 1) {
    double result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
    if (result != input) {
      CDBError("linearTransform [%f] != [%f] wrong result", input, result);
      throw __LINE__;
    }
  }

  double input, result, expected;

  input = 0.4;
  expected = 0.0;
  result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
  if (result != expected) {
    CDBError("linearTransform [%f] != [%f] wrong result", result, expected);
    throw __LINE__;
  }
  input = 0.5;
  expected = 1.0;
  result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
  if (result != expected) {
    CDBError("linearTransform [%f] != [%f] wrong result", result, expected);
    throw __LINE__;
  }
  input = -0.5;
  expected = 0.0;
  result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
  if (result != expected) {
    CDBError("linearTransform [%f] != [%f] wrong result", result, expected);
    throw __LINE__;
  }
  input = -0.6;
  expected = -1.0;
  result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
  if (result != expected) {
    CDBError("linearTransform [%f] != [%f] wrong result", result, expected);
    throw __LINE__;
  }

  // CDBDebug("OK %f", linearTransform(5.0, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW));
  return 0;
}
