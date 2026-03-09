#ifndef CDRAWFUNCTION
#define CDRAWFUNCTION

#include <cmath>
#include <float.h>
#include <pthread.h>
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CGenericDataWarper.h"
#include "CStyleConfiguration.h"
#include "GenericDataWarper/GDWDrawFunctionSettings.h"
struct MemoizationForDeterminePixelColorFromValue {
  double value;
  GDWDrawFunctionSettings *settings;
  CColor color;
};

template <class T> void setPixelInDrawImage(int x, int y, T val, GDWDrawFunctionSettings *settings);
template <class T> CColor determinePixelColorFromValue(T val, GDWDrawFunctionSettings *settings);
template <class T> CColor memoizedDeterminePixelColorFromValue(T val, GDWDrawFunctionSettings *settings, MemoizationForDeterminePixelColorFromValue &memo);

#endif