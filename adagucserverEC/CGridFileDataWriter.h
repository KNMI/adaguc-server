#ifndef CGridFileDataWriter_H
#define CGridFileDataWriter_H
#include "CIBaseDataWriterInterface.h"
#include "CDataReader.h"
#include "CGenericDataWarper.h"
#include "GridFileWriters/GridFileData.h"

/**
 * The output drivers that a <WCSFormat driver="..."> can name
 */
enum class WCSDriver { NetCDF, GeoTiff, AAIGrid, Unsupported };

/**
 * Resolves the driver name of a WCSFormat (case insensitive): ADAGUCNetCDF, GTiff and AAIGrid.
 * NetCDF, the name of the GDAL NetCDF driver used in older configurations, maps to the ADAGUC NetCDF writer.
 * Without a driver name, the format name decides (geotiff or aaigrid).
 */
WCSDriver getWCSDriver(const std::string &driverName, const std::string &formatName);

/**
 * Error message for a WCSFormat with a driver that is not supported
 */
std::string getUnsupportedWCSDriverMessage(const std::string &driverName, const std::string &formatName);

/**
 * WCS GetCoverage writer for the GeoTIFF (GTiff) and AAIGrid output formats: warps every band to the requested grid and writes it with
 * libtiff/libgeotiff or as text, in the layout GDAL used to write before GDAL was removed from adaguc-server.
 */
class CGridFileDataWriter : public CBaseDataWriterInterface {
private:
  struct DrawFunctionState {
    size_t width;
    size_t height;
    void *data;
  };

  template <class T> static void drawFunction(int x, int y, T val, GDWState &, DrawFunctionState &drawFunctionState) {
    if (x >= 0 && y >= 0 && x < (int)drawFunctionState.width && y < (int)drawFunctionState.height) {
      ((T *)drawFunctionState.data)[x + y * drawFunctionState.width] = val;
    }
  }

  CServerParams *srvParam = nullptr;
  WCSDriver driver;
  double dfDstBBOX[4];
  double dfSrcBBOX[4];
  CDataReader reader;

  int NrOfBands = 0;
  std::string mimeType;
  std::string TimeUnit;
  GridFileData grid;

  CDataSource *_dataSource = nullptr;

  std::string generateGetCoverageFileName();
  std::string getDimensionValue(int d, CCDFDims *dims);
  int initCRS();
  void addDatasetMetadata();
  int writeFile(const std::string &fileName);
  int streamFile(const std::string &fileName);

public:
  /** driver must be GeoTiff or AAIGrid, mimeType is the one configured for the WCSFormat */
  CGridFileDataWriter(WCSDriver driver, const std::string &mimeType) : driver(driver), mimeType(mimeType) {}

  // Virtual functions
  int init(CServerParams *srvParam, CDataSource *dataSource, int nrOfBands);
  int addData(std::vector<CDataSource *> &dataSources);
  int end();

  friend std::string generateUniqueGetCoverageFileName(CGridFileDataWriter *dataWriter);
};
#endif
