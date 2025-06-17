#include "CTypes.h"
#include "CDebugger.h"

#ifndef CGETFILEINFO_H
#define CGETFILEINFO_H
class CGetFileInfo {
private:
  DEF_ERRORFUNCTION();

public:
  static CT::string getLayersForFile(const char *filename);
};

#endif