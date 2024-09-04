#include "LayerMetadataType.h"

WMSLayer::WMSLayer() {
  hasError = 0;
  dataSource = NULL;
}

WMSLayer::~WMSLayer() {
  delete dataSource;
  dataSource = NULL;
  for (size_t j = 0; j < layerMetadata.projectionList.size(); j++) {
    delete layerMetadata.projectionList[j];
  }
  for (size_t j = 0; j < layerMetadata.dimList.size(); j++) {
    delete layerMetadata.dimList[j];
  }
  for (size_t j = 0; j < layerMetadata.styleList.size(); j++) {
    delete layerMetadata.styleList[j];
  }
}
