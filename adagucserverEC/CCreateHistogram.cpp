#include "CCreateHistogram.h"
#include "CGenericDataWarper.h"
#include "CImageDataWriter.h"
const char *CCreateHistogram::className = "CCreateHistogram";

int CCreateHistogram::createHistogram(CDataSource *dataSource, CDrawImage *) {

  CDBDebug("createHistogram");
  CDBDebug("Building JSON");

  CT::string resultJSON;
  if (dataSource->srvParams->JSONP.length() == 0) {
    CDBDebug("CREATING JSON");
    printf("%s%s%c%c\n", "Content-Type: application/json", dataSource->srvParams->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
  } else {
    CDBDebug("CREATING JSONP %s", dataSource->srvParams->JSONP.c_str());
    printf("%s%s%c%c", "Content-Type: application/javascript", dataSource->srvParams->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
    printf("\n%s(", dataSource->srvParams->JSONP.c_str());
  }

  // puts("{\"a\": 1}");

  if (dataSource->srvParams->JSONP.length() != 0) {
    printf(");");
  }
  resetErrors();

  return 0;
}

int CCreateHistogram::init(CServerParams *, CDataSource *dataSource, int) {
  baseDataSource = dataSource;

  //  int numTimeSteps = dataSource->getNumTimeSteps();

  JSONdata = "";

  return 0;
}
int CCreateHistogram::addData(std::vector<CDataSource *> &dataSources) {
  CDBDebug("addData");

  int status;
  for (size_t i = 0; i < dataSources.size(); i++) {

    CDataSource *dataSource = dataSources[i];
    JSONdata.concat("{");
    CDataReader reader;
    status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_ALL);

    if (status != 0) {
      CDBError("Could not open file: %s", dataSource->getFileName());
      return 1;
    }

    // Warp
    void *warpedData = NULL;

    double dfNoData = NAN;
    if (dataSource->getDataObject(0)->hasNodataValue == 1) {
      dfNoData = dataSource->getDataObject(0)->dfNodataValue;
    }

    CDF::allocateData(CDF_FLOAT, &warpedData, dataSource->srvParams->geoParams.width * dataSource->srvParams->geoParams.height);

    if (CDF::fill(warpedData, CDF_FLOAT, dfNoData, dataSource->srvParams->geoParams.width * dataSource->srvParams->geoParams.height) != 0) {
      CDBError("Unable to initialize data field to nodata value");
      return 1;
    }

    CImageWarper warper;

    status = warper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);
    if (status != 0) {
      CDBError("Unable to initialize projection");
      return 1;
    }

    for (int k = 0; k < dataSource->getNumTimeSteps(); k++) {
      dataSource->setTimeStep(k);

      CDataReader reader;
      status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_ALL);
      if (status != 0) {
        CDBError("Could not open file: %s", dataSource->getFileName());
        return 1;
      }

      void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;
      CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();

      CCreateHistogramSettings settings;
      settings.width = dataSource->srvParams->geoParams.width;
      settings.height = dataSource->srvParams->geoParams.height;
      settings.data = warpedData;

      GeoParameters sourceGeo;

      sourceGeo.width = dataSource->dWidth;
      sourceGeo.height = dataSource->dHeight;
      sourceGeo.bbox = dataSource->dfBBOX;
      sourceGeo.cellsizeX = dataSource->dfCellSizeX;
      sourceGeo.cellsizeY = dataSource->dfCellSizeY;
      sourceGeo.crs = dataSource->nativeProj4;

      CDBDebug("Rendering %f,%f", sourceGeo.bbox.left, sourceGeo.bbox.bottom);
      GenericDataWarper genericDataWarper;
      GDWArgs args = {.warper = &warper, .sourceData = sourceData, .sourceGeoParams = sourceGeo, .destGeoParams = dataSource->srvParams->geoParams};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return drawFunction(x, y, val, warperState, settings); });
      ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
    }
    reader.close();
    CDBDebug("Addata finished, data warped");

    if (dataSource->statistics == NULL) {
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->calculate(dataSource);
      dataSource->statistics->calculate(dataSource->srvParams->geoParams.width * dataSource->srvParams->geoParams.height, (float *)warpedData, CDF_FLOAT, dataSource->getDataObject(0)->dfNodataValue,
                                        dataSource->getDataObject(0)->hasNodataValue);
    }
    float fieldMin = (float)dataSource->statistics->getMinimum();
    float fieldMax = (float)dataSource->statistics->getMaximum();

    int maxNumBins = 50;

    int bins[maxNumBins];

    for (int j = 0; j < maxNumBins; j++) bins[j] = 0;

    double min = CImageDataWriter::getValueForColorIndex(dataSource, 0);
    double max = CImageDataWriter::getValueForColorIndex(dataSource, 240);
    if (max == INFINITY) max = 239;
    if (min == INFINITY) min = 0;
    if (max == min) max = max + 0.000001;
    // Round min and max

    double roundFactor1 = pow(10, floor(log10((max - min) / double(maxNumBins))));
    CDBDebug("roundFactor1 %f", roundFactor1);
    double roundFactor = floor(((max - min) / double(maxNumBins)) / roundFactor1) * roundFactor1;
    double binSize = roundFactor * 2;
    min = floor(min / roundFactor) * roundFactor;
    max = ceil(max / roundFactor) * roundFactor;
    CDBDebug("binSize = %f min=%f max=%f", binSize, min, max);

    size_t gridSize = dataSource->srvParams->geoParams.width * dataSource->srvParams->geoParams.height;
    for (size_t j = 0; j < gridSize; j++) {
      float val = ((float *)warpedData)[j];
      if (val != (float)dfNoData) {
        // CDBDebug("%f",val);
        int binIndex = int((val - min) / binSize);
        if (binIndex < 0 || binIndex >= maxNumBins) {
          // CDBError("Histogram errors!");
        } else {
          bins[binIndex]++;
        }
      }
    }

    int numBins = floor((max - min) / binSize);

    //       JSONdata.printconcat("\"dims\":{");
    //       for(size_t d=0;d<dataSources[i]->requiredDims.size();d++){
    //         JSONdata.printconcat("\"%s\":\"%s\"",dataSources[i]->requiredDims[d]->name.c_str(),dataSources[i]->requiredDims[d]->value.c_str());
    //       }
    //
    //       JSONdata.concat("},");
    //

    JSONdata.printconcat("\"%s\":{", dataSource->layerName.c_str());

    // Print info
    JSONdata.printconcat("\"numdatasources\":%d,", dataSources.size());
    JSONdata.printconcat("\"numdataobjects\":%d,", dataSource->getNumDataObjects());
    // Units
    JSONdata.printconcat("\"units\":\"%s\",", dataSource->getDataObject(0)->getUnits().c_str());

    // Name
    JSONdata.printconcat("\"layername\":\"%s\",", dataSource->layerName.c_str());

    // Min/Max
    JSONdata.printconcat("\"min\":%f,", dataSource->statistics->getMinimum());
    JSONdata.printconcat("\"max\":%f,", dataSource->statistics->getMaximum());

    JSONdata.printconcat("\"average\":%f,", dataSource->statistics->getAverage());
    JSONdata.printconcat("\"stddev\":%f,", dataSource->statistics->getStdDev());

    // FieldMin/Fieldmax
    JSONdata.printconcat("\"fieldmin\":%f,", fieldMin);
    JSONdata.printconcat("\"fieldmax\":%f,", fieldMax);

    // Print interval
    JSONdata.printconcat("\"interval\":[");
    for (int j = 0; j < numBins; j++) {
      if (j > 0) {
        JSONdata.concat(",");
      }
      JSONdata.printconcat("%f", j * binSize + min);
    }
    JSONdata.concat("],");
    // Print quantity
    JSONdata.printconcat("\"quantity\":[");
    for (int j = 0; j < numBins; j++) {
      if (j > 0) {
        JSONdata.concat(",");
      }
      JSONdata.printconcat("%d", bins[j]);
    }
    JSONdata.concat("],");
    // Print width
    JSONdata.printconcat("\"width\":[");
    for (int j = 0; j < numBins; j++) {
      if (j > 0) {
        JSONdata.concat(",");
      }
      JSONdata.printconcat("%f", binSize);
    }
    JSONdata.concat("]");
    JSONdata.concat("}"); // layer
    JSONdata.concat("}");
  }

  return 0;
}
int CCreateHistogram::end() {

  CDBDebug("createHistogram");
  CDBDebug("Building JSON");

  CT::string resultJSON;
  if (baseDataSource->srvParams->JSONP.length() == 0) {
    CDBDebug("CREATING JSON");
    printf("%s%s%c%c\n", "Content-Type: application/json", baseDataSource->srvParams->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
  } else {
    CDBDebug("CREATING JSONP %s", baseDataSource->srvParams->JSONP.c_str());
    printf("%s%s%c%c", "Content-Type: application/javascript", baseDataSource->srvParams->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
    printf("\n%s(", baseDataSource->srvParams->JSONP.c_str());
  }

  puts(JSONdata.c_str());

  if (baseDataSource->srvParams->JSONP.length() != 0) {
    printf(");");
  }
  resetErrors();

  return 0;
}
