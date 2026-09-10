/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2026-09-10
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
#include "json_adaguc.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"

void CDF::_dumpPrintAttributes(const char *variableName, std::vector<CDF::Attribute *> attributes, std::string &dumpString, int) {
  // print attributes:
  for (size_t a = 0; a < attributes.size(); a++) {
    CDF::Attribute *attr = attributes[a];
    if (attr->type != CDF_STRING) {
      CT::printfconcat(dumpString, "\t\t%s:%s =", variableName, attr->name.c_str());
    } else {
      CT::printfconcat(dumpString, "\t\tstring %s:%s =", variableName, attr->name.c_str());
    }

    // print data
    if (attr->type == CDF_CHAR) {
      char *data = new char[attr->length + 1];
      memcpy(data, attr->data, attr->length);
      data[attr->length] = '\0';
      CT::printfconcat(dumpString, " \"%s\"", data);
      delete[] data;
    }

    if (attr->type == CDF_BYTE)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %db", ((char *)attr->data)[n]);
    if (attr->type == CDF_UBYTE)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %uub", ((unsigned char *)attr->data)[n]);
    if (attr->type == CDF_INT)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %d", ((int *)attr->data)[n]);
    if (attr->type == CDF_UINT)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %u", ((unsigned int *)attr->data)[n]);
    if (attr->type == CDF_INT64)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %ldll", ((long *)attr->data)[n]);
    if (attr->type == CDF_UINT64)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %luul", ((unsigned long *)attr->data)[n]);
    if (attr->type == CDF_SHORT)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %d", ((short *)attr->data)[n]);
    if (attr->type == CDF_USHORT)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %u", ((unsigned short *)attr->data)[n]);
    if (attr->type == CDF_FLOAT)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %ff", ((float *)attr->data)[n]);
    if (attr->type == CDF_DOUBLE)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " %fdf", ((double *)attr->data)[n]);
    if (attr->type == CDF_STRING)
      for (size_t n = 0; n < attr->length; n++) CT::printfconcat(dumpString, " \"%s\"", ((char **)attr->data)[n]);
    dumpString += " ;\n";
  }
}

void CDF::_dump(CDF::Variable *cdfVariable, std::string &dumpString, int returnType) {
  CT::printfconcat(dumpString, "\t%s %s", CDF::getCDataTypeName(cdfVariable->getNativeType()).c_str(), cdfVariable->name.c_str());
  if (cdfVariable->dimensionlinks.size() > 0) {
    dumpString += "(";
    for (size_t i = 0; i < cdfVariable->dimensionlinks.size(); i++) {
      if (i > 0 && i < cdfVariable->dimensionlinks.size()) dumpString += ", ";
      CT::printfconcat(dumpString, "%s", cdfVariable->dimensionlinks[i]->name.c_str());
    }
    dumpString += ")";
  }
  dumpString += " ;\n";
  _dumpPrintAttributes(cdfVariable->name.c_str(), cdfVariable->attributes, dumpString, returnType);
}

std::string CDF::dump(CDFObject *cdfObject) {
  std::string d;
  _dump(cdfObject, d, CCDFDATAMODEL_DUMP_STANDARD);
  return d;
}

json convertCDFVariableToJSON(CDF::Variable *variable) {
  json variableJSON;
  json variableDimensionsJSON = json::array();
  for (size_t i = 0; i < variable->dimensionlinks.size(); i++) {
    variableDimensionsJSON.push_back(variable->dimensionlinks[i]->name.c_str());
  }
  json variableAttributesJSON = json::object();
  for (size_t i = 0; i < variable->attributes.size(); i++) {
    CDF::Attribute *attr = variable->attributes[i];
    if (attr->name != "_NCProperties") { /* The NetCDF library sometimes add their own attributes, skip those */
      variableAttributesJSON[attr->name.c_str()] = attr->toString().c_str();
    }
  }
  variableJSON["dimensions"] = variableDimensionsJSON;
  variableJSON["attributes"] = variableAttributesJSON;
  variableJSON["type"] = CDFNetCDFWriter::NCtypeConversionToString(variable->getNativeType()).c_str();
  return variableJSON;
}

std::string CDF::dumpAsJSON(CDFObject *cdfObject) {
  std::string d;
  /* List dimensions */
  json dimensionsJSON;
  for (size_t j = 0; j < cdfObject->dimensions.size(); j++) {
    json dimensionJSON;
    dimensionJSON = {{"length", cdfObject->dimensions[j]->getSize()}};
    dimensionsJSON[cdfObject->dimensions[j]->name.c_str()] = dimensionJSON;
  }
  json resultJSON;
  resultJSON["dimensions"] = dimensionsJSON;
  /* List variables */
  json variablesJSON;
  for (size_t j = 0; j < cdfObject->variables.size(); j++) {
    CDF::Variable *variable = cdfObject->variables[j];
    variablesJSON[variable->name.c_str()] = convertCDFVariableToJSON(variable);
    variablesJSON["nc_global"] = convertCDFVariableToJSON(cdfObject);
  }
  resultJSON["variables"] = variablesJSON;
  d = resultJSON.dump(2);
  return d;
}

std::string CDF::dump(CDF::Variable *cdfVariable) {
  std::string d;
  _dump(cdfVariable, d, CCDFDATAMODEL_DUMP_STANDARD);
  return d;
}

void CDF::_dump(CDFObject *cdfObject, std::string &dumpString, int returnType) {
  // print dimensions:
  dumpString = "CCDFDataModel {\ndimensions:\n";

  for (size_t j = 0; j < cdfObject->dimensions.size(); j++) {
    CT::printfconcat(dumpString, "\t%s = %d ;\n", cdfObject->dimensions[j]->name.c_str(), int(cdfObject->dimensions[j]->length));
  }
  dumpString += "variables:\n";
  for (size_t j = 0; j < cdfObject->variables.size(); j++) {
    {
      CT::printfconcat(dumpString, "\t%s %s", CDF::getCDataTypeName(cdfObject->variables[j]->getNativeType()).c_str(), cdfObject->variables[j]->name.c_str());
      if (cdfObject->variables[j]->dimensionlinks.size() > 0) {
        dumpString += "(";
        for (size_t i = 0; i < cdfObject->variables[j]->dimensionlinks.size(); i++) {
          if (i > 0 && i < cdfObject->variables[j]->dimensionlinks.size()) dumpString += ", ";
          CT::printfconcat(dumpString, "%s", cdfObject->variables[j]->dimensionlinks[i]->name.c_str());
        }
        dumpString += ")";
      }
      dumpString += " ;\n";
      // print attributes:
      _dumpPrintAttributes(cdfObject->variables[j]->name.c_str(), cdfObject->variables[j]->attributes, dumpString, returnType);
    }
  }
  // print GLOBAL attributes:
  dumpString += "\n// global attributes:\n";
  _dumpPrintAttributes("", cdfObject->attributes, dumpString, returnType);
  dumpString += "}\n";
}
