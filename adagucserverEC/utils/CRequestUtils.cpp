#include <CServerParams.h>
#include <CDataSource.h>
#include <CImageWarper.h>

#include "CRequestUtils.h"
#include "LayerUtils.h"
#include <tuple>
#include <array>

std::tuple<int, std::array<double, 4>> findBBoxForDataSource(std::vector<CDataSource *> dataSources) {
  double dfBBOX[4] = {-180, -90, 180, 90};
  for (size_t d = 0; d < dataSources.size(); d++) {
    if (dataSources[d]->dLayerType != CConfigReaderLayerTypeCascaded && dataSources[d]->dLayerType != CConfigReaderLayerTypeBaseLayer &&
        dataSources[d]->dLayerType != CConfigReaderLayerTypeLiveUpdate) {
      CImageWarper warper;
      CDataReader reader;
      int status = reader.open(dataSources[d], CNETCDFREADER_MODE_OPEN_HEADER);
      reader.close();
      status = warper.initreproj(dataSources[d], dataSources[0]->srvParams->Geo, &dataSources[0]->srvParams->cfg->Projection);
      if (status != 0) {
        warper.closereproj();
        CDBDebug("Unable to initialize projection ");
      }
      warper.findExtent(dataSources[d], dfBBOX);
      warper.closereproj();
      CDBDebug("Found bbox %s %f %f %f %f", dataSources[0]->srvParams->Geo->CRS.c_str(), dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]);
      std::array<double, 4> box = {dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]};
      return std::make_tuple(0, box);
    }
  }

  std::array<double, 4> box = {dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]};
  return std::make_tuple(-1, box);
}

CServerConfig::XMLE_Layer *findLayerConfigForRequestedLayer(CServerParams *srvParam, CT::string requestedLayerName) {
  for (size_t layerNo = 0; layerNo < srvParam->cfg->Layer.size(); layerNo++) {
    auto cfgLayer = srvParam->cfg->Layer[layerNo];
    if (makeUniqueLayerName(cfgLayer).equals(requestedLayerName)) {
      return cfgLayer;
    }
  }
  return nullptr;
}