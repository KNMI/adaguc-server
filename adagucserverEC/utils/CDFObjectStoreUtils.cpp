/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2024-08-28
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
#include "CDFObjectStoreUtils.h"
#include "CServerConfig_CPPXSD.h"
#include <iostream>
#include <string>
#include <regex>
#include "CDataSource.h"
#include <CTime.h>

int regex_match_item_return_int(std::string baseName, CT::string itemToTest) {
  std::regex rgx(itemToTest);
  std::smatch matches;
  if (std::regex_search(baseName, matches, rgx)) {
    if (matches.size() == 2) {
      return atoi(matches[1].str().c_str());
    }
  }
  return 0;
}

CDF::Variable *add_dimension_variable(CDF::Variable *sourceVariable, CT::string dimensionNameToAdd) {
  CDFObject *cdfObject = (CDFObject *)sourceVariable->getParentCDFObject();

  // Create the new dimension if not available
  CDF::Dimension *cdfDimensionToAdd = cdfObject->getDimensionNE(dimensionNameToAdd);
  if (cdfDimensionToAdd == nullptr) {
    cdfDimensionToAdd = cdfObject->addDimension(new CDF::Dimension("time", 1));
  }

  // Create the new variable if not available
  CDF::Variable *newDimensionVariable = cdfObject->getVariableNE(dimensionNameToAdd);
  if (newDimensionVariable == nullptr) {
    newDimensionVariable = cdfObject->addVariable(new CDF::Variable(dimensionNameToAdd, CDF_DOUBLE, &cdfDimensionToAdd, 1, true));
    newDimensionVariable->allocateData(1);
    newDimensionVariable->setAttributeText("units", "seconds since 1970-01-01 0:0:0");
    newDimensionVariable->setAttributeText("standard_name", dimensionNameToAdd.c_str());
    newDimensionVariable->setAttributeText("info", "added by adaguc-server add_dimension_variable routines");
  }

  // Make sure the new dimensionvariable is part of the dimensionlist of the source variable, but do not add it twice.
  if (sourceVariable->getDimensionNE(dimensionNameToAdd) == nullptr) {
    sourceVariable->dimensionlinks.insert(sourceVariable->dimensionlinks.begin(), cdfDimensionToAdd);
  }

  return newDimensionVariable;
}

void filename_regexp_extracttime(CDataSource *dataSource, CDFObject *cdfObject, CT::string fileLocationToOpen, CServerConfig::XMLE_AddDimension *addDimensionElement) {

  auto dimensionNameToAdd = addDimensionElement->attr.name;
  auto *cfgVariable = dataSource->cfgLayer->Variable[0];
  auto *layerVariable = cdfObject->getVariable(cfgVariable->value);
  auto *timeVariable = add_dimension_variable(layerVariable, dimensionNameToAdd);
  auto *ctime = CTime::GetCTimeInstance(timeVariable);
  if (ctime == nullptr) {
    throw(CTIME_GETINSTANCE_ERROR_MESSAGE);
  }

  std::string baseName = fileLocationToOpen.basename().c_str();
  int year = regex_match_item_return_int(baseName, addDimensionElement->attr.year);
  int month = regex_match_item_return_int(baseName, addDimensionElement->attr.month);
  int day = regex_match_item_return_int(baseName, addDimensionElement->attr.day);
  int hour = regex_match_item_return_int(baseName, addDimensionElement->attr.hour);
  int minute = regex_match_item_return_int(baseName, addDimensionElement->attr.minute);
  int second = regex_match_item_return_int(baseName, addDimensionElement->attr.second);

  CT::string dateString;
  dateString.print("%04d-%02d-%02dT%02d:%02d:%02dZ", year, month, day, hour, minute, second);
  timeVariable->setAttributeText("info_datestring", dateString.c_str());

  // Convert the timestring into the double value matching the units.
  ((double *)timeVariable->data)[0] = ctime->dateToOffset(ctime->freeDateStringToDate(dateString));
}

void handleAddDimension(CDataSource *dataSource, CDFObject *cdfObject, CT::string fileLocationToOpen) {
  for (auto a : dataSource->cfgLayer->AddDimension) {
    if (a->attr.type.equals(ADDDIMENSION_TYPE_REGEXP_EXTRACTTIME)) {
      filename_regexp_extracttime(dataSource, cdfObject, fileLocationToOpen, a);
    }
  }
}
