
#include "GeometryUtils.h"

f8point compute2DPolygonCentroid(const std::vector<f8point> &vertices) {
  f8point centroid = {0, 0};
  double signedArea = 0.0;
  double x0 = 0.0; // Current vertex X
  double y0 = 0.0; // Current vertex Y
  double x1 = 0.0; // Next vertex X
  double y1 = 0.0; // Next vertex Y
  double a = 0.0;  // Partial signed area
  int vertexCount = vertices.size();
  int lastdex = vertexCount - 1;
  const f8point *prev = &(vertices[lastdex]);
  const f8point *next;

  // For all vertices in a loop
  for (int i = 0; i < vertexCount; ++i) {
    next = &(vertices[i]);
    x0 = prev->x;
    y0 = prev->y;
    x1 = next->x;
    y1 = next->y;
    a = x0 * y1 - x1 * y0;
    signedArea += a;
    centroid.x += (x0 + x1) * a;
    centroid.y += (y0 + y1) * a;
    prev = next;
  }

  signedArea *= 0.5;
  centroid.x /= (6.0 * signedArea);
  centroid.y /= (6.0 * signedArea);

  return centroid;
}

f8point getCentroid(const float *polyX, const float *polyY, const int numPoints) {
  std::vector<f8point> vertices(numPoints);
  for (int i = 0; i < numPoints; i++) {
    vertices[i].x = polyX[i];
    vertices[i].y = polyY[i];
  }
  return compute2DPolygonCentroid(vertices);
}

f8point getPixelCoordinateFromGetMapCoordinate(f8point in, CDataSource &dataSource) {
  f8point pixelCoord;
  pixelCoord.x =
      ((in.x - dataSource.srvParams->geoParams.bbox.left) / (dataSource.srvParams->geoParams.bbox.right - dataSource.srvParams->geoParams.bbox.left)) * dataSource.srvParams->geoParams.width;
  pixelCoord.y =
      ((in.y - dataSource.srvParams->geoParams.bbox.bottom) / (dataSource.srvParams->geoParams.bbox.top - dataSource.srvParams->geoParams.bbox.bottom)) * dataSource.srvParams->geoParams.height;
  return pixelCoord;
}

void getPixelCoordinateListFromGetMapCoordinateListInPlace(std::vector<f8point> &in, CDataSource &dataSource) {
  for (auto &p: in) {
    p = getPixelCoordinateFromGetMapCoordinate(p, dataSource);
  }
}

f8point getGetMapCoordinateFromPixelCoordinate(f8point in, CDataSource &dataSource) {
  f8point getmapCoord;
  getmapCoord.x = (in.x / dataSource.srvParams->geoParams.width) * (dataSource.srvParams->geoParams.bbox.right - dataSource.srvParams->geoParams.bbox.left) + dataSource.srvParams->geoParams.bbox.left;
  getmapCoord.y =
      (in.y / dataSource.srvParams->geoParams.height) * (dataSource.srvParams->geoParams.bbox.top - dataSource.srvParams->geoParams.bbox.bottom) + dataSource.srvParams->geoParams.bbox.bottom;
  return getmapCoord;
}

f8point getStrideFromGetMapLocation(CDataSource &dataSource, CImageWarper &warper, f8point pixelOffset) {

  // Calculate center of modelgrid
  f8point middlePointModelData = {.x = (dataSource.dfBBOX[0] + dataSource.dfBBOX[2]) / 2, .y = (dataSource.dfBBOX[1] + dataSource.dfBBOX[3]) / 2};
  f8point middlePoint = middlePointModelData;

  //  Convert to GetMap CRS coords
  warper.reprojpoint_inv(middlePointModelData);

  //  Convert to GetMap Pixel Coords
  f8point middleBoxInPixelCoords = getPixelCoordinateFromGetMapCoordinate(middlePointModelData, dataSource);

  f8point middleBoxInPixelCoordsX = middleBoxInPixelCoords;
  f8point middleBoxInPixelCoordsY = middleBoxInPixelCoords;

  // Add the desired pixel offset
  middleBoxInPixelCoordsX.x += pixelOffset.x;
  middleBoxInPixelCoordsY.y += pixelOffset.y;

  // Back to GetMap CRS coords
  auto getMapMiddleX = getGetMapCoordinateFromPixelCoordinate(middleBoxInPixelCoordsX, dataSource);
  auto getMapMiddleY = getGetMapCoordinateFromPixelCoordinate(middleBoxInPixelCoordsY, dataSource);

  // Back to model coords
  warper.reprojpoint(getMapMiddleX);
  warper.reprojpoint(getMapMiddleY);

  // Substract from model center, make it absolute and round up.
  return {.x = ceil(fabs((getMapMiddleX.x - middlePoint.x) / dataSource.dfCellSizeX)), .y = ceil(fabs((getMapMiddleY.y - middlePoint.y) / dataSource.dfCellSizeY))};
}
