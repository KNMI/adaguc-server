/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Packages PNG into a NetCDF file
 * Author:   Maarten Plieger (KNMI)
 * Date:     2026-09-10
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include "CCDFPNGIO.h"

#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <CReadFile.h>

#include <cmath>
#include "../adagucserverEC/Types/GeoParameters.h"

static const bool CCDFPNGIO_DEBUG = false;

f8point tileXYZtoMerc(int tile_x, int tile_y, int zoom) {
  double tileSize = 256;
  double initialResolution = 2 * M_PI * 6378137 / tileSize;
  double originShift = 2 * M_PI * 6378137 / 2.0;
  double tileRes = initialResolution / pow(2, zoom);
  f8point p;
  p.x = originShift - tile_x * tileRes;
  p.y = originShift - tile_y * tileRes;
  return p;
}

f8box getBounds(int tile_x, int tile_y, int zoom) {
  f8point p1 = tileXYZtoMerc((tile_x - 1) * 256, (tile_y) * 256, zoom);
  f8point p2 = tileXYZtoMerc((tile_x) * 256, (tile_y + 1) * 256, zoom);
  f8box b;
  b.left = p2.x;
  b.right = p1.x;
  b.bottom = p2.y;
  b.top = p1.y;
  return b;
}

int CDFPNGReader::open(const char *fileName) {

  if (cdfObject == NULL) {
    CDBError("No CDFObject defined, use CDFObject::attachCDFReader(CDFNetCDFReader*). Please note that this function should be called by CDFObject open routines.");
    return 1;
  }
  if (CCDFPNGIO_DEBUG) {
    CDBDebug("open [%s]", fileName);
  }
  this->fileName = fileName;

  if (pngRaster != NULL) {
    CDBError("pngRaster already defined!");
    return 1;
  }

  cdfObject->addAttribute(new CDF::Attribute("Conventions", "CF-1.6"));
  cdfObject->addAttribute(new CDF::Attribute("history", "Metadata adjusted by ADAGUC from PNG to NetCDF-CF"));

  std::string fileBaseName;
  const char *last = rindex(fileName, '/');
  if ((last != NULL) && (*last)) {
    fileBaseName = (last + 1);
  } else {
    fileBaseName = (fileName);
  }

  /* Now always add a CRS variable */
  CDF::Variable *CRS = cdfObject->getVariableNE("crs");
  if (CRS == NULL) {
    CRS = cdfObject->addVariable(new CDF::Variable("crs", CDF_UINT, NULL, 0, false));
  }

  try {
    std::string infoFile = fileName;
    infoFile += ".info";
    std::string infoData = readFile(infoFile);
    std::vector<std::string> lines = CT::split(infoData, "\n");

    for (size_t l = 0; l < lines.size(); l++) {
      CDBDebug("Info file line %s", lines[l].c_str());
      if (CT::startsWith(lines[l], "proj4_params=")) {
        std::string proj4Params = CT::substring(lines[l], 13, -1);
        CDBDebug("proj4params=%s", proj4Params.c_str());
        CRS->setAttributeText("proj4", proj4Params.c_str());
      }
      if (CT::startsWith(lines[l], "bbox=")) {
        std::string bbox = CT::substring(lines[l], 5, -1);
        std::vector<std::string> bboxItems = CT::split(bbox, ",");
        if (bboxItems.size() == 4) {
          double d[4];
          d[0] = CT::toDouble(bboxItems[0]);
          d[1] = CT::toDouble(bboxItems[1]);
          d[2] = CT::toDouble(bboxItems[2]);
          d[3] = CT::toDouble(bboxItems[3]);
          CRS->setAttribute("bbox", CDF_DOUBLE, d, 4);
        }
      }
    }
  } catch (int e) {
  }

  if (isSlippyMapFormat == true) {
    rasterWidth = 256;
    rasterHeight = 256;
  }

  if (isSlippyMapFormat == false) {

    if (pngRaster != NULL) {
      delete pngRaster;
      CDBWarning("PNGRaster was already defined");
    }
    pngRaster = CReadPNG_read_png_file(this->fileName.c_str(), true);
    if (pngRaster == NULL) {
      CDBError("Unable to open PNG check logs");
      return 1;
    }
    rasterWidth = pngRaster->width;
    rasterHeight = pngRaster->height;

    /* Put in headers from PNG */
    double bbox[] = {0, 0, 0, 0};
    for (size_t j = 0; j < pngRaster->headers.size(); j++) {
      if (CCDFPNGIO_DEBUG) {
        CDBDebug("HEADERS [%s]=[%s]", pngRaster->headers[j].key.c_str(), pngRaster->headers[j].value.c_str());
      }
      /* Proj4 params */
      if (pngRaster->headers[j].key == "proj4_params") {
        CRS->setAttributeText("proj4", pngRaster->headers[j].value.c_str());
      }

      /* BBOX */
      if (pngRaster->headers[j].key == "bbox") {
        std::vector<std::string> bboxItems = CT::split(pngRaster->headers[j].value, ",");
        if (bboxItems.size() == 4) {

          bbox[0] = CT::toDouble(bboxItems[0]);
          bbox[1] = CT::toDouble(bboxItems[1]);
          bbox[2] = CT::toDouble(bboxItems[2]);
          bbox[3] = CT::toDouble(bboxItems[3]);
          CRS->setAttribute("bbox", CDF_DOUBLE, bbox, 4);
        }
      }

      /* Time dimension */
      if (pngRaster->headers[j].key == "time") {
        CDF::Dimension *timeDimension = cdfObject->getDimensionNE("time");
        if (!timeDimension) {
          timeDimension = cdfObject->addDimension(new CDF::Dimension("time", 1));
        }
        CDF::Variable *timeVariable = cdfObject->getVariableNE("time");
        if (timeVariable == NULL) {
          timeVariable = cdfObject->addVariable(new CDF::Variable("time", CDF_DOUBLE, &timeDimension, 1, true));
        }
        timeVariable->setAttributeText("units", "seconds since 1970-01-01 0:0:0");
        timeVariable->setAttributeText("standard_name", "time");
        timeVariable->setCDFReaderPointer(this);
        timeVariable->allocateData(1);
        CTime *ctime = CTime::GetCTimeInstance(timeVariable);
        if (ctime == nullptr) {
          CDBDebug(CTIME_GETINSTANCE_ERROR_MESSAGE);
          return 1;
        }
        ((double *)timeVariable->data)[0] = ctime->dateToOffset(ctime->freeDateStringToDate(pngRaster->headers[j].value.c_str()));
      }
      /* Reference time dimension */
      if (pngRaster->headers[j].key == "reference_time") {
        CDF::Dimension *referenceTimeDimension = cdfObject->getDimensionNE("forecast_reference_time");
        if (!referenceTimeDimension) {
          referenceTimeDimension = cdfObject->addDimension(new CDF::Dimension("forecast_reference_time", 1));
        }
        CDF::Variable *referenceTimeVariable = cdfObject->getVariableNE("forecast_reference_time");
        if (referenceTimeVariable == NULL) {
          referenceTimeVariable = cdfObject->addVariable(new CDF::Variable("forecast_reference_time", CDF_DOUBLE, &referenceTimeDimension, 1, true));
        }
        referenceTimeVariable->setAttributeText("units", "seconds since 1970-01-01 0:0:0");
        referenceTimeVariable->setAttributeText("standard_name", "forecast_reference_time");
        referenceTimeVariable->allocateData(1);
        CTime *ctime = CTime::GetCTimeInstance(referenceTimeVariable);
        if (ctime == nullptr) {
          CDBDebug(CTIME_GETINSTANCE_ERROR_MESSAGE);
          return 1;
        }
        ((double *)referenceTimeVariable->data)[0] = ctime->dateToOffset(ctime->freeDateStringToDate(pngRaster->headers[j].value.c_str()));
      }
    }

    /* Temporarily checking invalid metadata */
    if (bbox[0] < -5570000) {
      if (CRS->getAttributeThrows("proj4")->toString() == "+proj=geos +a=6378.169 +b=6356.584 +h=35785.831 +lat_0=0 +lon_0=0.0") {
        for (size_t j = 0; j < 4; j++) {
          bbox[j] /= 1000;
        }
        CRS->setAttribute("bbox", CDF_DOUBLE, bbox, 4);
      }
    }
  }

  CDF::Dimension *xDim = cdfObject->addDimension(new CDF::Dimension("x", rasterWidth));
  CDF::Variable *xVar = cdfObject->addVariable(new CDF::Variable(xDim->getName().c_str(), CDF_DOUBLE, &xDim, 1, true));
  CDF::Dimension *yDim = cdfObject->addDimension(new CDF::Dimension("y", rasterHeight));
  CDF::Variable *yVar = cdfObject->addVariable(new CDF::Variable(yDim->getName().c_str(), CDF_DOUBLE, &yDim, 1, true));

  if (CCDFPNGIO_DEBUG) {
    CDBDebug("Defining PNG variable");
  }
  CDF::Dimension *timeDimension = cdfObject->getDimensionNE("time");

  if (!timeDimension) {
    CDF::Dimension *varDims[] = {yDim, xDim};
    cdfObject->addVariable(new CDF::Variable("pngdata", CDF_UINT, varDims, 2, false));
  } else {
    CDF::Dimension *varDims[] = {timeDimension, yDim, xDim};
    cdfObject->addVariable(new CDF::Variable("pngdata", CDF_UINT, varDims, 3, false));
  }
  CDF::Variable *PNGData = cdfObject->getVariableThrows("pngdata");

  xVar->setCDFReaderPointer(this);
  xVar->setParentCDFObject(cdfObject);

  yVar->setCDFReaderPointer(this);
  yVar->setParentCDFObject(cdfObject);

  PNGData->setCDFReaderPointer(this);
  PNGData->setParentCDFObject(cdfObject);

  PNGData->setAttributeText("ADAGUC_BASENAME", fileBaseName.c_str());
  PNGData->setAttributeText("grid_mapping", "crs");
  PNGData->setAttributeText("standard_name", "rgba");

  if (isSlippyMapFormat == true) {
    auto parts = CT::split(this->fileName, "/");

    if (parts.size() > 3) {
      int zoom = atoi(parts[parts.size() - 3].c_str());
      int level = 17 - zoom;
      cdfObject->setAttribute("adaguctilelevel", CDF_INT, &level, 1);
    }
    CDF::Variable *CRS = cdfObject->addVariable(new CDF::Variable("crs", CDF_UINT, NULL, 0, false));
    CRS->setAttributeText("proj4", "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext  +no_defs");
  }

  if (CCDFPNGIO_DEBUG) {
    CDBDebug("Done");
  }

  return 0;
}

int CDFPNGReader::_readVariableData(CDF::Variable *var, CDFType) {

  bool isSingleImageWithCoordinates = true;

  double tilex1 = 0, tilex2 = 0, tiley1 = 0, tiley2 = 0;
  if (isSlippyMapFormat) {
    auto parts = CT::split(this->fileName, "/");

    if (parts.size() > 3) {
      int zoom = atoi(parts[parts.size() - 3].c_str());
      int tile_y = atoi(CT::split(parts[parts.size() - 1], ".")[0].c_str());
      int tile_x = atoi(parts[parts.size() - 2].c_str());
      auto bbox = getBounds(tile_x, tile_y, zoom);
      if (CCDFPNGIO_DEBUG) {
        CDBDebug("%d %d %d", tile_x, tile_y, zoom);
      }
      tilex1 = (bbox.left);
      tiley1 = (bbox.bottom);
      tilex2 = (bbox.right);
      tiley2 = (bbox.top);
    }
  }
  f8box bbox;
  bbox.left = -180;
  bbox.top = 90;
  bbox.right = 180;
  bbox.bottom = -90;

  CDF::Variable *CRS = cdfObject->getVariableNE("crs");
  if (CRS != NULL) {
    CDF::Attribute *bboxAttr = CRS->getAttributeNE("bbox");
    if (bboxAttr != NULL) {
      double bboxData[4];
      bboxAttr->getData(bboxData, 4);
      bbox.left = bboxData[0];
      bbox.top = bboxData[1];
      bbox.right = bboxData[2];
      bbox.bottom = bboxData[3];
    }
  }

  if (var->name == "x") {

    CDF::Variable *xVar = var;
    CDF::Dimension *xDim = ((CDFObject *)var->getParentCDFObject())->getDimensionThrows(var->name.c_str());

    xVar->allocateData(xDim->getSize());

    if (isSingleImageWithCoordinates) {
      for (size_t j = 0; j < xDim->getSize(); j++) {
        float r = ((float(j) + 0.5) / float(xDim->getSize())) * (bbox.right - bbox.left);
        ((double *)xVar->data)[j] = (r + bbox.left);
      }
    }
    if (isSlippyMapFormat) {
      for (size_t j = 0; j < xDim->getSize(); j++) {
        float r = (float(j) / float(xDim->getSize())) * (tilex2 - tilex1);
        ((double *)xVar->data)[j] = (r - tilex1);
      }
    }
  }

  if (var->name == "y") {

    CDF::Variable *yVar = var;
    CDF::Dimension *yDim = ((CDFObject *)var->getParentCDFObject())->getDimensionThrows(var->name.c_str());

    yVar->allocateData(yDim->getSize());
    if (isSingleImageWithCoordinates) {
      for (size_t j = 0; j < yDim->getSize(); j++) {
        float r = ((float(j) + 0.5) / float(yDim->getSize())) * (bbox.top - bbox.bottom);
        ((double *)yVar->data)[j] = (bbox.top - r);
      }
    }
    if (isSlippyMapFormat) {
      for (size_t j = 0; j < yDim->getSize(); j++) {
        float r = (float(j) / float(yDim->getSize())) * (tiley2 - tiley1);
        ((double *)yVar->data)[j] = (tiley2 - r);
      }
    }
  }
  if (var->name == "pngdata") {
    if (var->data != NULL) {
      CDBDebug("Warning: reusing pngdata variable");
    } else {
      if (pngRaster != NULL && pngRaster->data) {
        // Verbose logging:
      } else {
        if (pngRaster != NULL) {
          CDBDebug("Info: reusing pngRaster object.");
          delete pngRaster;
        }
        pngRaster = CReadPNG_read_png_file(this->fileName.c_str(), false);
      }
      var->allocateData(rasterWidth * rasterHeight);
      // Copy PNG raster data into variable as unsigned it, representing RGBA as 4 bytes.
      for (size_t y = 0; y < rasterHeight; y++) {
        for (size_t x = 0; x < rasterWidth; x++) {
          size_t j = x + y * rasterWidth;
          ((unsigned int *)var->data)[x + y * rasterWidth] =
              pngRaster->data[j * 4 + 0] + pngRaster->data[j * 4 + 1] * 256 + pngRaster->data[j * 4 + 2] * 256 * 256 + pngRaster->data[j * 4 + 3] * (256 * 256 * 256);
        }
      }
    }
  }
  return 0;
}

int CDFPNGReader::_readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *) {
  if (CCDFPNGIO_DEBUG) {
    CDBDebug("_readVariableData %s %d", var->name.c_str(), type);
  }

  size_t requestedSize = 1;

  for (size_t j = 0; j < var->dimensionlinks.size(); j++) {
    requestedSize *= count[j];
  }
  var->allocateData(requestedSize);

  if (var->name == "x" || var->name == "y") {
    CDF::Variable *dummyVar = new CDF::Variable();
    dummyVar->name = var->name;
    dummyVar->setType(type);
    dummyVar->setParentCDFObject(var->getParentCDFObject());
    this->_readVariableData(dummyVar, type);
    for (size_t j = 0; j < count[0]; j++) {
      size_t i = j + start[0];
      if (i < dummyVar->getSize()) {
        ((double *)var->data)[j] = ((double *)dummyVar->data)[i];
      }
    }

    delete dummyVar;
  }

  if (var->name == "pngdata") {
    if (pngRaster != NULL && pngRaster->data) {
      CDBDebug("Info: reusing pngdata with start/count");
    } else {
      if (pngRaster != NULL) delete pngRaster;
      pngRaster = CReadPNG_read_png_file(this->fileName.c_str(), false);
    }
    if (pngRaster == NULL) {
      CDBError("Unable to open PNG check logs");
      return 1;
    }
    for (size_t y = 0; y < count[0]; y++) {
      for (size_t x = 0; x < count[1]; x++) {
        size_t sourceX = x + start[1];
        size_t sourceY = y + start[0];
        if (sourceX < rasterWidth && sourceY < rasterHeight) {
          size_t j = (sourceX) + (sourceY)*rasterWidth;
          ((unsigned int *)var->data)[x + y * count[1]] =
              pngRaster->data[j * 4 + 0] + pngRaster->data[j * 4 + 1] * 256 + pngRaster->data[j * 4 + 2] * 256 * 256 + pngRaster->data[j * 4 + 3] * (256 * 256 * 256);
        }
      }
    }
  }
  return 0;
}

int CDFPNGReader::close() {
  if (pngRaster != nullptr) {
    delete pngRaster;
    pngRaster = nullptr;
  }
  return 0;
}

CDFPNGReader::~CDFPNGReader() { close(); }