#include "CTypes.h"

class CGetFileInfo{
private:
  DEF_ERRORFUNCTION();
public:
  static CT::string getLayersForFile(const char *filename);
};
