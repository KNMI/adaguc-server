#include <cstddef>
#include <COGCDims.h>
#include <CCDFObject.h>

#include <CDataSource.h>
struct GFIElement {
  std::string value;
  std::string units;
  std::string standard_name;
  std::string feature_name;
  std::string long_name;
  std::string var_name;
  CCDFDims cdfDims;
  CDF::Variable *variable = nullptr;
  CDataSource *dataSource = nullptr;
};
struct GetFeatureInfoResult {
  int x_imagePixel, y_imagePixel;
  double x_imageCoordinate, y_imageCoordinate;
  double x_rasterCoordinate, y_rasterCoordinate;
  int x_rasterIndex, y_rasterIndex;
  double lat_coordinate, lon_coordinate;
  std::string layerName;
  std::string layerTitle;
  int dataSourceIndex;
  std::vector<GFIElement> elements;
};
