#ifndef UPDATELAYERMETADATA_H
#define UPDATELAYERMETADATA_H
#include <CRequest.h>

int setCRequestConfigFromEnvironment(CRequest *request, const char *additionalDataset = nullptr);

int updateLayerMetadata(CRequest &request);

#endif