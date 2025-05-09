/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2020-12-09
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

#include "CImgWarpGeneric.h"
#include "CImageDataWriter.h"
#include "GenericDataWarper/CGenericDataWarper.h"
#include "CImageOperators/smoothRasterField.h"
#include "GenericDataWarper/gdwDrawFunction.h"
#include <CImageOperators/drawContour.h>

const char *CImgWarpGeneric::className = "CImgWarpGeneric";

void CImgWarpGeneric::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  GDWDrawFunctionSettings settings = getDrawFunctionSettings(dataSource, drawImage, styleConfiguration);
  CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();
  void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;
  CGeoParams sourceGeo(dataSource);

  settings.destinationGrid = new double[drawImage->Geo->dWidth * drawImage->Geo->dHeight];

  // In case of contourlines and bilinear
  if (settings.drawInImage == DrawInImageNearest) {
    settings.useHalfCellOffset = false;
  } else {
    settings.useHalfCellOffset = true;
  }

  for (size_t j = 0; j < size_t(drawImage->Geo->dWidth * drawImage->Geo->dHeight); j += 1) {
    ((double *)settings.destinationGrid)[j] = NAN;
  }
  settings.setValueInDestinationFunction = gdwDrawFunction;
  warp(warper, sourceData, dataType, &sourceGeo, drawImage->Geo, &settings);

  drawContour(((double *)settings.destinationGrid), dataSource, drawImage);
  auto a = CColor(0, 0, 0, 255);
  auto b = CColor(100, 100, 255, 255);
  CT::string text;
  text.print("GENERIC %d", settings.drawInImage);
  drawImage->setTextStroke(50, 80, -0.5, text.c_str(), NULL, 20, 2, a, b);
  delete[] ((double *)settings.destinationGrid);
  settings.destinationGrid = nullptr;
  return;
}

int CImgWarpGeneric::set(const char *) { return 0; }
