
#ifndef LAYER_METADATA_TYPE_H
#define LAYER_METADATA_TYPE_H

#include "CTString.h"
#include <CDataSource.h>

struct LayerMetadataDim {
  std::string serviceName;
  std::string cdfName;
  std::string units;
  std::string values;
  std::string defaultValue;
  std::string type;
  int hasMultipleValues;
  bool hidden;
};

struct LayerMetadataProjection {
  LayerMetadataProjection(const std::string &name, const double bbox[]) {
    this->name = name;
    for (size_t j = 0; j < 4; j++) {
      this->dfBBOX[j] = bbox[j];
    }
  }
  std::string name;
  double dfBBOX[4];
};

struct LayerMetadataStyle {
  std::string name;
  std::string title;
  std::string abstract;
};

struct LayerMetadataVariable {
  std::string variableName;
  std::string units;
  std::string label;
  std::string standard_name;
};

struct LayerMetadata {
  int width = -1;
  int height = -1;
  double cellsizeX = 0;
  double cellsizeY = 0;
  double dfLatLonBBOX[4] = {-180, -90, 180, 90};
  double dfBBOX[4] = {-180, -90, 180, 90};
  int isQueryable = 0;
  bool hidden = false;
  bool enable_edr = true;
  std::string name, title, wmsgroup, abstract, nativeEPSG, projstring, collection;
  std::vector<LayerMetadataProjection> projectionList;
  std::vector<LayerMetadataDim> dimList;
  std::vector<LayerMetadataStyle> styleList;
  std::vector<LayerMetadataVariable> variableList;
};

// TODO should rename this class
struct MetadataLayer {
  // TODO: Would be nice to get rid of these in this class
  CServerConfig::XMLE_Layer *layer = nullptr;
  CDataSource *dataSource = nullptr;
  CServerParams *srvParams = nullptr;
  std::string fileName;
  bool readFromDb = false;
  int hasError = 0;
  LayerMetadata layerMetadata;
};

#endif