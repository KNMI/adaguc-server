#ifndef CMakeEProfile_H
#define CMakeEProfile_H

#include <string>
#include <map>
#include <set>

#include "CImageDataWriter.h"
#include "CDrawImage.h"
#include "CImageWarper.h"

class CMakeEProfile {
public:
  class DayPass {
  public:
    DayPass(int x, double offset);
    double offset;
    int x;
  };
  static int MakeEProfile(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, std::string &json);
};

#endif