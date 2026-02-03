#include "CTString.h"
#include "CDebugger.h"

#ifndef CGETFILEINFO_H
#define CGETFILEINFO_H
class CGetFileInfo {
private:
public:
  static CT::string getLayersForFile(const char *filename);
};

#endif