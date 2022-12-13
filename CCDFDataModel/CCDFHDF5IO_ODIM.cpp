/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2022-11-16
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

#include "CCDFHDF5IO.h"

double getAttrValueDouble(CDF::Variable *var, const char *attrName, double initialValue) {
  CDF::Attribute *attr = var->getAttributeNE(attrName);
  if (attr != NULL) {
    return attr->getDataAsString().toDouble();
  }
  return initialValue;
}

CDF::Variable *CDFHDF5Reader::getWhatVar(CDFObject *cdfObject, int datasetCounter, int dataCounter) {
  CDF::Variable *whatVar = NULL;
  CT::string whatVarName;
  /* First try "dataset%d.data%d.what" */
  whatVarName.print("dataset%d.data%d.what", datasetCounter, dataCounter);
  whatVar = cdfObject->getVariableNE(whatVarName.c_str());
  if (whatVar == NULL) {
    /* Second try "dataset%d.what" */
    whatVarName.print("dataset%d.what", datasetCounter);
    whatVar = cdfObject->getVariableNE(whatVarName.c_str());
    if (whatVar == NULL) {
      /* Finally try "what" */
      whatVarName.print("what");
      whatVar = cdfObject->getVariableNE(whatVarName.c_str());
    }
  }
  return whatVar;
}

CDF::Attribute *CDFHDF5Reader::getNestedAttribute(CDFObject *cdfObject, int datasetCounter, int dataCounter, const char *varName, const char *attrName) {
  CDF::Variable *nestedVar = NULL;
  CDF::Attribute *attr = NULL;
  CT::string nestedVarName;

  /* First try "dataset%d.data%d.what" */
  nestedVarName.print("dataset%d.data%d.%s", datasetCounter, dataCounter, varName);
  nestedVar = cdfObject->getVariableNE(nestedVarName.c_str());
  attr = (nestedVar != NULL) ? nestedVar->getAttributeNE(attrName) : NULL;

  if (attr == NULL) {
    CDBDebug("Did not find %s / %s", nestedVarName.c_str(), attrName);
    /* Second try "dataset%d.what" */
    nestedVarName.print("dataset%d.%s", datasetCounter, varName);
    nestedVar = cdfObject->getVariableNE(nestedVarName.c_str());
    attr = (nestedVar != NULL) ? nestedVar->getAttributeNE(attrName) : NULL;
    if (attr == NULL) {
      CDBDebug("Did not find %s / %s", nestedVarName.c_str(), attrName);
      /* Finally try "what" */
      nestedVarName.print("%s", varName);
      nestedVar = cdfObject->getVariableNE(nestedVarName.c_str());
      attr = (nestedVar != NULL) ? nestedVar->getAttributeNE(attrName) : NULL;
    }
  }

  if (attr == NULL) {
    CDBDebug("Did not find %s / %s", nestedVarName.c_str(), attrName);
  } else {
    CDBDebug("Found %s / %s", nestedVarName.c_str(), attrName);
  }
  return attr;
}

/*  https://www.eumetnet.eu/wp-content/uploads/2021/07/ODIM_H5_v2.4.pdf */
int CDFHDF5Reader::convertODIMHDF5toCF() {
  CDF::Variable *whereVar = cdfObject->getVariableNE("where");
  if (whereVar == NULL) {
    return 2;
  }
  std::map<std::string, std::string> quantityToUnits = {{"TH", "dBZ"}, {"TV", "dBZ"}, {"DBZH", "dBZ"}, {"DBZV", "dBZ"}, {"ZDR", "dB"}, {"UZDR", "dB"}, {"RHOHV", "-"}, {"URHOHV", "-"}, {"ACRR", "mm"}};
  // CDBDebug("convertODIMHDF5toCF");

  for (size_t datasetCounter = 1; datasetCounter < 100; datasetCounter += 1) {
    int dataCounter = 1;
    CT::string datasetId = "dataset";
    datasetId.printconcat("%d", datasetCounter);
    CT::string datasetIdDataId = datasetId + ".data1.data";

    /* Check for the data variable */
    CDF::Variable *dataVar = cdfObject->getVariableNE(datasetIdDataId.c_str());
    if (dataVar == NULL) {
      if (datasetCounter > 1) {
        return 0;
      }
      CDBDebug("Looks like ODIM, but unable to find %s variable", datasetIdDataId.c_str());
      return 2;
    }

    /* Check for the what variable */
    CDF::Variable *whatVarCheck = getWhatVar(cdfObject, datasetCounter, dataCounter);
    if (whatVarCheck == NULL) {
      CDBDebug("Looks like ODIM, but unable to find what variable for dataset %d", datasetCounter);
      return 2;
    }

    cdfObject->setAttributeText("ADAGUC_ODIM_CONVERTER", "true");

    /* Start collecting projection attributes */
    double xScale;
    double yScale;
    CT::string projectionString;

    CDF::Attribute *xScaleAttr = whereVar->getAttributeNE("xscale");
    if (xScaleAttr != NULL) {
      xScale = xScaleAttr->getDataAt<double>(0);
    }
    CDF::Attribute *yScaleAttr = whereVar->getAttributeNE("yscale");
    if (yScaleAttr != NULL) {
      yScale = yScaleAttr->getDataAt<double>(0);
    }
    CDF::Attribute *projDefAttr = whereVar->getAttributeNE("projdef");
    if (projDefAttr != NULL) {
      projectionString = projDefAttr->getDataAsString();
    }

    double cornerX[4];
    double cornerY[4];

    cornerX[0] = getAttrValueDouble(whereVar, "LL_lon", -1);
    cornerY[0] = getAttrValueDouble(whereVar, "LL_lat", -1);
    cornerX[1] = getAttrValueDouble(whereVar, "LR_lon", -1);
    cornerY[1] = getAttrValueDouble(whereVar, "LR_lat", -1);
    cornerX[2] = getAttrValueDouble(whereVar, "UL_lon", -1);
    cornerY[2] = getAttrValueDouble(whereVar, "UL_lat", -1);
    cornerX[3] = getAttrValueDouble(whereVar, "UR_lon", -1);
    cornerY[3] = getAttrValueDouble(whereVar, "UR_lat", -1);

    projCtx proj4Context = pj_ctx_alloc();
    projPJ sourcepj = pj_init_plus_ctx(proj4Context, projectionString.c_str());
    projPJ latlonpj = pj_init_plus_ctx(proj4Context, "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
    for (size_t j = 0; j < 4; j += 1) {
      cornerX[j] *= DEG_TO_RAD;
      cornerY[j] *= DEG_TO_RAD;
    }
    pj_transform(latlonpj, sourcepj, 4, 0, cornerX, cornerY, NULL);

    pj_free(sourcepj);
    pj_free(latlonpj);
    pj_ctx_free(proj4Context);

    /* Set scale and offset */
    CDF::Attribute *offsetAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "offset");
    CDF::Attribute *gainAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "gain");
    if (offsetAttr != NULL && gainAttr != NULL) {
      float dataOffset = 0;
      float dataGain = 1;
      dataOffset = offsetAttr->getDataAt<float>(0);
      dataGain = gainAttr->getDataAt<float>(0);
      dataVar->setAttribute("scale_factor", CDF_FLOAT, &dataGain, 1);
      dataVar->setAttribute("add_offset", CDF_FLOAT, &dataOffset, 1);
    }

    /* Add units*/
    CDF::Attribute *quantityAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "quantity");
    if (quantityAttr != NULL) {
      /* Try to find the units based on the quantity, otherwise forward the quantity. */
      auto result = quantityToUnits.find(quantityAttr->getDataAsString().toUpperCase().c_str());
      if (result == quantityToUnits.end()) {
        dataVar->setAttributeText("units", quantityAttr->getDataAsString().c_str());
      } else {
        dataVar->setAttributeText("units", result->second.c_str());
      }
    }

    /* Add nodata*/
    CDF::Attribute *noDataAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "nodata");
    if (noDataAttr != NULL) {
      float fillValue = noDataAttr->getDataAt<float>(0);
      dataVar->setAttribute("_FillValue", CDF_FLOAT, &fillValue, 1);
    }

    /* Add standard_name*/
    CDF::Attribute *productAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "product");
    if (productAttr != NULL) {
      dataVar->setAttributeText("standard_name", productAttr->getDataAsString().c_str());
      dataVar->setAttributeText("long_name", productAttr->getDataAsString().c_str());
    }

    if (datasetCounter == 1) {
      /* Handle time based on date and time from the HDF5 ODIM file */
      CDF::Attribute *startDateAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "startdate");
      if (startDateAttr == NULL) startDateAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "date");
      CDF::Attribute *startTimeAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "starttime");
      if (startTimeAttr == NULL) startTimeAttr = getNestedAttribute(cdfObject, datasetCounter, dataCounter, "what", "time");
      if (startDateAttr != NULL && startTimeAttr != NULL) {
        /* Compose the timestring based on date and time from the HDF5 ODIM file */
        CT::string timeString;
        timeString.print("%sT%sZ", startDateAttr->getDataAsString().c_str(), startTimeAttr->getDataAsString().c_str());
        // CDBDebug("timeString %s", timeString.c_str());

        /* Add the time dimension and timevariable */
        CDF::Dimension *timeDim = new CDF::Dimension("time", 1);
        cdfObject->addDimension(timeDim);
        CDF::Dimension *varDims[] = {timeDim};
        CDF::Variable *timeVar = new CDF::Variable(timeDim->getName().c_str(), CDF_DOUBLE, varDims, 1, true);
        cdfObject->addVariable(timeVar);
        timeVar->allocateData(1);
        timeVar->setAttributeText("standard_name", "time");
        timeVar->setAttributeText("units", "seconds since 1970-01-01");

        /* Set the offset time in the time variable */
        CTime *ctime = CTime::GetCTimeInstance(timeVar);
        ((double *)timeVar->data)[0] = ctime->dateToOffset(ctime->freeDateStringToDate(timeString.c_str()));
        // CDBDebug("Time offset = %f", ((double *)timeVar->data)[0]);
      }

      CDF::Dimension *dimX = dataVar->dimensionlinks[1];
      CDF::Dimension *dimY = dataVar->dimensionlinks[0];

      CDF::Variable *varX = cdfObject->getVariableNE(dimX->name.c_str());
      CDF::Variable *varY = cdfObject->getVariableNE(dimY->name.c_str());
      varX->setName("x");
      varY->setName("y");
      dimX->setName("x");
      dimY->setName("y");
      if (CDF::allocateData(varX->currentType, &varX->data, dimX->length)) {
        throw(__LINE__);
      }
      if (CDF::allocateData(varY->currentType, &varY->data, dimY->length)) {
        throw(__LINE__);
      };

      // CDBDebug("Metadata xScale %f, Calculated yScale: %f", xScale, yScale);
      double offsetX = cornerX[0]; //-double(dimX->length) / 2;
      xScale = (((cornerX[1] - cornerX[0]) + (cornerX[3] - cornerX[2])) / 2) / double(dimX->length);

      // CDBDebug("Calculated xScale %f, Calculated yScale: %f", xScale, yScale);
      for (size_t j = 0; j < dimX->length; j = j + 1) {
        double x = (double(j)) * (xScale);
        ((double *)varX->data)[j] = x + offsetX + xScale / 2;
      }

      double offsetY = cornerY[0]; //-double(dimY->length) / 2;
      yScale = (((cornerY[2] - cornerY[0]) + (cornerY[3] - cornerY[1])) / 2) / double(dimY->length);
      for (size_t j = 0; j < dimY->length; j = j + 1) {
        double y = (+double(j)) * (yScale);
        ((double *)varY->data)[(dimY->length - 1) - j] = y + offsetY + yScale / 2;
      }
    }

    /* Handle dataVar and draw it*/
    if (dataVar != NULL) {
      if (dataVar->dimensionlinks.size() >= 2) {

        dataVar->setAttributeText("grid_mapping", "crs");
        CDF::Variable *crs = NULL;
        crs = cdfObject->getVariableNE("crs");
        if (crs == NULL) {
          crs = new CDF::Variable("crs", CDF_CHAR, NULL, 0, false);
          cdfObject->addVariable(crs);
        }
        crs->setAttributeText("proj4_params", projectionString.c_str());

        if (dataVar->dimensionlinks.size() == 2) {
          if (cdfObject->getDimensionNE("time") != NULL) {
            dataVar->dimensionlinks.insert(dataVar->dimensionlinks.begin(), cdfObject->getDimensionNE("time"));
          }
        }

      } else {
        CDBWarning("Data variable has only [%d] dimensions", dataVar->dimensionlinks.size());
      }
    }
  }
  return 0;
}