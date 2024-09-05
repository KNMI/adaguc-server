#include "LayerMetadataType.h"

WMSLayer::WMSLayer() {
  hasError = 0;
  dataSource = NULL;
}

WMSLayer::~WMSLayer() {
  for (size_t j = 0; j < layerMetadata.projectionList.size(); j++) {
    delete layerMetadata.projectionList[j];
  }
  layerMetadata.projectionList.clear();
  for (size_t j = 0; j < layerMetadata.dimList.size(); j++) {
    delete layerMetadata.dimList[j];
  }
  layerMetadata.dimList.clear();
  for (size_t j = 0; j < layerMetadata.styleList.size(); j++) {
    delete layerMetadata.styleList[j];
  }
  layerMetadata.styleList.clear();
}
