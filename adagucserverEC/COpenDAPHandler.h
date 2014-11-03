#ifndef COpenDAPHandler_H
#define COpenDAPHandler_H

#include "Definitions.h"
#include "CStopWatch.h"
#include "CIBaseDataWriterInterface.h"
#include "CImgWarpNearestNeighbour.h"
#include "CImgWarpNearestRGBA.h"
#include "CImgWarpBilinear.h"
#include "CImgWarpBoolean.h"
#include "CImgRenderPoints.h"
#include "CStyleConfiguration.h"
#include "CMyCURL.h"
#include "CXMLParser.h"
#include "CDebugger.h"

class COpenDAPHandler{
public:
  DEF_ERRORFUNCTION();
  static int HandleOpenDAPRequest(const char *path,const char *query,CServerParams *srvParams);
};

#endif