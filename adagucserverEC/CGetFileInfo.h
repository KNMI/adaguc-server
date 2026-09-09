#include "CTString.h"
#include "CDebugger.h"

#ifndef CGETFILEINFO_H
#define CGETFILEINFO_H
class CGetFileInfo {
public:
  static std::string getLayersForFile(const char *filename);
};

#endif