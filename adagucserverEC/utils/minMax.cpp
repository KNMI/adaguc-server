
#include "minMax.h"
#include <cstddef>
#include <CCDFObject.h>
#include "CCDFVariable.h"

MinMax getMinMax(double *data, bool hasFillValue, double fillValue, size_t numElements) {
  MinMax minMax;
  bool firstSet = false;
  for (size_t j = 0; j < numElements; j++) {
    double v = data[j];
    if (v == v) {
      if ((v != fillValue) || (!hasFillValue)) {
        if (firstSet == false) {
          firstSet = true;
          minMax.min = v;
          minMax.max = v;
          minMax.isSet = true;
        }
        if (v < minMax.min) minMax.min = v;
        if (v > minMax.max) minMax.max = v;
      }
    }
  }
  if (minMax.isSet == false) {
    throw __LINE__;
  }
  return minMax;
}

MinMax getMinMax(float *data, bool hasFillValue, double fillValue, size_t numElements) {
  MinMax minMax;
  bool firstSet = false;
  for (size_t j = 0; j < numElements; j++) {
    float v = data[j];
    if (v == v) {
      if ((v != fillValue) || (!hasFillValue)) {
        if (firstSet == false) {
          firstSet = true;
          minMax.min = v;
          minMax.max = v;
          minMax.isSet = true;
        }
        if (v < minMax.min) minMax.min = v;
        if (v > minMax.max) minMax.max = v;
      }
    }
  }
  if (minMax.isSet == false) {
    throw __LINE__ + 100;
  }
  return minMax;
}

void Statistics::setMinMax(MinMax minMax) {
  this->min = minMax.min;
  this->max = minMax.max;
}

int Statistics::calculate(CDataSource *dataSource) {
  // Get Min and Max
  // CDBDebug("calculate stat ");
  DataObject *dataObject = dataSource->getFirstAvailableDataObject();
  if (dataObject->cdfVariable->data != NULL) {
    size_t size = dataObject->cdfVariable->getSize(); // dataSource->dWidth*dataSource->dHeight;

    if (dataObject->cdfVariable->getType() == CDF_CHAR) calcMinMax<char>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_BYTE) calcMinMax<char>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_UBYTE) calcMinMax<unsigned char>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_SHORT) calcMinMax<short>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_USHORT) calcMinMax<unsigned short>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_INT) calcMinMax<int>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_UINT) calcMinMax<unsigned int>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_FLOAT) calcMinMax<float>(size, dataSource->getDataObjectsVector());
    if (dataObject->cdfVariable->getType() == CDF_DOUBLE) calcMinMax<double>(size, dataSource->getDataObjectsVector());
  }
  return 0;
}

template <class T> void Statistics::calcMinMax(size_t size, std::vector<DataObject *> *dataObject) {
#ifdef MEASURETIME
  StopWatch_Stop("Start min/max calculation");
#endif
  if (dataObject->size() == 1) {
    T *data = (T *)(*dataObject)[0]->cdfVariable->data;
    CDFType type = (*dataObject)[0]->cdfVariable->getType();
    double dfNodataValue = (*dataObject)[0]->dfNodataValue;
    bool hasNodataValue = (*dataObject)[0]->hasNodataValue;
    calculate(size, data, type, dfNodataValue, hasNodataValue);
  }

  // Wind vector min max calculation
  if (dataObject->size() == 2) {
    T *dataU = (T *)(*dataObject)[0]->cdfVariable->data;
    T *dataV = (T *)(*dataObject)[1]->cdfVariable->data;
    T _min = (T)0.0f, _max = (T)0.0f;
    int firstDone = 0;
    T s = 0;
    for (size_t p = 0; p < size; p++) {

      T u = dataU[p];
      T v = dataV[p];

      if (((((T)v) != (T)(*dataObject)[0]->dfNodataValue || (!(*dataObject)[0]->hasNodataValue)) && v == v) &&
          ((((T)u) != (T)(*dataObject)[0]->dfNodataValue || (!(*dataObject)[0]->hasNodataValue)) && u == u)) {
        s = (T)hypot(u, v);
        if (firstDone == 0) {
          _min = s;
          _max = s;
          firstDone = 1;
        } else {

          if (s < _min) _min = s;
          if (s > _max) _max = s;
        }
      }
    }
    min = (double)_min;
    max = (double)_max;
  }
#ifdef MEASURETIME
  StopWatch_Stop("Finished min/max calculation");
#endif
}

MinMax getMinMax(CDF::Variable *var) {
  MinMax minMax;
  if (var != NULL) {
    if (var->getType() == CDF_FLOAT) {

      float *data = (float *)var->data;

      float scaleFactor = 1, addOffset = 0, fillValue = 0;
      bool hasFillValue = false;

      try {
        var->getAttributeThrows("scale_factor")->getData(&scaleFactor, 1);
      } catch (int e) {
      }
      try {
        var->getAttributeThrows("add_offset")->getData(&addOffset, 1);
      } catch (int e) {
      }
      try {
        var->getAttributeThrows("_FillValue")->getData(&fillValue, 1);
        hasFillValue = true;
      } catch (int e) {
      }

      size_t lsize = var->getSize();

      // Apply scale and offset
      if (scaleFactor != 1 || addOffset != 0) {
        for (size_t j = 0; j < lsize; j++) {
          data[j] = data[j] * scaleFactor + addOffset;
        }
        fillValue = fillValue * scaleFactor + addOffset;
      }

      minMax = getMinMax(data, hasFillValue, fillValue, lsize);
    } else if (var->getType() == CDF_DOUBLE) {

      double *data = (double *)var->data;

      double scaleFactor = 1, addOffset = 0, fillValue = 0;
      bool hasFillValue = false;

      try {
        var->getAttributeThrows("scale_factor")->getData(&scaleFactor, 1);
      } catch (int e) {
      }
      try {
        var->getAttributeThrows("add_offset")->getData(&addOffset, 1);
      } catch (int e) {
      }
      try {
        var->getAttributeThrows("_FillValue")->getData(&fillValue, 1);
        hasFillValue = true;
      } catch (int e) {
      }

      size_t lsize = var->getSize();

      // Apply scale and offset
      if (scaleFactor != 1 || addOffset != 0) {
        for (size_t j = 0; j < lsize; j++) {
          data[j] = data[j] * scaleFactor + addOffset;
        }
        fillValue = fillValue * scaleFactor + addOffset;
      }

      minMax = getMinMax(data, hasFillValue, fillValue, lsize);
    }

  } else {
    throw __LINE__;
  }
  return minMax;
}