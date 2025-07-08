#ifndef HANDLETILEREQUEST_H
#define HANDLETILEREQUEST_H

#include "CServerParams.h"
#include "CDataSource.h"

CDBStore::Store *handleTileRequest(CDataSource *dataSource);
int findExtentForTiles(const char *srcProj4Str, CServerParams *srvParam, double nativeViewPortBBOX[4]);

#endif