#ifndef CHandleMetadata_H
#define CHandleMetadata_H
#include "CDebugger.h"
#include "CServerParams.h"

class CHandleMetadata {
    private:
    DEF_ERRORFUNCTION();
  
    public:
    int process(CServerParams *srvParam);

};

#endif
