#ifndef getFeatureInfoVirtualForSolarTerminator_H
#define getFeatureInfoVirtualForSolarTerminator_H
#include <CImageDataWriter.h>
int getFeatureInfoVirtualForSolarTerminator(CImageDataWriter *img, std::vector<CDataSource *> dataSources, int dataSourceIndex, int dX, int dY, CServerParams *srvParams);
#endif