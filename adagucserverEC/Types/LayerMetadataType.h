
#ifndef LAYER_METADATA_TYPE_H
#define LAYER_METADATA_TYPE_H

#include "CTString.h"
#include <CDataSource.h>

struct LayerMetadataDim {
  CT::string serviceName;
  CT::string cdfName;
  CT::string units;
  CT::string values;
  CT::string defaultValue;
  CT::string type;
  int hasMultipleValues;
  bool hidden;
};

struct LayerMetadataProjection {
  LayerMetadataProjection(const CT::string &name, const double bbox[]) {
    this->name = name;
    for (size_t j = 0; j < 4; j++) {
      this->dfBBOX[j] = bbox[j];
    }
  }
  CT::string name;
  double dfBBOX[4];
};

struct LayerMetadataStyle {
  CT::string name;
  CT::string title;
  CT::string abstract;
};

struct LayerMetadataVariable {
  CT::string variableName;
  CT::string units;
  CT::string label;
  CT::string standard_name;
};

struct LayerMetadata {
  int width = -1;
  int height = -1;
  double cellsizeX = 0;
  double cellsizeY = 0;
  double dfLatLonBBOX[4] = {-180, -90, 180, 90};
  double dfBBOX[4] = {-180, -90, 180, 90};
  int isQueryable = 0;
  CT::string name, title, wmsgroup, abstract, nativeEPSG, projstring, collection;
  std::vector<LayerMetadataProjection> projectionList;
  std::vector<LayerMetadataDim> dimList;
  std::vector<LayerMetadataStyle> styleList;
  std::vector<LayerMetadataVariable> variableList;
};

// TODO should rename this class
class MetadataLayer {
public:
  // TODO: Would be nice to get rid of these in this class
  CServerConfig::XMLE_Layer *layer;
  CDataSource *dataSource = nullptr;
  CServerParams *srvParams;
  CT::string fileName;
  bool readFromDb = false;
  int hasError = 0;

  LayerMetadata layerMetadata;
};

#endif