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
 * Renders a Live Update layer depending on the presence or absence of post-processors.
 */
int layerTypeLiveUpdateRender(CDataSource *dataSource, CServerParams *srvParam);

/**
 * Renders time into an image the based on the time dimension in the GetMap request.
 */
int layerTypeLiveUpdateRenderIntoDrawImage(CDrawImage *image, CServerParams *srvParam);

/**
 * Renders an image algorithmically based on the time dimension in the GetMap request.
 */
int layerTypeLiveUpdateRenderIntoImageDataWriter(CDataSource *dataSource, CServerParams *srvParam);

/**
 * Configures an actual time range in a WMSLayer object. This is used for generating the Layer element in the WMS GetCapabilities file
 */
int layerTypeLiveUpdateConfigureWMSLayerForGetCapabilities(MetadataLayer *metadataLayer);

#endif // !LAYERTYPELIVEUPDATE_H
