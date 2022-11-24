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

/*  https://www.eumetnet.eu/wp-content/uploads/2021/07/ODIM_H5_v2.4.pdf */
int CDFHDF5Reader::convertODIMHDF5toCF() {

  CDF::Variable *whereVar = cdfObject->getVariableNE("where");
  if (whereVar == NULL) {
    return 2;
  }

  CDBDebug("convertODIMHDF5toCF");

  /* Check for the data variable */
  CDF::Variable *dataVar = cdfObject->getVariableNE("dataset1.data1.data");
  if (dataVar == NULL) {
    CDBDebug("Looks like ODIM, but unable to find dataset1.data1.data variable");
    return 2;
  }

  /* Check for the what variable */
  CDF::Variable *whatVar = cdfObject->getVariableNE("dataset1.what");
  if (whatVar == NULL) {
    whatVar = cdfObject->getVariableNE("what");
    if (whatVar == NULL) {
      CDBDebug("Looks like ODIM, but unable to find dataset1.what variable");
      return 2;
    }
  }

  cdfObject->setAttributeText("ADAGUC_ODIM_CONVERTER", "true");

  std::map<std::string, std::string> quantityToUnits = {
      {"TH", "dBZ"}, {"TV", "dBZ"}, {"DBZH", "dBZ"}, {"DBZV", "dBZ"}, {"ZDR", "dB"}, {"UZDR", "dB"}, {"RHOHV", "-"}, {"URHOHV", "-"},
  };

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

  /* Set scale and offset */
  CDF::Attribute *offsetAttr = whatVar->getAttributeNE("offset");
  CDF::Attribute *gainAttr = whatVar->getAttributeNE("gain");
  if (offsetAttr != NULL && gainAttr != NULL) {
    float dataOffset = 0;
    float dataGain = 1;
    dataOffset = offsetAttr->getDataAt<float>(0);
    dataGain = gainAttr->getDataAt<float>(0);
    dataVar->setAttribute("scale_factor", CDF_FLOAT, &dataGain, 1);
    dataVar->setAttribute("add_offset", CDF_FLOAT, &dataOffset, 1);
  }

  /* Add units*/
  CDF::Attribute *quantityAttr = whatVar->getAttributeNE("quantity");
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
  CDF::Attribute *noDataAttr = whatVar->getAttributeNE("nodata");
  if (noDataAttr != NULL) {
    float fillValue = noDataAttr->getDataAt<float>(0);
    dataVar->setAttribute("_FillValue", CDF_FLOAT, &fillValue, 1);
  }

  /* Add standard_name*/
  CDF::Attribute *productAttr = whatVar->getAttributeNE("product");
  if (productAttr != NULL) {
    dataVar->setAttributeText("standard_name", productAttr->getDataAsString().c_str());
    dataVar->setAttributeText("long_name", productAttr->getDataAsString().c_str());
  }

  /* Handle time based on date and time from the HDF5 ODIM file */
  CDF::Attribute *startDateAttr = whatVar->getAttributeNE("startdate");
  if (startDateAttr == NULL) startDateAttr = whatVar->getAttributeNE("date");
  CDF::Attribute *startTimeAttr = whatVar->getAttributeNE("starttime");
  if (startTimeAttr == NULL) startTimeAttr = whatVar->getAttributeNE("time");
  if (startDateAttr != NULL && startTimeAttr != NULL) {
    /* Compose the timestring based on date and time from the HDF5 ODIM file */
    CT::string timeString;
    timeString.print("%sT%sZ", startDateAttr->getDataAsString().c_str(), startTimeAttr->getDataAsString().c_str());
    CDBDebug("timeString %s", timeString.c_str());

    /* Add the time dimension and timevariable */
    CDF::Dimension *timeDim = new CDF::Dimension("time", 1);
    cdfObject->addDimension(timeDim);
    CDF::Dimension *varDims[] = {timeDim};
    CDF::Variable *timeVar = new CDF::Variable(timeDim->getName().c_str(), CDF_DOUBLE, varDims, 1, true);
    cdfObject->addVariable(timeVar);
    timeVar->allocateData(1);
    timeVar->setAttributeText("standard_name", "time");
    timeVar->setAttributeText("units", "seconds since 1970-01-1");

    /* Set the offset time in the time variable */
    CTime *ctime = CTime::GetCTimeInstance(timeVar);
    ((double *)timeVar->data)[0] = ctime->dateToOffset(ctime->freeDateStringToDate(timeString.c_str()));
    CDBDebug("Time offset = %f", ((double *)timeVar->data)[0]);
  }

  /* Handle dataVar and draw it*/
  if (dataVar != NULL) {
    if (dataVar->dimensionlinks.size() >= 2) {
      double offsetX = 0;
      double offsetY = 0;

      dataVar->setAttributeText("grid_mapping", "crs");
      CDF::Variable *crs = NULL;
      crs = cdfObject->getVariableNE("crs");
      if (crs == NULL) {
        crs = new CDF::Variable("crs", CDF_CHAR, NULL, 0, false);
        cdfObject->addVariable(crs);
      }
      crs->setAttributeText("proj4_params", projectionString.c_str());

      CDF::Dimension *dimX = dataVar->dimensionlinks[1];
      CDF::Dimension *dimY = dataVar->dimensionlinks[0];

      if (dataVar->dimensionlinks.size() == 2) {
        if (cdfObject->getDimensionNE("time") != NULL) {
          dataVar->dimensionlinks.insert(dataVar->dimensionlinks.begin(), cdfObject->getDimensionNE("time"));
        }
      }

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
      offsetX = -double(dimX->length) / 2;
      for (size_t j = 0; j < dimX->length; j = j + 1) {
        double x = (offsetX + double(j)) * (xScale);
        ((double *)varX->data)[j] = x;
      }

      offsetY = -double(dimY->length) / 2;
      for (size_t j = 0; j < dimY->length; j = j + 1) {
        double y = (offsetY + double(j)) * (yScale);
        ((double *)varY->data)[(dimY->length - 1) - j] = y;
      }
    } else {
      CDBWarning("Data variable has only [%d] dimensions", dataVar->dimensionlinks.size());
    }
  }
  return 0;
}