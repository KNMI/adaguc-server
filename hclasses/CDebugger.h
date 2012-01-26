#include "CDebugger_H2.h"

//##define MEMLEAKCHECK

#ifdef MEMLEAKCHECK
  #include "CDebugger_H.h"
  #define new new(__FILE__, __LINE__)
#endif

