/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Packages PNG into a NetCDF file
 * Author:   Maarten Plieger (KNMI)
 * Date:     2017-08-08
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <CReadFile.h>

// #define CCDFPNGIO_DEBUG

#include <cmath>
#include "../adagucserverEC/Types/GeoParameters.h"

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
#ifdef CCDFPNGIO_DEBUG
  CDBDebug("open [%s]", fileName);
#endif
  this->fileName = fileName;

  if (pngRaster != NULL) {
    CDBError("pngRaster already defined!");
    return 1;
  }

  cdfObject->addAttribute(new CDF::Attribute("Conventions", "CF-1.6"));
  cdfObject->addAttribute(new CDF::Attribute("history", "Metadata adjusted by ADAGUC from PNG to NetCDF-CF"));

  CT::string fileBaseName;
  const char *last = rindex(fileName, '/');
  if ((last != NULL) && (*last)) {
    fileBaseName.copy(last + 1);
  } else {
    fileBaseName.copy(fileName);
  }

  /* Now always add a CRS variable */
  CDF::Variable *CRS = cdfObject->getVariableNE("crs");
  if (CRS == NULL) {
    CRS = cdfObject->addVariable(new CDF::Variable("crs", CDF_UINT, NULL, 0, false));
  }

  try {
    CT::string infoFile = fileName;
    infoFile.concat(".info");
    CT::string infoData = CReadFile::open(infoFile.c_str());
    std::vector<CT::string> lines = infoData.split("\n");

    for (size_t l = 0; l < lines.size(); l++) {
      CDBDebug("Info file line %s", lines[l].c_str());
      if (lines[l].startsWith("proj4_params=")) {
        CT::string proj4Params = lines[l];
        proj4Params.substringSelf(13, -1);
        CDBDebug("proj4params=%s", proj4Params.c_str());
        CRS->setAttributeText("proj4", proj4Params.c_str());
      }
      if (lines[l].startsWith("bbox=")) {
        CT::string bbox = lines[l];
        bbox.substringSelf(5, -1);
        std::vector<CT::string> bboxItems = bbox.split(",");
        if (bboxItems.size() == 4) {
          double d[4];
          d[0] = bboxItems[0].trim().toDouble();
          d[1] = bboxItems[1].trim().toDouble();
          d[2] = bboxItems[2].trim().toDouble();
          d[3] = bboxItems[3].trim().toDouble();
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
#ifdef CCDFPNGIO_DEBUG
      CDBDebug("HEADERS [%s]=[%s]", pngRaster->headers[j].name.c_str(), pngRaster->headers[j].value.c_str());
#endif
      /* Proj4 params */
      if (pngRaster->headers[j].key.equals("proj4_params")) {
        CRS->setAttributeText("proj4", pngRaster->headers[j].value.c_str());
      }

      /* BBOX */
      if (pngRaster->headers[j].key.equals("bbox")) {
        std::vector<CT::string> bboxItems = pngRaster->headers[j].value.split(",");
        if (bboxItems.size() == 4) {

          bbox[0] = bboxItems[0].trim().toDouble();
          bbox[1] = bboxItems[1].trim().toDouble();
          bbox[2] = bboxItems[2].trim().toDouble();
          bbox[3] = bboxItems[3].trim().toDouble();
          CRS->setAttribute("bbox", CDF_DOUBLE, bbox, 4);
        }
      }

      /* Time dimension */
      if (pngRaster->headers[j].key.equals("time")) {
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
        timeVariable->allocateData(1);
        CTime *ctime = CTime::GetCTimeInstance(timeVariable);
        if (ctime == nullptr) {
          CDBDebug(CTIME_GETINSTANCE_ERROR_MESSAGE);
          return 1;
        }
        ((double *)timeVariable->data)[0] = ctime->dateToOffset(ctime->freeDateStringToDate(pngRaster->headers[j].value.c_str()));
      }
      /* Reference time dimension */
      if (pngRaster->headers[j].key.equals("reference_time")) {
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
      if (CRS->getAttribute("proj4")->getDataAsString().equals("+proj=geos +a=6378.169 +b=6356.584 +h=35785.831 +lat_0=0 +lon_0=0.0")) {
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

#ifdef CCDFPNGIO_DEBUG
  CDBDebug("Defining PNG variable");
#endif
  CDF::Dimension *timeDimension = cdfObject->getDimensionNE("time");

  if (!timeDimension) {
    CDF::Dimension *varDims[] = {yDim, xDim};
    cdfObject->addVariable(new CDF::Variable("pngdata", CDF_UINT, varDims, 2, false));
  } else {
    CDF::Dimension *varDims[] = {timeDimension, yDim, xDim};
    cdfObject->addVariable(new CDF::Variable("pngdata", CDF_UINT, varDims, 3, false));
  }
  CDF::Variable *PNGData = cdfObject->getVariable("pngdata");

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
    auto parts = this->fileName.split("/");

    if (parts.size() > 3) {
      int zoom = parts[parts.size() - 3].toInt();
      int level = 17 - zoom;
      cdfObject->setAttribute("adaguctilelevel", CDF_INT, &level, 1);
    }
    CDF::Variable *CRS = cdfObject->addVariable(new CDF::Variable("crs", CDF_UINT, NULL, 0, false));
    CRS->setAttributeText("proj4", "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext  +no_defs");
  }

#ifdef CCDFPNGIO_DEBUG
  CDBDebug("Done");
#endif

  return 0;
}

int CDFPNGReader::_readVariableData(CDF::Variable *var, CDFType) {

  bool isSingleImageWithCoordinates = true;

  double tilex1, tilex2, tiley1, tiley2;
  if (isSlippyMapFormat) {
    auto parts = this->fileName.split("/");

    if (parts.size() > 3) {
      int zoom = parts[parts.size() - 3].toInt();
      int tile_y = parts[parts.size() - 1].split(".")[0].toInt();
      int tile_x = parts[parts.size() - 2].toInt();
      auto bbox = getBounds(tile_x, tile_y, zoom);
#ifdef CCDFPNGIO_DEBUG
      CDBDebug("%d %d %d", tile_x, tile_y, zoom);
#endif
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

  if (var->name.equals("x")) {

    CDF::Variable *xVar = var;
    CDF::Dimension *xDim = ((CDFObject *)var->getParentCDFObject())->getDimension(var->name.c_str());

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

  if (var->name.equals("y")) {

    CDF::Variable *yVar = var;
    CDF::Dimension *yDim = ((CDFObject *)var->getParentCDFObject())->getDimension(var->name.c_str());

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
  if (var->name.equals("pngdata")) {
    if (var->data != NULL) {
      CDBDebug("Warning: reusing pngdata variable");
    } else {
      if (pngRaster != NULL && pngRaster->data) {
        // Verbose logging:
        // CDBDebug("Info: reusing pngRaster object with data.");
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
#ifdef CCDFPNGIO_DEBUG
  CDBDebug("_readVariableData %s %d", var->name.c_str(), type);
#endif

  size_t requestedSize = 1;

  for (size_t j = 0; j < var->dimensionlinks.size(); j++) {
    requestedSize *= count[j];
  }
  var->allocateData(requestedSize);

  if (var->name.equals("x") || var->name.equals("y")) {
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

  if (var->name.equals("pngdata")) {
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
    // CDBDebug("READ: %d\t%d\t%d\t%d\t%d\t%d", start[0], start[1], count[0], count[1],rasterWidth, rasterHeight);
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