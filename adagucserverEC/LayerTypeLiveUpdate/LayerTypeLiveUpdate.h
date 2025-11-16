#include "CDataSource.h"
#include "CDrawImage.h"
#include "CServerParams.h"
#include "CServerParams.h"
#include "CXMLGen.h"

#ifndef LAYERTYPELIVEUPDATE_H

#define LIVEUPDATE_DEFAULT_INTERVAL "PT10M"
#define LIVEUPDATE_DEFAULT_OFFSET "P1Y"
/**
 * Config for the time dimension (not backed by the DataSource info)
 */
struct LiveUpdateTimeRange {
  CT::string startTime;
  CT::string stopTime;
  CT::string defaultTime;
  CT::string interval;
};

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

/**
 * Configures the corresponding time range
 */
LiveUpdateTimeRange calculateLiveUpdateTimeRange(const char *interval = LIVEUPDATE_DEFAULT_INTERVAL, const char *offset = LIVEUPDATE_DEFAULT_OFFSET);

#endif // !LAYERTYPELIVEUPDATE_H
