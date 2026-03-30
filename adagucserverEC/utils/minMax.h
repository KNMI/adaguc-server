
#include <cstddef>
#include <vector>
#include <cmath>
#include <CDataSource.h>
#include <CCDFVariable.h>
#include "CDataSource.h"

#ifndef MINMAX_H
#define MINMAX_H

// TODO THIS NEEDS refactoring, e.g. combine MinMax and Statistics into one and dont use pointers.
// The calculate routine should be a standalone function
class DataObject;

struct MinMax {
  bool isSet = false;
  double min, max;
};

/**
 * Returns minmax values for a float data array
 * throws integer if no min max are found
 * @param data The data array in float format
 * @param hasFillValue Is there a nodata value
 * @param fillValue the Nodata value
 * @param numElements The length of the data array
 * @return minmax object
 */
MinMax getMinMax(float *data, bool hasFillValue, double fillValue, size_t numElements);

/**
 * Returns minmax values for a double data array
 * throws integer if no min max are found
 * @param data The data array in double format
 * @param hasFillValue Is there a nodata value
 * @param fillValue the Nodata value
 * @param numElements The getStdDevlength of the data array
 * @return minmax object
 */
MinMax getMinMax(double *data, bool hasFillValue, double fillValue, size_t numElements);

/**
 * Returns minmax values for a variable
 * @param var The variable to retrieve the min max for.
 * @return minmax object
 */
MinMax getMinMax(CDF::Variable *var);

class Statistics {
public:
  void calculate(size_t size, void *data, CDFType type, double dfNodataValue, bool hasNodataValue) {
    if (type == CDF_CHAR) calculate<char>(size, (char *)data, type, dfNodataValue, hasNodataValue);
    if (type == CDF_BYTE) calculate<char>(size, (char *)data, type, dfNodataValue, hasNodataValue);
    if (type == CDF_UBYTE) calculate<unsigned char>(size, (unsigned char *)data, type, dfNodataValue, hasNodataValue);
    if (type == CDF_SHORT) calculate<short>(size, (short *)data, type, dfNodataValue, hasNodataValue);
    if (type == CDF_USHORT) calculate<unsigned short>(size, (unsigned short *)data, type, dfNodataValue, hasNodataValue);
    if (type == CDF_INT) calculate<int>(size, (int *)data, type, dfNodataValue, hasNodataValue);
    if (type == CDF_UINT) calculate<unsigned int>(size, (unsigned int *)data, type, dfNodataValue, hasNodataValue);
    if (type == CDF_FLOAT) calculate<float>(size, (float *)data, type, dfNodataValue, hasNodataValue);
    if (type == CDF_DOUBLE) calculate<double>(size, (double *)data, type, dfNodataValue, hasNodataValue);
  }

  template <class T> void calculate(size_t size, T *data, CDFType type, double dfNodataValue, bool hasNodataValue) {
    T _min = (T)NAN, _max = (T)NAN;
    double _sum = 0, _sumsquared = 0;
    numSamples = 0;
    T maxInf = (T)INFINITY;
    T minInf = (T)-INFINITY;

    bool checkInfinity = false;
    if (type == CDF_FLOAT || type == CDF_DOUBLE) checkInfinity = true;
    int firstDone = 0;

    for (size_t p = 0; p < size; p++) {
      T v = data[p];
      if ((((T)v) != (T)dfNodataValue || (!hasNodataValue)) && v == v) {
        if ((checkInfinity && v != maxInf && v != minInf) || (!checkInfinity)) {
          if (firstDone == 0) {
            _min = v;
            _max = v;
            firstDone = 1;
          } else {
            if (v < _min) _min = v;
            if (v > _max) _max = v;
          }
          _sum += v;
          _sumsquared += (v * v);
          numSamples++;
        }
      }
    }
    avg = _sum / double(numSamples);
    stddev = sqrt((numSamples * _sumsquared - _sum * _sum) / (numSamples * (numSamples - 1)));
    min = (double)_min;
    max = (double)_max;
  }

private:
  template <class T> void calcMinMax(size_t size, std::vector<DataObject> &dataObject);

public:
  double min, max, avg, stddev;
  size_t numSamples;

public:
  Statistics() {
    min = 0;
    max = 0;
    avg = 0;
    stddev = 0;
    numSamples = 0;
  }
  size_t getNumSamples() { return numSamples; };
  int calculate(CDataSource *dataSource);
  void setMinMax(MinMax minMax);
};

#endif
