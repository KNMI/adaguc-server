#include "CDataSource.h"
#include "CDrawImage.h"
#include "CServerParams.h"
#include "CServerParams.h"
#include "CXMLGen.h"

#ifndef LAYERTYPELIVEUPDATE_H

/**
 * Configures a datasource with a fake time dimension
 */
int layerTypeLiveUpdateConfigureDimensionsInDataSource(CDataSource *dataSource);

/**
 * Renders time into an image the based on the time dimension in the GetMap request.
 */
int layerTypeLiveUpdateRenderIntoDrawImage(CDrawImage *image, CServerParams *srvParam);

/**
 * Configures an actual time range in a WMSLayer object. This is used for generating the Layer element in the WMS GetCapabilities file
 */
int layerTypeLiveUpdateConfigureWMSLayerForGetCapabilities(MetadataLayer *myWMSLayer);

#endif // !LAYERTYPELIVEUPDATE_H
