#include <cstddef>
#include <COGCDims.h>
#include <CCDFObject.h>
#include <CDataSource.h>
class GetFeatureInfoResult {
public:
  ~GetFeatureInfoResult() {
    for (size_t j = 0; j < elements.size(); j++) delete elements[j];
  }

  int x_imagePixel, y_imagePixel;
  double x_imageCoordinate, y_imageCoordinate;
  double x_rasterCoordinate, y_rasterCoordinate;
  int x_rasterIndex, y_rasterIndex;
  double lat_coordinate, lon_coordinate;

  std::string layerName;
  std::string layerTitle;
  int dataSourceIndex;

  class Element {
  public:
    std::string value;
    std::string units;
    std::string standard_name;
    std::string feature_name;
    std::string long_name;
    std::string var_name;
    // CT::string time;
    CDF::Variable *variable;
    CDataSource *dataSource;
    CCDFDims cdfDims;
  };
  std::vector<Element *> elements;
};
