/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
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

#include "CCDFNetCDFIO.h"
#include "CStopWatch.h"
#include "CDFCopyData.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include "traceTimings/traceTimings.h"

#define CDFNetCDFGroupSeparator "/"

static const bool CCDFNETCDFIO_DEBUG = false;
static const bool CCDFNETCDFIO_DEBUG_OPEN = false;
static const bool CCDFNETCDFWRITER_DEBUG = false;

CDFNetCDFReader::CDFNetCDFReader() : CDFReader() {
  if (CCDFNETCDFIO_DEBUG) {
    StopWatch_Stop("New CDFNetCDFReader");
  }
  root_id = -1;
  keepFileOpen = false;
}
CDFNetCDFReader::~CDFNetCDFReader() { close(); }

int CDFNetCDFReader::cdfReadVariableData(CDF::Variable *var, CDFType type) { return cdfReadVariableData(var, type, NULL, NULL, NULL); }

// Re-opens the file (closed since the last read, e.g. because the CDFObjectStore evicted it) and
// re-resolves var's id, since a fresh nc_open assigns ids independently of what was cached on var.
int CDFNetCDFReader::_netcdfReOpen(CDF::Variable *var) {
  CDBDebug("NC_OPEN re-opening %s for %s", fileName.c_str(), var->name.c_str());
  traceTimingsSpanStart(TraceTimingType::FSNCOPEN);
  status = nc_open(fileName.c_str(), NC_NOWRITE, &root_id);
  traceTimingsSpanEnd(TraceTimingType::FSNCOPEN);
  if (status != NC_NOERR) {
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_open: ", status);
    return 1;
  }

  if (CCDFNETCDFIO_DEBUG_OPEN) {
    CDBDebug("root_id %d", root_id);
    var->id = -1;
    CDBDebug("VARNAME %s id: %d", var->name.c_str(), var->id);
  }

  /*Check if var id is still OK*/
  const int groupId = _findNCGroupIdForCDFVariable(var->name);
  if (groupId == -1) {
    CDBError("_findNCGroupIdForCDFVariable for %s = -1", var->name.c_str());
    return 1;
  }
  // var->name may be prefixed with a group path (e.g. "grp/subgrp/varname"); nc_inq_varid wants the local name within groupId.
  std::string localVarName = var->name;
  auto slashPos = localVarName.find_last_of(CDFNetCDFGroupSeparator);
  if (slashPos != std::string::npos) {
    localVarName = localVarName.substr(slashPos + 1);
  }
  // Look up the variable id directly instead of linearly scanning (and nc_inq_var-ing) every variable in the file.
  status = nc_inq_varid(groupId, localVarName.c_str(), &var->id);
  if (status != NC_NOERR) {
    CDBError("[%s]: %s %d for variable %s", nc_strerror(status), "nc_inq_varid: ", status, var->name.c_str());
    return 1;
  }
  return 0;
}

int CDFNetCDFReader::_readStringVariableData(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride, int varGroupId, bool useStartCount, bool useStriding) {
  if (var->isString()) {
    /*Mimic char(numString,maxStrlen64) behaviour to CDF_STRING */
    const size_t stringSize = var->dimensionlinks[1]->getSize(); // e.g. maxStrlen64;
    size_t numStrings = var->dimensionlinks[0]->getSize();       // The number of strings
    int step = 1;
    int beginAt = 0;
    if (count != NULL) numStrings = count[0];
    if (stride != NULL) step = stride[0];
    if (start != NULL) beginAt = start[0];
    char *tempData = new char[numStrings * stringSize];
    status = nc_get_var(varGroupId, var->id, tempData);
    if (status != NC_NOERR) {
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_var_char (for CDF_STRING): ", status);
    }
    for (size_t j = 0; j < numStrings; j = j + step) {
      char *stringToAdd = tempData + (j + beginAt) * stringSize;
      const size_t length = strlen(stringToAdd);
      ((char **)var->data)[j] = (char *)malloc(length + 1);
      snprintf(((char **)var->data)[j], length + 1, "%s", stringToAdd);
    }
    delete[] tempData;
  } else {
    if (useStartCount == true) {
      if (useStriding) {
        if (CCDFNETCDFIO_DEBUG_OPEN) {
          CDBDebug("READ NSCS: [%s]", var->name.c_str());
        }
        status = nc_get_vars_string(varGroupId, var->id, start, count, stride, (char **)var->data);
        if (status != NC_NOERR) {
          CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_vars (typeconversion): ", status);
        }
      } else {
        if (CCDFNETCDFIO_DEBUG_OPEN) {
          CDBDebug("READ NSC: [%s]", var->name.c_str());
        }
        status = nc_get_vara_string(varGroupId, var->id, start, count, (char **)var->data);
        if (status != NC_NOERR) {
          CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_vara (typeconversion): ", status);
        }
      }
    } else {
      if (CCDFNETCDFIO_DEBUG_OPEN) {
        CDBDebug("READ N: [%s]", var->name.c_str());
      }
      status = nc_get_var_string(varGroupId, var->id, (char **)var->data);
      if (status != NC_NOERR) {
        CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_var_string (typeconversion): ", status);
      }
    }
  }
  if (status != NC_NOERR) {
    CDBError("Problem with variable %s of type %s (requested %s):", var->name.c_str(), CDF::getCDFDataTypeName(var->currentType).c_str(), CDF::getCDFDataTypeName(CDF_STRING).c_str());
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_var: ", status);
    return 1;
  }

  if (adagucMeasureTime) {
    StopWatch_Stop("<CDFNetCDFReader::cdfReadVariableData");
  }
  return 0;
}

int CDFNetCDFReader::cdfReadVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride) {
  if (adagucMeasureTime) {
    StopWatch_Stop(">CDFNetCDFReader::cdfReadVariableData");
  }
  // CDBDebug("CDFNetCDFReader::cdfReadVariableData read from [%s]", fileName.c_str());
  if (root_id == -1) {
    CDBDebug("reopen");
    if (_netcdfReOpen(var) != 0) {
      return 1;
    }
  }

  if (CCDFNETCDFIO_DEBUG) {
    CDBDebug("reading %s with id %d from file %s", var->name.c_str(), var->id, fileName.c_str());
  }
  const int varGroupId = _findNCGroupIdForCDFVariable(var->name);
  if (varGroupId == -1) {
    CDBError("_findNCGroupIdForCDFVariable for %s = -1", var->name.c_str());
    return 1;
  }

  bool useStartCount = (start != NULL && count != NULL && stride != NULL);
  bool useStriding = false;

  size_t totalVariableSize = 1;

  if (useStartCount == false) {
    for (auto *dimensionlink: var->dimensionlinks) {
      totalVariableSize *= dimensionlink->length;
    }
  } else {
    for (size_t i = 0; i < var->dimensionlinks.size(); i++) {
      totalVariableSize *= count[i]; // stride[i];
      if (count[i] + start[i] > var->dimensionlinks[i]->getSize()) {
        CDBError("Start and count are out of bounds for dim nr [%lu]): dim [%s]: start[%lu], count[%lu], dimensionlinks size[%lu]", i, var->dimensionlinks[i]->name.c_str(), start[i], count[i],
                 var->dimensionlinks[i]->getSize());
        return 1;
      }
      if (stride[i] != 1) {
        useStriding = true;
      }
      if (CCDFNETCDFIO_DEBUG) {
        CDBDebug("%s: [%zu %zu %td]", var->dimensionlinks[i]->name.c_str(), start[i], count[i], stride[i]);
      }
    }
  }

  if (CCDFNETCDFIO_DEBUG) {
    CDBDebug("Allocating data for variable %s, type: %s, size: %zu", var->name.c_str(), CDF::getCDFDataTypeName(var->currentType).c_str(), totalVariableSize);
  }
  var->setType(type);
  var->allocateData(totalVariableSize);

  if (type == CDF_STRING) {
    return _readStringVariableData(var, start, count, stride, varGroupId, useStartCount, useStriding);
  }

  // Data is requested with another type than requested. We will perform type conversion in the following piece of code.
  if (type != var->nativeType) {
    if (_readVariableRequestedType(var, type, start, count, stride, varGroupId, useStartCount, useStriding) != 0) {
      return 1;
    }
  }

  if (type == var->nativeType) {
    if (_readVariableNativeType(var, type, start, count, stride, varGroupId, useStartCount, useStriding) != 0) {
      return 1;
    }
  }

  if (CCDFNETCDFIO_DEBUG) {
    CDBDebug("Ready.");
  }
  if (adagucMeasureTime) {
    StopWatch_Stop("<CDFNetCDFReader::cdfReadVariableData");
  }
  return 0;
}

// Data is requested with another type than the variable's native type. Performs type conversion via a temporary buffer.
int CDFNetCDFReader::_readVariableRequestedType(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride, int varGroupId, bool useStartCount, bool useStriding) {
  // Temporal buffer for reading netcdf variable
  void *voidData = NULL;
  CDF::allocateData(var->nativeType, &voidData, var->getSize());

  if (useStartCount == true) {
    if (useStriding) {
      if (CCDFNETCDFIO_DEBUG_OPEN) {
        CDBDebug("READ SCS: [%s]", var->name.c_str());
      }
      status = nc_get_vars(varGroupId, var->id, start, count, stride, voidData);
      if (status != NC_NOERR) {
        CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_vars (typeconversion): ", status);
      }
    } else {
      if (CCDFNETCDFIO_DEBUG_OPEN) {
        CDBDebug("READ SC: [%s]", var->name.c_str());
      }
      status = nc_get_vara(varGroupId, var->id, start, count, voidData);
      if (status != NC_NOERR) {
        CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_vara (typeconversion): ", status);
      }
    }
  } else {
    if (CCDFNETCDFIO_DEBUG_OPEN) {
      CDBDebug("READ: [%s]", var->name.c_str());
    }

    status = nc_get_var(varGroupId, var->id, voidData);
    if (status != NC_NOERR) {
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_var (typeconversion): ", status);
    }
  }

  if (status != NC_NOERR) {
    CDBError("Problem with variable %s of type %s (requested %s):", var->name.c_str(), CDF::getCDFDataTypeName(var->currentType).c_str(), CDF::getCDFDataTypeName(type).c_str());
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_var: ", status);
    return 1;
  }

  if (CCDFNETCDFIO_DEBUG) {
    CDBDebug("Copying %zu elements from type %s to %s", var->getSize(), CDF::getCDFDataTypeName(var->nativeType).c_str(), CDF::getCDFDataTypeName(type).c_str());
  }

  CDFCopyData(var->data, type, voidData, var->nativeType, 0, 0, var->getSize());

  if (CCDFNETCDFIO_DEBUG) {
    CDBDebug("Freeing temporary data object");
  }
  CDF::freeData(&voidData);

  // End of reading data and performing type conversion
  return 0;
}

// Data is requested with the variable's native type, so it can be read directly into var->data without conversion.
int CDFNetCDFReader::_readVariableNativeType(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride, int varGroupId, bool useStartCount, bool useStriding) {
  if (useStartCount) {
    if (useStriding) {
      if (CCDFNETCDFIO_DEBUG_OPEN) {
        std::string dims = "";
        for (size_t j = 0; j < var->dimensionlinks.size(); j++) {
          if (j > 0) dims += ",";
          CT::printfconcat(dims, "%s[%zu:%zu:%zu]", var->dimensionlinks[j]->name.c_str(), start[j], count[j], start[j]);
        }
        CDBDebug("READ NSCS: [%s](%s)", var->name.c_str(), dims.c_str());
      }
      status = nc_get_vars(varGroupId, var->id, start, count, stride, var->data);
      if (status != NC_NOERR) {
        CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_vars (native): ", status);
      }
    } else {
      if (CCDFNETCDFIO_DEBUG_OPEN) {
        std::string dims = "";
        for (size_t j = 0; j < var->dimensionlinks.size(); j++) {
          if (j > 0) dims += ",";
          CT::printfconcat(dims, "%s[%zu:%zu]", var->dimensionlinks[j]->name.c_str(), start[j], count[j]);
        }
        CDBDebug("READ NSC: [%s](%s)", var->name.c_str(), dims.c_str());
      }
      status = nc_get_vara(varGroupId, var->id, start, count, var->data);
      if (status != NC_NOERR) {
        CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_vara (native): ", status);
      }
    }
  } else {
    if (CCDFNETCDFIO_DEBUG_OPEN) {
      CDBDebug("READ N: [%s]", var->name.c_str());
    }
    status = nc_get_var(varGroupId, var->id, var->data);
    if (status != NC_NOERR) {
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_var (native): ", status);
    }
  }
  if (status != NC_NOERR) {
    CDBError("Problem with variable %s of type %s (requested %s):", var->name.c_str(), CDF::getCDFDataTypeName(var->currentType).c_str(), CDF::getCDFDataTypeName(type).c_str());
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_var: ", status);
    return 1;
  }
  // End of reading data natively.
  return 0;
}

int CDFNetCDFReader::_readDimensions(int groupId, std::string &groupName) {
  int nDims;

  // Obtain number of dimensions for this groupId
  status = nc_inq_ndims(groupId, &nDims);
  if (status != NC_NOERR) {
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_ndims: ", status);
    return 1;
  }

  // Obtain the actual ids and put them in a vector
  std::vector<int> dimIds(nDims);
  status = nc_inq_dimids(groupId, &nDims, dimIds.data(), 0);
  if (status != NC_NOERR) {
    CDBError("For groupName %s: ", groupName.c_str());
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_dimids: ", status);
    return 1;
  }

  // Now inquire per dim
  for (const int dimId: dimIds) {
    char flatname[NC_MAX_NAME + 1];
    size_t length;
    status = nc_inq_dim(groupId, dimId, flatname, &length);

    if (status != NC_NOERR) {
      CDBError("For groupName %s: ", groupName.c_str());
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_dim: ", status);
      return 1;
    }

    const std::string name = groupName + flatname;

    CDF::Dimension *existingDim = cdfObject->getDim(name);
    if (existingDim != nullptr) {
      // Only add non existing variables;
      CDBWarning("Reassigning dim %s", name.c_str());
      if (existingDim->length != length) {
        CDBError("Previously dimensions size for dim %s is not the same as new definition", name.c_str());
        return 1;
      }
    } else {
      CDF::Dimension *dim = new CDF::Dimension();
      dim->id = dimId;
      dim->setName(name);
      dim->length = length;
      cdfObject->dimensions.push_back(dim);
    }
  }
  return 0;
}

int CDFNetCDFReader::_readAttributes(int root_id, std::vector<CDF::Attribute *> &attributes, int varID, int natt) {
  char name[NC_MAX_NAME + 1];
  nc_type type;
  size_t length;
  for (int i = 0; i < natt; i++) {
    status = nc_inq_attname(root_id, varID, i, name);
    if (status != NC_NOERR) {
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_attname: ", status);
      return 1;
    }
    // Only add non existing attributes;
    const bool attributeExists = std::find_if(attributes.begin(), attributes.end(), [&name](const CDF::Attribute *attribute) { return attribute->name == name; }) != attributes.end();
    if (attributeExists == false) {
      status = nc_inq_att(root_id, varID, name, &type, &length);
      if (status != NC_NOERR) {
        CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_att: ", status);
        return 1;
      }
      CDF::Attribute *attr = new CDF::Attribute();
      attr->setName(name);
      attr->type = _typeConversionAtt(type);
      if (attr->type == CDF_UNKNOWN) {
        CDBWarning("Unknown attribute type for attribute %s %d %d %lu", name, type, attr->type, attr->length);
      }
      attr->length = length;
      CDF::allocateData(attr->getType(), &attr->data, attr->length + 1);
      if (type != NC_STRING) {
        status = nc_get_att(root_id, varID, name, attr->data);
        if (type == NC_CHAR) ((char *)attr->data)[attr->length] = '\0';
        if (status != NC_NOERR) {
          CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_att: ", status);
          return 1;
        }
      } else {
        status = nc_get_att_string(root_id, varID, name, (char **)attr->data);
        if (status != NC_NOERR) {
          CDBError("[%s]: %s %d", nc_strerror(status), "nc_get_att: ", status);
          return 1;
        }
      }
      attributes.push_back(attr);
    }
  }
  return 0;
}

int CDFNetCDFReader::_findNCGroupIdForCDFVariable(const std::string &varName) {
  // Most files have a flat variable namespace (no NetCDF4 groups), so avoid the
  // vector<string> allocation from CT::split for the common case where there's nothing to split.
  if (varName.find(CDFNetCDFGroupSeparator) == std::string::npos) {
    return root_id;
  }
  auto paths = CT::split(varName, CDFNetCDFGroupSeparator);
  if (paths.size() <= 1) {
    return root_id;
  }
  int currentId = root_id;
  for (size_t j = 0; j < paths.size() - 1; j++) {
    int grp_ncid;
    status = nc_inq_ncid(currentId, paths[j].c_str(), &grp_ncid);
    if (status != NC_NOERR) {
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_ncid: ", status);
      return -1;
    }
    currentId = grp_ncid;
  }
  return currentId;
};

int CDFNetCDFReader::_readVariables(int groupId, std::string &groupName, int mode) {
  int nGroups;
  status = nc_inq_grps(groupId, &nGroups, NULL);
  if (status != NC_NOERR) {
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_grpname_full: ", status);
    return 1;
  }

  if (nGroups > 0) {
    int *groupIds = new int[nGroups];
    status = nc_inq_grps(groupId, NULL, groupIds);
    if (status != NC_NOERR) {
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_grps: ", status);
      delete[] groupIds;
      return 1;
    }

    for (int g = 0; g < nGroups; g++) {
      char foundGroupName[NC_MAX_NAME + 1];
      status = nc_inq_grpname(groupIds[g], foundGroupName);
      if (status != NC_NOERR) {
        CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_grpname: ", status);
        delete[] groupIds;
        return 1;
      }
      std::string newGroupName;
      if (groupName.length() > 0) {
        newGroupName = CT::printf("%s%s%s", groupName.c_str(), foundGroupName, CDFNetCDFGroupSeparator);
      } else {
        newGroupName = CT::printf("%s%s", foundGroupName, CDFNetCDFGroupSeparator);
      }
      status = _readVariables(groupIds[g], newGroupName, mode);
      if (status != 0) {
        CDBError("_readVariables failed for group [%s]", newGroupName.c_str());
        delete[] groupIds;
        return 1;
      }
    }

    delete[] groupIds;
  }
  if (CCDFNETCDFWRITER_DEBUG) {
    CDBDebug("Start reading group [%s] with id [%d]", groupName.c_str(), groupId);
  }

  if (mode == 0) {
    status = _readDimensions(groupId, groupName);
    if (status != 0) return 1;
    if (status != 0) {
      CDBError("_readDimensions failed for group [%s]", groupName.c_str());
      return 1;
    }
    if (adagucMeasureTime) {
      StopWatch_Stop("readDim");
    }
    return 0;
  }

  char flatname[NC_MAX_NAME + 1];
  nc_type type;
  int ndims;
  int natt;
  int dimids[NC_MAX_VAR_DIMS];
  bool isDimension;
  int nVars;
  status = nc_inq_nvars(groupId, &nVars);
  if (status != NC_NOERR) {
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_nvars: ", status);
    return 1;
  }

  // Dimensions are fully read before this (mode==1) pass starts, and don't change during it,
  // so this lookup can be built once here instead of linearly scanning cdfObject->dimensions
  // by id for every variable below (which, for files with many variables, made opening them O(n^2)).
  std::unordered_map<int, CDF::Dimension *> dimensionsById;
  for (auto *dimension: cdfObject->dimensions) {
    dimensionsById[dimension->id] = dimension;
  }

  for (int j = 0; j < nVars; j++) {
    status = nc_inq_var(groupId, j, flatname, &type, &ndims, dimids, &natt);
    if (status != NC_NOERR) {
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq_var: ", status);
      return 1;
    }

    const std::string name = groupName + flatname;

    // Only add non existing variables...
    if (cdfObject->getVar(name) == nullptr) {
      CDF::Variable *var = new CDF::Variable();
      cdfObject->variables.push_back(var);
      isDimension = cdfObject->getDim(name) != nullptr;

      // Dimension links:
      for (int k = 0; k < ndims; k++) {
        auto dimIt = dimensionsById.find(dimids[k]);
        if (dimIt == dimensionsById.end()) {
          CDBError("For variable [%s] unable to find dimensionwith id %d: Nr of needed dims: %d, numdims found in "
                   "cdfobject: %lu",
                   name.c_str(), dimids[k], ndims, cdfObject->dimensions.size());
          for (size_t dimIndex = 0; dimIndex < cdfObject->dimensions.size(); dimIndex++) {
            CDBDebug("%lu: %s with id %d", dimIndex, cdfObject->dimensions[dimIndex]->name.c_str(), cdfObject->dimensions[dimIndex]->id);
          }
          return 1;
        }
        var->dimensionlinks.push_back(dimIt->second);
      }

      // Attributes:
      status = _readAttributes(groupId, var->attributes, j, natt);
      if (status != 0) return 1;

      // Check for signed/unsigned status via opendap
      bool varIsUnsigned = false;
      CDF::Attribute *_Unsigned = var->getAttributeNE("_Unsigned");
      if (_Unsigned != NULL) {
        if (_Unsigned->toString() == "true") {
          varIsUnsigned = true;
        }
      }

      // Variable type
      nc_type thisType = _typeConversionVar(type, varIsUnsigned);

      if (varIsUnsigned) {
        CDF::Attribute *FillValue = var->getAttributeNE("_FillValue");
        if (FillValue != NULL) {
          FillValue->type = thisType;
        }
      }

      if (thisType == CDF_CHAR && var->dimensionlinks.size() == 2) {
        if (var->dimensionlinks[1]->name == "maxStrlen64") {
          var->dimensionlinks.pop_back();
          thisType = CDF_STRING;
          var->isString(true);
        }
      }

      var->setType(thisType);
      var->nativeType = thisType;
      var->setName(name);
      var->id = j;
      var->setParentCDFObject(cdfObject);
      var->isDimension = isDimension;

      // It is essential that the variable knows which reader can be used to read the data
      var->setCDFReaderPointer((void *)this);
    }
  }
  return 0;
}

CDFType CDFNetCDFReader::_typeConversionAtt(nc_type type) {
  switch (type) {
  case NC_BYTE:
    return CDF_BYTE;
  case NC_UBYTE:
    return CDF_UBYTE;
  case NC_CHAR:
    return CDF_CHAR;
  case NC_SHORT:
    return CDF_SHORT;
  case NC_USHORT:
    return CDF_USHORT;
  case NC_INT:
    return CDF_INT;
  case NC_UINT:
    return CDF_UINT;
  case NC_INT64:
    return CDF_INT64;
  case NC_UINT64:
    return CDF_UINT64;
  case NC_FLOAT:
    return CDF_FLOAT;
  case NC_DOUBLE:
    return CDF_DOUBLE;
  case NC_STRING:
    return CDF_STRING;
  default:
    CDBWarning("Warning unknown attribute type %d", type);
    return CDF_UNKNOWN;
  }
}

CDFType CDFNetCDFReader::_typeConversionVar(nc_type type, bool isUnsigned) {
  switch (type) {
  case NC_BYTE:
    return isUnsigned ? CDF_UBYTE : CDF_BYTE;
  case NC_UBYTE:
    return CDF_UBYTE;
  case NC_CHAR:
    return isUnsigned ? CDF_UBYTE : CDF_CHAR;
  case NC_SHORT:
    return isUnsigned ? CDF_USHORT : CDF_SHORT;
  case NC_USHORT:
    return CDF_USHORT;
  case NC_INT:
    return isUnsigned ? CDF_UINT : CDF_INT;
  case NC_UINT:
    return CDF_UINT;
  case NC_INT64:
    return isUnsigned ? CDF_UINT64 : CDF_INT64;
  case NC_UINT64:
    return CDF_UINT64;
  case NC_FLOAT:
    return CDF_FLOAT;
  case NC_DOUBLE:
    return CDF_DOUBLE;
  case NC_STRING:
    return CDF_STRING;
  default:
    return CDF_UNKNOWN;
  }
}

int CDFNetCDFReader::open(const char *fileName) {

  if (cdfObject == NULL) {
    CDBError("No CDFObject defined, use CDFObject::attachCDFReader(CDFNetCDFReader*). Please note that this function "
             "should be called by CDFObject open routines.");
    return 1;
  }
  this->fileName = fileName;

  if (CCDFNETCDFIO_DEBUG_OPEN) {
    StopWatch_Stop("NC_OPEN opening %s", fileName);
  }

  traceTimingsSpanStart(TraceTimingType::FSNCOPEN);
  status = nc_open(fileName, NC_NOWRITE, &root_id);
  traceTimingsSpanEnd(TraceTimingType::FSNCOPEN);
  if (status != NC_NOERR) {
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_open: ", status);
    return 1;
  }

  if (adagucMeasureTime) {
    StopWatch_Stop("CDFNetCDFReader open file\n");
  }
  int nDims, nVars, nRootAttributes, unlimDimIdP;
  status = nc_inq(root_id, &nDims, &nVars, &nRootAttributes, &unlimDimIdP);
  if (status != NC_NOERR) {
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_inq: ", status);
    return 1;
  }
  if (adagucMeasureTime) {
    StopWatch_Stop("NC_INQ");
  }

  // First readdims
  std::string groupName = "";
  traceTimingsSpanStart(TraceTimingType::FSNCREADDIMS);
  status = _readVariables(root_id, groupName, 0);
  traceTimingsSpanEnd(TraceTimingType::FSNCREADDIMS);
  if (status != 0) return 1;

  // Second read vars
  groupName = "";
  traceTimingsSpanStart(TraceTimingType::FSNCREADVARS);
  status = _readVariables(root_id, groupName, 1);
  traceTimingsSpanEnd(TraceTimingType::FSNCREADVARS);
  if (status != 0) return 1;

  if (adagucMeasureTime) {
    StopWatch_Stop("readVar");
  }
  traceTimingsSpanStart(TraceTimingType::FSNCREADATTRS);
  status = _readAttributes(root_id, cdfObject->attributes, NC_GLOBAL, nRootAttributes);
  traceTimingsSpanEnd(TraceTimingType::FSNCREADATTRS);
  if (status != 0) return 1;
  if (adagucMeasureTime) {
    StopWatch_Stop("readAttr");
  }
  return 0;
}

int CDFNetCDFReader::close() {
  if (root_id != -1) {
    if (CCDFNETCDFIO_DEBUG) {
      CDBDebug("CLOSING %s", fileName.c_str());
    }
    nc_close(root_id);
  }
  root_id = -1;
  return 0;
}