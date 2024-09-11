#include "LayerMetadataType.h"

MetadataLayer::MetadataLayer() {
  hasError = 0;
  dataSource = NULL;
}

MetadataLayer::~MetadataLayer() {
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
