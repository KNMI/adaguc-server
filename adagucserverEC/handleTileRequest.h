#ifndef HANDLETILEREQUEST_H
#define HANDLETILEREQUEST_H

#include "CServerParams.h"
#include "CDataSource.h"

CDBStore::Store *handleTileRequest(CDataSource *dataSource);
f8box reprojectExtent(CT::string targetProjection, CT::string sourceProjection, CServerParams *srvParam, f8box inputbox);

#endif