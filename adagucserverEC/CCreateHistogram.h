#ifndef CCreateHistogram_H
#define CCreateHistogram_H

#include "CGenericDataWarper.h"
#include "CIBaseDataWriterInterface.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CServerParams.h"

class CCreateHistogram : public CBaseDataWriterInterface {
private:
  std::string JSONdata;

  struct CCreateHistogramSettings {
    size_t width;
    size_t height;
    void *data;
  };

  template <class T> static void drawFunction(int x, int y, T val, GDWState &, CCreateHistogramSettings &settings) {
    if (x >= 0 && y >= 0 && x < (int)settings.width && y < (int)settings.height) {
      ((float *)settings.data)[x + y * settings.width] = val;
    }
  }
  CDataSource *baseDataSource;

public:
  virtual ~CCreateHistogram() {};
  static int createHistogram(CDataSource *dataSource, CDrawImage *legendImage);
  // Virtual functions
  int init(CServerParams *srvParam, CDataSource *dataSource, int nrOfBands);
  int addData(std::vector<CDataSource *> &dataSources);
  int end();
};

#endif
