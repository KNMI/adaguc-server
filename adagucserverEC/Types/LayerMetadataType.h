
#ifndef LAYER_METADATA_TYPE_H
#define LAYER_METADATA_TYPE_H

#include "CTString.h"
#include <CDataSource.h>

struct LayerMetadataDim {
  CT::string name;
  CT::string units;
  CT::string values;
  CT::string defaultValue;
  int hasMultipleValues;
  bool hidden = false;
};

struct LayerMetadataProjection {
  CT::string name;
  double dfBBOX[4];
};

struct LayerMetadataStyle {
  CT::string name;
  CT::string title;
  CT::string abstract;
};

struct LayerMetadataVariable {
  CT::string units;
};

struct LayerMetadata {

  int width = -1;
  int height = -1;
  double cellsizeX = 0;
  double cellsizeY = 0;
  double dfLatLonBBOX[4] = {-180, -90, 180, 90};
  double dfBBOX[4] = {-180, -90, 180, 90};
  int isQueryable = 0;
  CT::string name, title, group, abstract, nativeEPSG;

  std::vector<LayerMetadataProjection *> projectionList;
  std::vector<LayerMetadataDim *> dimList;
  std::vector<LayerMetadataStyle *> styleList;
  std::vector<LayerMetadataVariable *> variableList;
};

// TODO should rename this class
class WMSLayer {
public:
  WMSLayer();
  ~WMSLayer();
  // TODO: Would be nice to get rid of these in this class
  CServerConfig::XMLE_Layer *layer;
  CDataSource *dataSource;
  CServerParams *srvParams;
  CT::string fileName;
  bool readFromDb = false;

  int hasError;

  LayerMetadata layerMetadata;
};

#endif