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

template <class T> void setPixelInDrawImage(int x, int y, T val, GDWDrawFunctionSettings *settings);
#endif