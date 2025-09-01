#ifndef CNetCDFDataWriter_H
#define CNetCDFDataWriter_H

#include "Definitions.h"
#include "CGenericDataWarper.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CIBaseDataWriterInterface.h"
#include "CDebugger.h"

#define CNetCDFDataWriter_NEAREST 0
#define CNetCDFDataWriter_AVG_RGB 1
class CNetCDFDataWriter : public CBaseDataWriterInterface {
private:
  CT::string JSONdata;
  DEF_ERRORFUNCTION();
  class Settings {
  public:
    size_t width;
    size_t height;
    void *data;
    float *rField, *gField, *bField;
    int *numField;
    bool trueColorRGBA;
  };

  template <class T> static void drawFunction_nearest(int x, int y, T val, GDWState &, Settings &settings) {
    if (x >= 0 && y >= 0 && x < (int)settings.width && y < (int)settings.height) {
      ((T *)settings.data)[x + y * settings.width] = val;
    }
  };

  int drawFunctionMode;

  template <class T> static void drawFunction_avg_rgb(int x, int y, T val, GDWState &, Settings &settings) {
    if (x >= 0 && y >= 0 && x < (int)settings.width && y < (int)settings.height) {

      size_t p = x + y * settings.width;
      uint v = val;
      unsigned char r = ((unsigned char)v);
      unsigned char g = ((unsigned char)(v >> 8));
      unsigned char b = ((unsigned char)(v >> 16));
      unsigned char a = ((unsigned char)(v >> 24));

      settings.numField[p]++;
      settings.rField[p] += r;
      settings.gField[p] += g;
      settings.bField[p] += b;

      r = (unsigned char)(settings.rField[p] / settings.numField[p]);
      g = (unsigned char)(settings.gField[p] / settings.numField[p]);
      b = (unsigned char)(settings.bField[p] / settings.numField[p]);

      if (a != 255) {
        // For cairo, Alpha is precomputed into components. We need to do this here as well.
        unsigned char r1 = float(r) * (float(a) / 256.);
        unsigned char g1 = float(g) * (float(a) / 256.);
        unsigned char b1 = float(b) * (float(a) / 256.);
        ((T *)settings.data)[p] = r1 + g1 * 256 + b1 * 256 * 256 + a * 256 * 256 * 256;
      } else {
        ((T *)settings.data)[p] = r + g * 256 + b * 256 * 256 + 4278190080;
      }
    }
  };
  CDataSource *baseDataSource;
  CDFObject *destCDFObject;
  CT::string tempFileName;
  CServerParams *srvParam;
  CDF::Dimension *projectionDimX, *projectionDimY; // Shorthand pointers to cdfdatamodel (do never delete!)
  CDF::Variable *projectionVarX, *projectionVarY;  // Shorthand pointers to cdfdatamodel (do never delete!)
  void createProjectionVariables(CDFObject *cdfObject, int width, int height, double *bbox);

public:
  CNetCDFDataWriter();
  ~CNetCDFDataWriter();
  bool silent = false;
  // Virtual functions
  int init(CServerParams *srvParam, CDataSource *dataSource, int nrOfBands);
  int addData(std::vector<CDataSource *> &dataSources);
  int writeFile(const char *fileName, int adagucTileLevel, bool enableCompression);
  int end();

  void setInterpolationMode(int mode);
};

#endif
