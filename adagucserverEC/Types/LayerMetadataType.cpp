#include "LayerMetadataType.h"

LayerMetadataProjection::LayerMetadataProjection(const std::string &name, const double bbox[]) {
  this->name = name;
  for (size_t j = 0; j < 4; j++) {
    this->dfBBOX[j] = bbox[j];
  }
}
