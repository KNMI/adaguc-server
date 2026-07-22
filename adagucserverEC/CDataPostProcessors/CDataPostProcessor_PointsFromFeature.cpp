#include <type_traits>
#include "CDataPostProcessor_PointsFromFeature.h"
#include <CImageWarper.h>
#include <CConvertGeoJSON.h>
#include <sstream>
#include <utils/GeometryUtils.h>

/************************/
/*      CDPPointsFromFeature     */
/************************/

const char *CDPPointsFromFeature::getId() { return CDATAPOSTPROCESSOR_PointsFromFeature_ID; }
int CDPPointsFromFeature::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm == CDATAPOSTPROCESSOR_PointsFromFeature_ID) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPointsFromFeature::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *, size_t) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  CDBError("Not implemented yet");
  return 1;
}

int CDPPointsFromFeature::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {

  if (isApplicable(proc, dataSource, mode) == false) return -1;
  if (mode != CDATAPOSTPROCESSOR_RUNAFTERREADING) return 0;

  CDBDebug(CDATAPOSTPROCESSOR_PointsFromFeature_ID);

  // Init projection
  CImageWarper warper;
  if (dataSource->srvParams->geoParams.crs.empty()) {
    dataSource->srvParams->geoParams.crs = "EPSG:4326";
  }
  warper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);

  int width = dataSource->srvParams->geoParams.width;
  int height = dataSource->srvParams->geoParams.height;

  // Get features
  std::map<std::string, std::vector<Feature *>> &featureStore = CConvertGeoJSON::featureStore;

  std::string name = dataSource->featureSet;

  if (name.empty()) {
    CDBWarning("FeatureSet for dataSource is empty");
    return 0;
  }

  auto it = featureStore.find(name);
  if (it == featureStore.end()) {
    CDBWarning("FeatureSet %s not found", name.c_str());
    return 0;
  }

  std::vector<Feature *> &features = it->second;

  if (dataSource->dataObjects.empty()) {
    CDBError("PointsFromFeature: No starting data objects available in dataSource");
    return 1;
  }
  auto &featuresObject = dataSource->dataObjects[0];

  // Split variable list
  std::stringstream ss(proc->attr.select);
  std::string variable;
  std::vector<DataObject> newObjects;
  while (std::getline(ss, variable, ',')) {
    // Look up variable, or create from scratch
    auto cdfVar = featuresObject.cdfObject->getVariableNE(variable.c_str());
    if (!cdfVar) {
      CDBWarning("PointsFromFeature: variable %s not found in cdfObject. Creating.", variable.c_str());
      auto cdfObj = featuresObject.cdfObject;
      cdfVar = cdfObj->addVariable(new CDF::Variable(variable, CDF_FLOAT, {cdfObj->getDimOrCreate("y", height), cdfObj->getDimOrCreate("x", width)}, false));
      cdfVar->setAttributeText("standard_name", variable);
    }

    // Data object for property
    DataObject destob;
    destob.cdfVariable = cdfVar;
    destob.variableName = variable;
    std::string varName = cdfVar->name.c_str();

    for (Feature *feature: features) {
      // Extract associated value from GeoJSON properties
      float value = NAN;
      auto fp = feature->getFp();
      auto itProp = fp->find(varName);

      if (itProp != fp->end()) {
        FeatureProperty *prop = itProp->second;
        if (prop->getType() != typeStr) {
          value = (float)prop->getDblVal();
        }
      }

      // If geometry is a point
      for (auto &pt: *feature->getPoints()) {
        f8point p = {.x = pt.getLon(), .y = pt.getLat()};
        warper.reprojfromLatLon(p.x, p.y);
        p = getPixelCoordinateFromGetMapCoordinate(p, *dataSource);

        int px = (int)p.x;
        int py = (int)p.y;

        if (px < 0 || py < 0 || px >= width || py >= height) continue;

        double lon = pt.getLon();
        double lat = pt.getLat();
        float val = value;

        auto newPoint = PointDVWithLatLon(px, py, lon, lat, val);
        destob.points.push_back(newPoint);
      }

      // If geometry is a polygon (use centroid calculation)
      for (auto &poly: *feature->getPolygons()) {
        int n = poly.getSize();
        if (n <= 0) continue;

        f8point centroid = getCentroid(poly.getLons(), poly.getLats(), n);

        warper.reprojfromLatLon(centroid.x, centroid.y);
        centroid = getPixelCoordinateFromGetMapCoordinate(centroid, *dataSource);

        int px = (int)centroid.x;
        int py = (int)centroid.y;

        if (px < 0 || py < 0 || px >= width || py >= height) continue;

        destob.points.emplace_back(px, py, centroid.x, centroid.y, value);
      }

      // If geometry is a line (use the first point)
      for (auto &line: *feature->getPolylines()) {
        int n = line.getSize();
        if (n <= 0) continue;

        f8point p = {.x = line.getLons()[0], .y = line.getLats()[0]};

        warper.reprojfromLatLon(p.x, p.y);
        p = getPixelCoordinateFromGetMapCoordinate(p, *dataSource);

        int px = (int)p.x;
        int py = (int)p.y;

        if (px < 0 || py < 0 || px >= width || py >= height) continue;

        destob.points.emplace_back(px, py, p.x, p.y, value);
      }
    }

    // Save data object
    newObjects.push_back(destob);
  }

  // Clear out the original "features" placeholder object, push new data objects
  dataSource->dataObjects.clear();
  for (const auto &obj: newObjects) {
    dataSource->dataObjects.push_back(obj);
  }
  dataSource->hasFieldData = false;
  return 0;
}