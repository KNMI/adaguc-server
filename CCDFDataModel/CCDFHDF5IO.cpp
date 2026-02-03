/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
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

#include <cmath>

int CDFHDF5Reader::CustomForecastReader::readData(CDF::Variable *thisVar, size_t *start, size_t *count, ptrdiff_t *stride) {
#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("READ data for %s called", thisVar->name.c_str());
#endif

  size_t newstart[thisVar->dimensionlinks.size()];
  for (size_t j = 0; j < thisVar->dimensionlinks.size(); j++) {
    newstart[j] = start[j];
#ifdef CCDFHDF5IO_DEBUG
    CDBDebug("%s %d %d %d %d", thisVar->dimensionlinks[j]->name.c_str(), j, start[j], count[j], stride[j]);
#endif
  }
  newstart[0] = 0;
  CT::string varName;
  varName.print("image%d%simage_data", (int)start[0] + 1, CCDFHDF5IO_GROUPSEPARATOR);
  CDF::Variable *var = ((CDFObject *)thisVar->getParentCDFObject())->getVariable(varName.c_str());

  CDBDebug("Start reading %s", var->name.c_str());

  CDFType readType = thisVar->getType();
  int status = var->readData(readType, newstart, count, stride, true);

  if (status != 0) {
    CDBError("CustomForecastReader: Unable to read variable %s", thisVar->name.c_str());
    return 1;
  }

  CDF::freeData(&thisVar->data);
  int size = 1;
  for (size_t j = 0; j < thisVar->dimensionlinks.size(); j++) {
    size *= int((float(count[j]) / float(stride[j])) + 0.5);
  }
  thisVar->setSize(size);
  CDF::allocateData(thisVar->getType(), &thisVar->data, size);
  status = CDF::DataCopier::copy(thisVar->data, thisVar->getType(), var->data, thisVar->getType(), 0, 0, size);
  if (status != 0) {
    CDBError("Unable to copy data");
    throw("__LINE__");
  }

  return 0;
}

CDFHDF5Reader::~CDFHDF5Reader() {
#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("CCDFHDF5IO close");
#endif
  close();
  /* Restore previous error handler */
  // H5Eset_auto2(error_stack, old_func, old_client_data);
}

CDFType CDFHDF5Reader::typeConversion(hid_t type) {
  if (H5Tequal(type, H5T_NATIVE_SCHAR) > 0) return CDF_BYTE;
  if (H5Tequal(type, H5T_NATIVE_UCHAR) > 0) return CDF_UBYTE;
  if (H5Tequal(type, H5T_NATIVE_CHAR) > 0) return CDF_CHAR;
  if (H5Tequal(type, H5T_NATIVE_SHORT) > 0) return CDF_SHORT;
  if (H5Tequal(type, H5T_NATIVE_USHORT) > 0) return CDF_USHORT;
  if (H5Tequal(type, H5T_NATIVE_INT) > 0) return CDF_INT;
  if (H5Tequal(type, H5T_NATIVE_UINT) > 0) return CDF_UINT;
  if (H5Tequal(type, H5T_NATIVE_FLOAT) > 0) return CDF_FLOAT;
  if (H5Tequal(type, H5T_NATIVE_DOUBLE) > 0) return CDF_DOUBLE;

  if (H5Tequal(type, H5T_NATIVE_LONG) > 0) return CDF_INT;
  if (H5Tequal(type, H5T_NATIVE_ULONG) > 0) return CDF_UINT;

  if (H5Tequal(type, H5T_NATIVE_LLONG) > 0) {
    CDBWarning("Warning: HDF5 type H5T_NATIVE_LLONG is not supported", type);
  }
  if (H5Tequal(type, H5T_NATIVE_ULLONG) > 0) {
    CDBWarning("Warning: HDF5 type H5T_NATIVE_ULLONG is not supported", type);
  }

  // CDBWarning("Warning: unknown HDF5 type (%d)",type);
  return CDF_NONE;
}

hid_t CDFHDF5Reader::cdfTypeToHDFType(CDFType type) {
  switch (type) {
  case CDF_BYTE:
    return H5T_NATIVE_SCHAR;
    break;
  case CDF_UBYTE:
    return H5T_NATIVE_UCHAR;
    break;
  case CDF_CHAR:
    return H5T_NATIVE_CHAR;
    break;
  case CDF_SHORT:
    return H5T_NATIVE_SHORT;
    break;
  case CDF_USHORT:
    return H5T_NATIVE_USHORT;
    break;
  case CDF_INT:
    return H5T_NATIVE_INT;
    break;
  case CDF_UINT:
    return H5T_NATIVE_UINT;
    break;
  case CDF_FLOAT:
    return H5T_NATIVE_FLOAT;
    break;
  case CDF_DOUBLE:
    return H5T_NATIVE_DOUBLE;
    break;
  }

  CDBWarning("Warning: unknown CDFType type (%d)", type);
  return H5T_NATIVE_DOUBLE;
}

CDF::Dimension *CDFHDF5Reader::makeDimension(const char *name, size_t len) {
  CDF::Dimension *dim = NULL;
  for (size_t j = 0; j < cdfObject->dimensions.size(); j++) {
    if (cdfObject->dimensions[j]->length == len) {
      // dim_1 == dim_1
      if (name[4] == cdfObject->dimensions[j]->name.c_str()[4]) {
        dim = cdfObject->dimensions[j];
        return dim;
      }
    }
  }
  char dimName[256];
  snprintf(dimName, 255, "%s_%d", name, int(cdfObject->dimensions.size()));
  dim = new CDF::Dimension();
  dim->length = len;
  CDF::Variable *var = new CDF::Variable();
  var->setCDFReaderPointer(this);
  var->setParentCDFObject(cdfObject);
  var->setName(dimName);
  var->id = cdfObject->variables.size();
  var->isDimension = true;
  var->dimensionlinks.push_back(dim);
  var->currentType = CDF_DOUBLE;
  var->nativeType = CDF_DOUBLE;
  if (CDF::allocateData(var->currentType, &var->data, dim->length)) {
    throw(__LINE__);
  }
  for (size_t i = 0; i < dim->length; i++) ((float *)var->data)[i] = 0.0f;
  var->setSize(dim->length);
  cdfObject->variables.push_back(var);
  cdfObject->dimensions.push_back(dim);
  dim->setName(dimName);

  return dim;
}

void CDFHDF5Reader::list(hid_t groupID, char *groupName) {
#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("list '%s'", groupName);
#endif

  H5G_info_t group_info;
  H5Gget_info(groupID, &group_info);
  // printf("%d\n",(int)group_info.nlinks);
  char name[256];

  for (int j = 0; j < (int)group_info.nlinks; j++) {
    H5Lget_name_by_idx(groupID, ".", H5_INDEX_NAME, H5_ITER_INC, j, name, 255, H5P_DEFAULT);
#ifdef CCDFHDF5IO_DEBUG
    CDBDebug("Getting group '%s'", name);
#endif

    hid_t objectID = H5Oopen(groupID, name, H5P_DEFAULT);
    H5I_type_t type = H5Iget_type(objectID);

    if (type == H5I_GROUP) {

      hid_t newGroupID = H5Gopen2(groupID, name, H5P_DEFAULT);
      // CDBDebug("Opened group %s with id %d from %d",name,newGroupID,groupID);

      if (newGroupID > 0) {
        char temp[1024];
        if (strlen(groupName) != 0) {
          snprintf(temp, 1023, "%s%s%s", groupName, CCDFHDF5IO_GROUPSEPARATOR, name);
        } else
          snprintf(temp, 1023, "%s", name);

        CDF::Variable *var = new CDF::Variable();
        var->currentType = CDF_CHAR;
        var->nativeType = CDF_CHAR;
        var->isDimension = false;
        var->setName(temp);
        var->id = cdfObject->variables.size();
        var->setCDFReaderPointer(this); // TODO
        var->setParentCDFObject(cdfObject);
        // Attributes:
        readAttributes(var->attributes, newGroupID);
        cdfObject->variables.push_back(var);
        list(newGroupID, temp);
        // CDBDebug("Closing %d",newGroupID);
        H5Gclose(newGroupID);
      }
      // readAttributes(cdfObject->attributes,newGroupID);
    }

    if (type == H5I_DATASET) {
#ifdef CCDFHDF5IO_DEBUG
      CDBDebug("H5I_DATASET: %d,%s", groupID, name);
#endif
      hid_t datasetID = H5Dopen2(groupID, name, H5P_DEFAULT);
#ifdef CCDFHDF5IO_DEBUG
      CDBDebug("Opened dataset %s with id %d from %d", name, datasetID, groupID);
#endif
      if (datasetID > 0) {
        hid_t datasetType = H5Dget_type(datasetID);
        if (datasetType > 0) {
          hid_t datasetNativeType = H5Tget_native_type(datasetType, H5T_DIR_ASCEND);
          if (datasetNativeType > 0) {
            char varName[1024];
            if (strlen(groupName) != 0) {
              snprintf(varName, 1023, "%s%s%s", groupName, CCDFHDF5IO_GROUPSEPARATOR, name);
            } else
              snprintf(varName, 1023, "%s", name);

            int cdfType = typeConversion(datasetNativeType);
            if (cdfType != CDF_NONE) {
              // CDBError("Unknown type for dataset %s",varName);
              // return;
              //}
#ifdef CCDFHDF5IO_DEBUG
              char tempType[20];
              CDF::getCDFDataTypeName(tempType, 19, cdfType);
              CDBDebug("DataType is %s", tempType);
#endif

              hid_t HDF5_dataspace = H5Dget_space(datasetID); /* dataspace handle */
              int ndims = H5Sget_simple_extent_ndims(HDF5_dataspace);
              if (ndims > 19) {
                CDBError("Maximum number of 20 dimensions supported, got %d dimensions", ndims);
                return;
              }
              hsize_t dims_out[20];
              H5Sget_simple_extent_dims(HDF5_dataspace, dims_out, NULL);
              CDF::Variable *var = cdfObject->getVariableNE(varName);
              if (var == NULL) {
                var = new CDF::Variable();
                cdfObject->variables.push_back(var);
              }
              var->currentType = cdfType;
              var->nativeType = cdfType;
              var->isDimension = false;
              var->setName(varName);

#ifdef CCDFHDF5IO_DEBUG
              CDBDebug("Adding %s", varName);
#endif
              var->id = cdfObject->variables.size();
              var->setCDFReaderPointer(this);
              var->setParentCDFObject(cdfObject);
              readAttributes(var->attributes, datasetID);
#ifdef CCDFHDF5IO_DEBUG
              CDBDebug("%s%s%s", groupName, CCDFHDF5IO_GROUPSEPARATOR, name);
#endif
              CDF::Dimension *dim;

              for (int d = 0; d < ndims; d++) {
#ifdef CCDFHDF5IO_DEBUG
                CDBDebug("Dim size %d=%d\t", d, (size_t)dims_out[d]);
#endif
                // Make fake dimensions
                char dimname[20];
                snprintf(dimname, 19, "dim_%d", d);
                if (ndims == 2) {
                  if (d == 0) dimname[4] = 'y';
                  if (d == 1) dimname[4] = 'x';
                }
#ifdef CCDFHDF5IO_DEBUG
                CDBDebug("Making dimension %s", dimname);
#endif
                dim = makeDimension(dimname, dims_out[d]);
                var->dimensionlinks.push_back(dim);
              }
              // printf("\n");

              H5Sclose(HDF5_dataspace);
              H5Tclose(datasetNativeType);
            } else {
              // CDBWarning("Unknown type for dataset %s",varName);
            }
          }
          // H5Tclose(datasetType);
        }

        H5Dclose(datasetID);
      }
    }
    H5Oclose(objectID);
  }
}

int CDFHDF5Reader::open(const char *fileName) {
  CT::string cpy = (fileName);
  this->fileName = cpy.c_str();
#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Opening HDF5 file %s", this->fileName.c_str());
#endif
  H5F_file = H5Fopen(this->fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

  if (H5F_file < 0) {
    CDBError("could not open HDF5 file [%s]", this->fileName.c_str());
    return 1;
  }

  // Read global attributes
#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Opening group \"%s\"", CCDFHDF5IO_GROUPSEPARATOR);
#endif
  // hid_t HDF5_group = H5Gopen(H5F_file,"."); API V1.6
  hid_t HDF5_group = H5Gopen2(H5F_file, ".", H5P_DEFAULT);
  if (HDF5_group < 0) {
    CDBError("could not open HDF5 group");
    H5Fclose(H5F_file);

    return 1;
  }
#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("readAttributes");
#endif
  readAttributes(cdfObject->attributes, HDF5_group); // TODO
  H5Gclose(HDF5_group);

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("list");
#endif
  list(H5F_file, (char *)"");

  if (b_EnableKNMIHDF5toCFConversion) {
#ifdef CCDFHDF5IO_DEBUG
    CDBDebug("convertKNMIHDF5toCF()");
#endif
    int status = convertKNMIHDF5toCF();
    if (status == 1) return 1;
    status = convertNWCSAFtoCF();
    if (status == 1) return 1;
    status = convertLSASAFtoCF();
    if (status == 1) return 1;
  }
  if (b_EnableODIMHDF5toCFConversion) {
    int status = convertODIMHDF5toCF();
    if (status == 1) return 1;
  }
  fileIsOpen = true;
  return 0;
}
int CDFHDF5Reader::close() {
  if (H5F_file != -1) H5Fclose(H5F_file);
  if (forecastReader != NULL) {
    delete forecastReader;
    forecastReader = NULL;
  }
  fileIsOpen = false;
  return 0;
}

hid_t CDFHDF5Reader::openH5GroupByName(char *varNameOut, size_t maxVarNameLen, const char *variableGroupName) {
  if (fileIsOpen == false) {
    //        CDBError("openH5GroupByName: File is not open");
    //      CDBDebug("Trying to open [%s]",fileName.c_str());
    if (open(fileName.c_str()) != 0) return -1;
  }
  hid_t HDF5_group = H5F_file;
  hid_t newGroupID;
  CT::string varName(variableGroupName);
  auto paths = varName.split(CCDFHDF5IO_GROUPSEPARATOR);
  if (paths.size() == 0) {
    return -1;
  }
  for (size_t j = 0; j < paths.size() - 1; j++) {

    newGroupID = H5Gopen2(HDF5_group, paths[j].c_str(), H5P_DEFAULT);
    // CDBDebug("Opened group %s with id %d from %d",paths[j].c_str(),newGroupID,HDF5_group);
    if (newGroupID < 0) {
      CDBError("group %s for variable %s not found", paths[j].c_str(), varName.c_str());
      return -1;
    }

    opengroups.push_back(newGroupID);
    HDF5_group = newGroupID;
  }

  if (maxVarNameLen < paths[paths.size() - 1].length() + 1) {
    CDBError("varName string size not large enough to hold variable name ");
    return -1;
  }
  snprintf(varNameOut, maxVarNameLen, "%s", paths[paths.size() - 1].c_str());
  return HDF5_group;
}
void CDFHDF5Reader::closeH5GroupByName(const char *variableGroupName) {
  ignoreParameter(variableGroupName);
  // CDBDebug("Warning %s variableGroupName not used", variableGroupName);
  while (opengroups.size() > 0) {
#ifdef CCDFHDF5IO_DEBUG
    CDBDebug("closing with id %d", opengroups.back());
#endif
    opengroups.pop_back();
  }
}
int CDFHDF5Reader::_readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *) {
  if (var->data != NULL) {
    CDBWarning("Not reading any data because it is already in memory");
    return 0;
  }
  if (fileIsOpen == false) {
    CDBError("openH5GroupByName: File is not open");
    CDBDebug("Trying to open [%s]", fileName.c_str());
    if (open(fileName.c_str()) != 0) return -1;
  }

  char typeName[32];
  CDF::getCDFDataTypeName(typeName, 31, type);
#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Reading %s --> %s with type %s", var->name.c_str(), var->orgName.c_str(), typeName);
#endif
  char varName[1024];
  hid_t HDF5_group = openH5GroupByName(varName, 1023, var->orgName.c_str());
  if (HDF5_group > 0) {
#ifdef CCDFHDF5IO_DEBUG
    CDBDebug("Group  %s Openend, got variable %s", var->orgName.c_str(), varName);
#endif
    hid_t datasetID = H5Dopen2(HDF5_group, varName, H5P_DEFAULT);
    if (datasetID > 0) {
#ifdef CCDFHDF5IO_DEBUG
      CDBDebug("Dataset Openend");
#endif
      hid_t HDF5_dataspace = H5Dget_space(datasetID);
      int ndims = H5Sget_simple_extent_ndims(HDF5_dataspace);
      hsize_t dims_out[ndims];
      H5Sget_simple_extent_dims(HDF5_dataspace, dims_out, NULL);
      hsize_t mem_count[ndims], mem_start[ndims];
      hsize_t data_count[ndims], data_start[ndims];
      int totalVariableSize = 1;
      int dimDiff = var->dimensionlinks.size() - ndims;
      if (dimDiff < 0) dimDiff = 0;
      for (int d = 0; d < ndims; d++) {
        mem_start[d] = 0;
        mem_count[d] = count[d + dimDiff];
        data_start[d] = start[d + dimDiff]; // mem_start[d];
        data_count[d] = mem_count[d];
#ifdef CCDFHDF5IO_DEBUG
        CDBDebug("%d %d, %d", d, data_start[d], data_count[d]);
#endif
        totalVariableSize *= mem_count[d];
        ;
      }

#ifdef CCDFHDF5IO_DEBUG
      CDBDebug("totalVariableSize= %d", totalVariableSize);
#endif
      var->setSize(totalVariableSize);
      if (CDF::allocateData(type, &var->data, var->getSize())) {
        throw(__LINE__);
      }
      hid_t HDF5_memspace = H5Screate_simple(2, mem_count, NULL);
      H5Sselect_hyperslab(HDF5_memspace, H5S_SELECT_SET, mem_start, NULL, mem_count, NULL);
      H5Sselect_hyperslab(HDF5_dataspace, H5S_SELECT_SET, data_start, NULL, data_count, NULL);
      // hid_t datasetType=H5Dget_type(datasetID);
      hid_t datasetType = cdfTypeToHDFType(type);
      H5Dread(datasetID, datasetType, HDF5_memspace, HDF5_dataspace, H5P_DEFAULT, var->data);

      /*
      if(type==CDF_USHORT){
      for(size_t j=0;j<var->getSize();j++){
      unsigned short a=((unsigned short*)(var->data))[j];
      unsigned char p1=a;
      unsigned char p2=a/256;
      a=p1*256+p2;
      ((unsigned short*)(var->data))[j]=a;
    }
    }
      if(type==CDF_SHORT){
      for(size_t j=0;j<var->getSize();j++){
      short a=((short*)(var->data))[j];
      char p1=a;
      char p2=a/256;
      a=p1*256+p2;
      ((short*)(var->data))[j]=a;
    }
    }*/

      // H5Tclose(datasetType);
      H5Sclose(HDF5_memspace);
      H5Sclose(HDF5_dataspace);
      H5Dclose(datasetID);
    } else {
      CDBError("Unable to open variable %s with group ID %d", varName, HDF5_group);
      closeH5GroupByName(var->name.c_str());
      return 1;
    }
  } else {
    closeH5GroupByName(var->name.c_str());
    CDBError("Unable to open HDF5 group name %s", var->orgName.c_str());
    return 1;
  }
  closeH5GroupByName(var->name.c_str());
  return 0;
}
int CDFHDF5Reader::_readVariableData(CDF::Variable *var, CDFType type) {
  // All ready in memory
  int status = 0;
  //      CDBDebug(" ***** %s size=%d",var->name.c_str(),var->size());
  if (var->data != NULL) {
    // CDBWarning("Not reading any data because it is already in memory");
    return 0;
  }

#ifdef CCDFHDF5IO_DEBUG
  char typeName[32];
  CDF::getCDFDataTypeName(typeName, 31, type);
  CDBDebug("Reading %s == %s with type %s", var->name.c_str(), var->orgName.c_str(), typeName);
#endif

  char varName[1024];
  hid_t HDF5_group = openH5GroupByName(varName, 1023, var->orgName.c_str());
  if (HDF5_group > 0) {
    hid_t datasetID = H5Dopen2(HDF5_group, varName, H5P_DEFAULT);
    if (datasetID > 0) {
      hid_t HDF5_dataspace = H5Dget_space(datasetID);
      int ndims = H5Sget_simple_extent_ndims(HDF5_dataspace);
      hsize_t dims_out[ndims];
      H5Sget_simple_extent_dims(HDF5_dataspace, dims_out, NULL);
      hsize_t mem_count[ndims], mem_start[ndims];
      int totalVariableSize = 1;

      for (int d = 0; d < ndims; d++) {
        mem_start[d] = 0;
        mem_count[d] = dims_out[d];
        totalVariableSize *= mem_count[d];
        ;
        // CDBDebug("dim size fpr %s = %d",varName,mem_count[d]);
      }

      var->setSize(totalVariableSize);
      if (CDF::allocateData(type, &var->data, var->getSize())) {
        throw(__LINE__);
      }
      hid_t HDF5_memspace = H5Screate_simple(2, mem_count, NULL);
      H5Sselect_hyperslab(HDF5_memspace, H5S_SELECT_SET, mem_start, NULL, mem_count, NULL);
      H5Sselect_hyperslab(HDF5_dataspace, H5S_SELECT_SET, mem_start, NULL, mem_count, NULL);
      // hid_t datasetType=H5Dget_type(datasetID);
      hid_t datasetType = cdfTypeToHDFType(type);
      // datasetType
      H5Dread(datasetID, datasetType, HDF5_memspace, HDF5_dataspace, H5P_DEFAULT, var->data);

      /* if(type==CDF_USHORT)
        {
          for(size_t j=0;j<var->getSize();j++){
            unsigned short a=((unsigned short*)(var->data))[j];
            unsigned char p1=a;
            unsigned char p2=a/256;
            a=p1*256+p2;
            ((unsigned short*)(var->data))[j]=a;
          }
      }*/

      // H5Tclose(datasetType);
      H5Sclose(HDF5_memspace);
      H5Sclose(HDF5_dataspace);
      H5Dclose(datasetID);
    } else {
      CDBError("Unable to find dataset id for variable %s", varName);
      status = -1;
    }
  } else {
    CDBError("Unable to find group id");
  }
  closeH5GroupByName(var->name.c_str());
  return status;
}

void CDFHDF5Reader::enableKNMIHDF5toCFConversion() {
  b_EnableKNMIHDF5toCFConversion = true;
  b_EnableODIMHDF5toCFConversion = true;
}

void CDFHDF5Reader::enableKNMIHDF5UseEndTime() { CDFHDF5Reader::b_KNMIHDF5UseEndTime = true; }

int CDFHDF5Reader::HDF5ToADAGUCTime(char *pszADAGUCTime, const char *pszRadarTime) {
  int M;
  char szMonth[4];
  // All month abbreviations
  const char *pszMonths[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  // Copy from input time the month number
  strncpy(szMonth, pszRadarTime + 3, 4);
  szMonth[3] = '\0';
  // Uppercase all
  for (unsigned int j = 0; j < 3; j++)
    if (szMonth[j] >= 'a' && szMonth[j] <= 'z') szMonth[j] -= 32;
  // Try to find the month
  for (M = 0; M < 12; M++)
    if (strncmp(szMonth, pszMonths[M], 3) == 0) break;
  M++; // Months are from 1-12 not 0-11
       // The month string was not found ...
  if (M == 13) {
    CDBError("Invalid month: %s", szMonth);
    return 1;
  }
  // 012345678901234
  strncpy(pszADAGUCTime, "20000101T000000\0", 16);

  snprintf(szMonth, 3, "%02d", M);
  strncpy(pszADAGUCTime, pszRadarTime + 7, 4);
  strncpy(pszADAGUCTime + 4, szMonth, 2);
  strncpy(pszADAGUCTime + 6, pszRadarTime, 2);
  strncpy(pszADAGUCTime + 8, "T\0", 2);
  strncpy(pszADAGUCTime + 9, pszRadarTime + 12, 2);  // Hours
  strncpy(pszADAGUCTime + 11, pszRadarTime + 15, 2); // Minutes
  //       if(strlen(pszRadarTime) > 15 ) {
  //         strncpy(pszADAGUCTime+13,pszRadarTime+18,2);
  //       }
  pszADAGUCTime[15] = '\0';
  // CDBDebug("%s --> %s",pszRadarTime,pszADAGUCTime);
  return 0;
}

int CDFHDF5Reader::convertNWCSAFtoCF() {
  if (cdfObject->getAttributeNE("PROJECTION") == NULL) {
    // Silently SKIP, this is not NWC SAF data.
    return 0;
  };

  bool dimsDone = false;
  for (size_t j = 0; j < cdfObject->variables.size(); j++) {
    cdfObject->variables[j]->setAttributeText("ADAGUC_SKIP", "true");
    CT::string projectionString = "";
    try {
      if (cdfObject->variables[j]->getAttribute("CLASS")->toString().toLowerCase().equals("image")) {
        cdfObject->variables[j]->removeAttribute("ADAGUC_SKIP");
        CDBDebug("Variable %s is an IMAGE", cdfObject->variables[j]->name.c_str());
        if (dimsDone == false) {

          float fXGEO_UP_LEFT[1];
          float fYGEO_UP_LEFT[1];
          float fXGEO_LOW_RIGHT[1];
          float fYGEO_LOW_RIGHT[1];

          CT::string timeString = "";

          CDF::Attribute *NOMINAL_PRODUCT_TIME = cdfObject->getAttributeNE("NOMINAL_PRODUCT_TIME");
          if (NOMINAL_PRODUCT_TIME != NULL) {
            timeString = NOMINAL_PRODUCT_TIME->toString();
          } else {
            CDBError("NOMINAL_PRODUCT_TIME  not found");
            return 1;
          }
          CDF::Attribute *PROJECTION = cdfObject->getAttributeNE("PROJECTION");
          if (PROJECTION != NULL) {
            projectionString = PROJECTION->toString();
          } else {
            CDBError("PROJECTION  not found");
            return 1;
          }
          CDF::Attribute *XGEO_UP_LEFT = cdfObject->getAttributeNE("XGEO_UP_LEFT");
          if (XGEO_UP_LEFT != NULL) {
            XGEO_UP_LEFT->getData(fXGEO_UP_LEFT, 1);
          } else {
            CDBError("XGEO_UP_LEFT not found");
            return 1;
          }
          CDF::Attribute *YGEO_UP_LEFT = cdfObject->getAttributeNE("YGEO_UP_LEFT");
          if (YGEO_UP_LEFT != NULL) {
            YGEO_UP_LEFT->getData(fYGEO_UP_LEFT, 1);
          } else {
            CDBError("YGEO_UP_LEFT not found");
            return 1;
          }
          CDF::Attribute *XGEO_LOW_RIGHT = cdfObject->getAttributeNE("XGEO_LOW_RIGHT");
          if (XGEO_LOW_RIGHT != NULL) {
            XGEO_LOW_RIGHT->getData(fXGEO_LOW_RIGHT, 1);
          } else {
            CDBError("XGEO_LOW_RIGHT not found");
            return 1;
          }
          CDF::Attribute *YGEO_LOW_RIGHT = cdfObject->getAttributeNE("YGEO_LOW_RIGHT");
          if (YGEO_LOW_RIGHT != NULL) {
            YGEO_LOW_RIGHT->getData(fYGEO_LOW_RIGHT, 1);
          } else {
            CDBError("YGEO_LOW_RIGHT not found");
            return 1;
          }
          CDBDebug("NOMINAL_PRODUCT_TIME = %s", timeString.c_str());
          CDBDebug("PROJECTION = %s", projectionString.c_str());
          CDBDebug("XGEO_UP_LEFT = %f", fXGEO_UP_LEFT[0]);
          CDBDebug("YGEO_UP_LEFT = %f", fYGEO_UP_LEFT[0]);
          CDBDebug("XGEO_LOW_RIGHT = %f", fXGEO_LOW_RIGHT[0]);
          CDBDebug("YGEO_LOW_RIGHT = %f", fYGEO_LOW_RIGHT[0]);

          CDF::Dimension *dimX = cdfObject->variables[j]->dimensionlinks[1];
          CDF::Dimension *dimY = cdfObject->variables[j]->dimensionlinks[0];
          CDF::Variable *varX = cdfObject->getVariableNE(dimX->name.c_str());
          CDF::Variable *varY = cdfObject->getVariableNE(dimY->name.c_str());

          if (varX == NULL) {
            CDBError("XDimension not found");
            return 1;
          }
          if (varY == NULL) {
            CDBError("YDimension not found");
            return 1;
          }

          if (CDF::allocateData(varX->currentType, &varX->data, dimX->length)) {
            throw(__LINE__);
          }
          if (CDF::allocateData(varY->currentType, &varY->data, dimY->length)) {
            throw(__LINE__);
          }

          varX->setName("x");
          varY->setName("y");
          dimX->setName("x");
          dimY->setName("y");

#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("Creating virtual dimensions x and y");
#endif

          double cellSizeX = (fXGEO_LOW_RIGHT[0] - fXGEO_UP_LEFT[0]) / float(dimX->length);
          double cellSizeY = (fYGEO_LOW_RIGHT[0] - fYGEO_UP_LEFT[0]) / float(dimY->length);
          double offsetX = fXGEO_UP_LEFT[0];
          double offsetY = fYGEO_UP_LEFT[0];

          CDBDebug("Cellsize X = %f", cellSizeX);
          CDBDebug("Cellsize Y = %f", cellSizeY);

          for (size_t j = 0; j < dimX->length; j = j + 1) {
            double x = offsetX + (double(j)) * cellSizeX + cellSizeX / 2;
            ((double *)varX->data)[j] = x;
          }

          for (size_t j = 0; j < dimY->length; j = j + 1) {
            double y = offsetY + (float(j)) * cellSizeY + cellSizeY / 2;
            ((double *)varY->data)[j] = y;
          }

          CDF::Variable *projection = NULL;
          projection = cdfObject->getVariableNE("projection");
          if (projection == NULL) {
            projection = new CDF::Variable();
            cdfObject->addVariable(projection);
            projection->setName("projection");
            projection->currentType = CDF_CHAR;
            projection->nativeType = CDF_CHAR;
            projection->isDimension = false;
          }
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("CProj4ToCF");
#endif
          CProj4ToCF proj4ToCF;
          proj4ToCF.convertProjToCF(projection, projectionString.c_str());
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("/CProj4ToCF");
#endif
          CDF::Attribute *proj4_params = projection->getAttributeNE("proj4_params");
          if (proj4_params == NULL) {
            proj4_params = new CDF::Attribute();
            projection->addAttribute(proj4_params);
            proj4_params->setName("proj4_params");
          }
          proj4_params->setData(CDF_CHAR, projectionString.c_str(), projectionString.length());

          // Set time dimension
          CDF::Variable *time = new CDF::Variable();
          CDF::Dimension *timeDim = new CDF::Dimension();
          cdfObject->addDimension(timeDim);
          cdfObject->addVariable(time);
          time->setName("time");
          timeDim->setName("time");
          timeDim->length = 1;
          time->currentType = CDF_DOUBLE;
          time->nativeType = CDF_DOUBLE;
          time->isDimension = true;
          CDF::Attribute *time_units = new CDF::Attribute();
          time_units->setName("units");
          time_units->setData("minutes since 2000-01-01 00:00:00\0");
          time->addAttribute(time_units);

          CDF::Attribute *standard_name = new CDF::Attribute();
          standard_name->setName("standard_name");
          standard_name->setData("time");
          time->addAttribute(standard_name);

          time->dimensionlinks.push_back(timeDim);

          // Set adaguc time
          CTime *ctime = new CTime();
          if (ctime->init((char *)time_units->data, NULL) != 0) {
            CDBError("Could not initialize CTIME: %s", (char *)time_units->data);
            return 1;
          }
          double offset;
          try {
            offset = ctime->dateToOffset(ctime->freeDateStringToDate(timeString.c_str()));
          } catch (int e) {
            CT::string message = CTime::getErrorMessage(e);
            CDBError("CTime Exception %s", message.c_str());
            delete ctime;
            return 1;
          }

          time->setSize(1);
          if (CDF::allocateData(time->currentType, &time->data, time->getSize())) {
            throw(__LINE__);
          }
          ((double *)time->data)[0] = offset;
          if (status != 0) {
            CDBError("Could not initialize time: %s", timeString.c_str());
            delete ctime;
            return 1;
          }
          delete ctime;

          dimsDone = true;
        }
        CDF::Attribute *grid_mapping = new CDF::Attribute();
        grid_mapping->setName("grid_mapping");
        grid_mapping->setData(CDF_CHAR, (char *)"projection\0", 11);
        cdfObject->variables[j]->addAttribute(grid_mapping);

        cdfObject->variables[j]->dimensionlinks.insert(cdfObject->variables[j]->dimensionlinks.begin(), 1, cdfObject->getDimension("time"));

        // Scale and offset

        CDF::Attribute *SCALING_FACTOR = cdfObject->variables[j]->getAttributeNE("SCALING_FACTOR");
        CDF::Attribute *OFFSET = cdfObject->variables[j]->getAttributeNE("OFFSET");
        if (SCALING_FACTOR != NULL && OFFSET != NULL) {
          float additionFactor[1];
          float multiplicationFactor[1];

          SCALING_FACTOR->getData(multiplicationFactor, 1);
          OFFSET->getData(additionFactor, 1);

          CDF::Attribute *add_offset = new CDF::Attribute();
          add_offset->setName("add_offset");
          add_offset->setData(CDF_FLOAT, additionFactor, 1);
          cdfObject->variables[j]->addAttribute(add_offset);

          CDF::Attribute *scale_factor = new CDF::Attribute();
          scale_factor->setName("scale_factor");
          scale_factor->setData(CDF_FLOAT, multiplicationFactor, 1);
          cdfObject->variables[j]->addAttribute(scale_factor);
        }

        if (cdfObject->variables[j]->getType() == CDF_UBYTE) {
          unsigned char FfillValue = 0;
          CDF::Attribute *fillValue = new CDF::Attribute();
          fillValue->setName("_FillValue");
          fillValue->setData(CDF_UBYTE, &FfillValue, 1);
          cdfObject->variables[j]->addAttribute(fillValue);
        }
      }
    } catch (int e) {
    }
  }
  return 0;
}

int CDFHDF5Reader::convertLSASAFtoCF() {
  if (cdfObject->getAttributeNE("PROJECTION_NAME") == NULL) {
    // Silently SKIP, this is not LSA SAF data.
    return 0;
  };

  bool dimsDone = false;
  for (size_t j = 0; j < cdfObject->variables.size(); j++) {
    cdfObject->variables[j]->setAttributeText("ADAGUC_SKIP", "true");
    CT::string projectionString = "";
    try {
      if (cdfObject->variables[j]->getAttribute("CLASS")->toString().toLowerCase().equals("data")) {
        cdfObject->variables[j]->removeAttribute("ADAGUC_SKIP");
        // CDBDebug("Variable %s is an IMAGE",cdfObject->variables[j]->name.c_str());
        if (dimsDone == false) {

          CT::string timeString = "";

          CDF::Attribute *SENSING_START_TIME = cdfObject->getAttributeNE("SENSING_START_TIME");
          if (SENSING_START_TIME != NULL) {
            timeString = SENSING_START_TIME->toString();
          } else {
            CDBError("SENSING_START_TIME  not found");
            return 1;
          }
          CDF::Attribute *PROJECTION = cdfObject->getAttributeNE("PROJECTION_NAME");
          if (PROJECTION != NULL) {
            projectionString = PROJECTION->toString();
          } else {
            CDBError("PROJECTION  not found");
            return 1;
          }

          float fXGEO_UP_LEFT;
          fXGEO_UP_LEFT = -922500;
          float fYGEO_UP_LEFT;
          fYGEO_UP_LEFT = 5422500;
          float fXGEO_LOW_RIGHT;
          fXGEO_LOW_RIGHT = 4180500;
          float fYGEO_LOW_RIGHT;
          fYGEO_LOW_RIGHT = 3469500;

          //           CDBDebug("SENSING_START_TIME = %s",timeString.c_str());
          //           CDBDebug("PROJECTION = %s",projectionString.c_str());
          //           CDBDebug("XGEO_UP_LEFT = %f",fXGEO_UP_LEFT);
          //           CDBDebug("YGEO_UP_LEFT = %f",fYGEO_UP_LEFT);
          //           CDBDebug("XGEO_LOW_RIGHT = %f",fXGEO_LOW_RIGHT);
          //           CDBDebug("YGEO_LOW_RIGHT = %f",fYGEO_LOW_RIGHT);
          //
          CDF::Dimension *dimX = cdfObject->variables[j]->dimensionlinks[1];
          CDF::Dimension *dimY = cdfObject->variables[j]->dimensionlinks[0];
          CDF::Variable *varX = cdfObject->getVariableNE(dimX->name.c_str());
          CDF::Variable *varY = cdfObject->getVariableNE(dimY->name.c_str());

          if (varX == NULL) {
            CDBError("XDimension not found");
            return 1;
          }
          if (varY == NULL) {
            CDBError("YDimension not found");
            return 1;
          }

          if (CDF::allocateData(varX->currentType, &varX->data, dimX->length)) {
            throw(__LINE__);
          }
          if (CDF::allocateData(varY->currentType, &varY->data, dimY->length)) {
            throw(__LINE__);
          }

          varX->setName("x");
          varY->setName("y");
          dimX->setName("x");
          dimY->setName("y");

#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("Creating virtual dimensions x and y");
#endif

          double cellSizeX = (fXGEO_LOW_RIGHT - fXGEO_UP_LEFT) / float(dimX->length);
          double cellSizeY = (fYGEO_LOW_RIGHT - fYGEO_UP_LEFT) / float(dimY->length);
          double offsetX = fXGEO_UP_LEFT;
          double offsetY = fYGEO_UP_LEFT;

          CDBDebug("Cellsize X = %f", cellSizeX);
          CDBDebug("Cellsize Y = %f", cellSizeY);

          for (size_t j = 0; j < dimX->length; j = j + 1) {
            double x = offsetX + (double(j)) * cellSizeX + cellSizeX / 2;
            ((double *)varX->data)[j] = x;
          }

          for (size_t j = 0; j < dimY->length; j = j + 1) {
            double y = offsetY + (float(j)) * cellSizeY + cellSizeY / 2;
            ((double *)varY->data)[j] = y;
          }

          CDF::Variable *projection = NULL;
          projection = cdfObject->getVariableNE("projection");
          if (projection == NULL) {
            projection = new CDF::Variable();
            cdfObject->addVariable(projection);
            projection->setName("projection");
            projection->currentType = CDF_CHAR;
            projection->nativeType = CDF_CHAR;
            projection->isDimension = false;
          }
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("CProj4ToCF");
#endif
          CProj4ToCF proj4ToCF;
          proj4ToCF.convertProjToCF(projection, projectionString.c_str());
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("/CProj4ToCF");
#endif
          CDF::Attribute *proj4_params = projection->getAttributeNE("proj4_params");
          if (proj4_params == NULL) {
            proj4_params = new CDF::Attribute();
            projection->addAttribute(proj4_params);
            proj4_params->setName("proj4_params");
          }
          proj4_params->setData("+proj=geos +a=6378169.0 +b=6356583.8 +lon_0=0.0 +h=35785831.0");

          // Set time dimension

          CDF::Variable *time = new CDF::Variable();
          CDF::Dimension *timeDim = new CDF::Dimension();
          cdfObject->addDimension(timeDim);
          cdfObject->addVariable(time);
          time->setName("time");
          timeDim->setName("time");
          timeDim->length = 1;
          time->currentType = CDF_DOUBLE;
          time->nativeType = CDF_DOUBLE;
          time->isDimension = true;
          CDF::Attribute *time_units = new CDF::Attribute();
          time_units->setName("units");
          time_units->setData("minutes since 2000-01-01 00:00:00\0");
          time->addAttribute(time_units);

          CDF::Attribute *standard_name = new CDF::Attribute();
          standard_name->setName("standard_name");
          standard_name->setData("time");
          time->addAttribute(standard_name);

          time->dimensionlinks.push_back(timeDim);

          // Set adaguc time
          CTime *ctime = new CTime();
          if (ctime->init((char *)time_units->data, NULL) != 0) {
            CDBError("Could not initialize CTIME: %s", (char *)time_units->data);
            return 1;
          }
          double offset;
          try {
            offset = ctime->dateToOffset(ctime->freeDateStringToDate(timeString.c_str()));
          } catch (int e) {
            CT::string message = CTime::getErrorMessage(e);
            CDBError("CTime Exception %s", message.c_str());
            delete ctime;
            return 1;
          }

          time->setSize(1);
          if (CDF::allocateData(time->currentType, &time->data, time->getSize())) {
            throw(__LINE__);
          }
          ((double *)time->data)[0] = offset;
          if (status != 0) {
            CDBError("Could not initialize time: %s", timeString.c_str());
            delete ctime;
            return 1;
          }
          delete ctime;

          dimsDone = true;
        }
        CDF::Attribute *grid_mapping = new CDF::Attribute();
        grid_mapping->setName("grid_mapping");
        grid_mapping->setData(CDF_CHAR, (char *)"projection\0", 11);
        cdfObject->variables[j]->addAttribute(grid_mapping);

        cdfObject->variables[j]->dimensionlinks.insert(cdfObject->variables[j]->dimensionlinks.begin(), 1, cdfObject->getDimension("time"));

        // Scale and offset

        CDF::Attribute *SCALING_FACTOR = cdfObject->variables[j]->getAttributeNE("SCALING_FACTOR");
        CDF::Attribute *OFFSET = cdfObject->variables[j]->getAttributeNE("OFFSET");
        if (SCALING_FACTOR != NULL && OFFSET != NULL) {
          float additionFactor[1];
          float multiplicationFactor[1];

          if (SCALING_FACTOR->size() == 1) {
            SCALING_FACTOR->getData(multiplicationFactor, 1);
            CDBDebug("Setting offset to %f and %f", multiplicationFactor[0]);
            CDF::Attribute *scale_factor = new CDF::Attribute();
            scale_factor->setName("scale_factor");
            scale_factor->setData(CDF_FLOAT, multiplicationFactor, 1);
            cdfObject->variables[j]->addAttribute(scale_factor);
          }

          // FOR LSA, scaling factor should be inversed. This is probably an error in the LSA files!
          if (SCALING_FACTOR->size() == 2) {
            SCALING_FACTOR->getData(multiplicationFactor, 1);
            if (multiplicationFactor[0] != 0) {
              multiplicationFactor[0] = 1 / multiplicationFactor[0];
              CDBDebug("Setting offset to %f and %f", multiplicationFactor[0]);
              CDF::Attribute *scale_factor = new CDF::Attribute();
              scale_factor->setName("scale_factor");
              scale_factor->setData(CDF_FLOAT, multiplicationFactor, 1);
              cdfObject->variables[j]->addAttribute(scale_factor);
            }
          }

          if (OFFSET->size() == 1) {
            OFFSET->getData(additionFactor, 1);
            CDBDebug("Setting offset to %f and %f", additionFactor[0]);
            CDF::Attribute *add_offset = new CDF::Attribute();
            add_offset->setName("add_offset");
            add_offset->setData(CDF_FLOAT, additionFactor, 1);
            cdfObject->variables[j]->addAttribute(add_offset);
          }
        }

        CDF::Attribute *UNITS = cdfObject->variables[j]->getAttributeNE("UNITS");
        if (UNITS != NULL) {
          cdfObject->variables[j]->setAttributeText("units", UNITS->toString().c_str());
        }

        CDF::Attribute *MISSING_VALUE = cdfObject->variables[j]->getAttributeNE("MISSING_VALUE");
        if (MISSING_VALUE != NULL) {
          MISSING_VALUE->setName("_FillValue");
        } else if (cdfObject->variables[j]->getType() == CDF_UBYTE) {
          unsigned char FfillValue = 0;
          CDF::Attribute *fillValue = new CDF::Attribute();
          fillValue->setName("_FillValue");
          fillValue->setData(CDF_UBYTE, &FfillValue, 1);
          cdfObject->variables[j]->addAttribute(fillValue);
        }
      }
    } catch (int e) {
    }
  }
  return 0;
}

int CDFHDF5Reader::readAttributes(std::vector<CDF::Attribute *> &attributes, hid_t HDF5_group) {
  hid_t HDF5_attr_class, HDF5_attribute;
  int dNumAttributes = H5Aget_num_attrs(HDF5_group);
  for (int j = 0; j < dNumAttributes; j++) {
    HDF5_attribute = H5Aopen_idx(HDF5_group, j);
    size_t attNameSize = H5Aget_name(HDF5_attribute, 0, NULL);
    attNameSize++;
    char attName[attNameSize + 1];
    H5Aget_name(HDF5_attribute, attNameSize, attName);
    hid_t HDF5_attr_type = H5Aget_type(HDF5_attribute);
    HDF5_attr_class = H5Tget_class(HDF5_attr_type);
    // Read H5T_INTEGER Attribute
    if (HDF5_attr_class == H5T_INTEGER && 1 == 1) {
      hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);
      hsize_t stSize = H5Aget_storage_size(HDF5_attribute) / sizeof(int);
      CDF::Attribute *attr = new CDF::Attribute();
      attributes.push_back(attr);
      attr->setName(attName);

      attr->type = typeConversion(HDF5_attr_memtype);
      attr->length = stSize;
      if (attr->length == 0) attr->length = 1; // TODO
      // printf("%s %d\n",attName,stSize);
      if (CDF::allocateData(attr->type, &attr->data, attr->length + 1) != 0) {
        CDBError("Unable to allocateData for attribute %s", attr->name.c_str());
        throw(__LINE__);
      }

      status = H5Aread(HDF5_attribute, HDF5_attr_memtype, attr->data);

      status = H5Tclose(HDF5_attr_memtype);
    }

    if (HDF5_attr_class == H5T_FLOAT && 1 == 1) {
      hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);
      hsize_t stSize = H5Aget_storage_size(HDF5_attribute) / sizeof(float);
      CDF::Attribute *attr = new CDF::Attribute();
      attr->setName(attName);
      attr->type = CDF_FLOAT;
      attr->length = stSize;
      if (CDF::allocateData(attr->type, &attr->data, attr->length + 1)) {
        throw(__LINE__);
      }

      status = H5Aread(HDF5_attribute, H5T_NATIVE_FLOAT, attr->data);
      attributes.push_back(attr);
      status = H5Tclose(HDF5_attr_memtype);
    }
    if (HDF5_attr_class == H5T_STRING) {
      hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);

      bool isVariableLength = (H5Tis_variable_str(HDF5_attr_memtype) > 0);
      CDF::Attribute *attr = new CDF::Attribute();
      attr->setName(attName);
      attr->type = CDF_CHAR;
      if (!isVariableLength) {
        hsize_t stSize = H5Aget_storage_size(HDF5_attribute) * sizeof(char);
        attr->length = stSize;
        if (CDF::allocateData(attr->type, &attr->data, attr->length + 1)) {
          throw(__LINE__);
        }
        status = H5Aread(HDF5_attribute, HDF5_attr_type, attr->data);
        size_t stringLength = strlen(attr->getDataAsString().c_str());
        if (attr->length > stringLength) attr->length = stringLength + 1;
        ((char *)attr->data)[attr->length] = '\0';
      } else {
        hid_t space = H5Aget_space(HDF5_attribute);
        hsize_t dims[1] = {1};
        int length = dims[0] * sizeof(char *);
        char **rdata = (char **)malloc(length);
        status = H5Aread(HDF5_attribute, HDF5_attr_type, rdata);
        if (status != -1) {
          attr->length = strlen(rdata[0]);
          if (CDF::allocateData(attr->type, &attr->data, attr->length + 1)) {
            throw(__LINE__);
          }
          memcpy((char *)attr->data, rdata[0], strlen(rdata[0]));
          H5Dvlen_reclaim(HDF5_attr_type, space, H5P_DEFAULT, rdata);
          free(rdata);
        }
        H5Sclose(space);
      }

      attributes.push_back(attr);
      status = H5Tclose(HDF5_attr_memtype);
    }

    H5Tclose(HDF5_attr_type);
    H5Aclose(HDF5_attribute);
  }
  return 0;
}

int CDFHDF5Reader::convertKNMIHDF5toCF() {

  CDF::Variable *geo = cdfObject->getVariableNE("geographic");
  if (geo == NULL) {
    return 2;
  }

  // Fill in dim ranges
  CT::string variableName;
  variableName.print("image1%simage_data", CCDFHDF5IO_GROUPSEPARATOR);
  CDF::Variable *var = cdfObject->getVariableNE(variableName.c_str());
  // if(var==NULL){CDBError("variable %s not found",variableName.c_str());return 1;}
  geo = cdfObject->getVariableNE("geographic");
  if (geo == NULL) {
    CDBError("variable geographic not found");
    return 0;
  }
  // if(var->dimensionlinks.size()!=2){CDBError("variable does not have 2 dims");return 1;}
  variableName.print("geographic%smap_projection", CCDFHDF5IO_GROUPSEPARATOR);
  CDF::Variable *proj = cdfObject->getVariableNE(variableName.c_str());
  if (proj == NULL) {
    CDBError("variable geographic.map_projection not found");
    return 1;
  }
  CDF::Attribute *cellsizeXattr = geo->getAttributeNE("geo_pixel_size_x");
  CDF::Attribute *cellsizeYattr = geo->getAttributeNE("geo_pixel_size_y");
  CDF::Attribute *offsetXattr = geo->getAttributeNE("geo_column_offset");
  CDF::Attribute *offsetYattr = geo->getAttributeNE("geo_row_offset");
  CDF::Attribute *proj4attr = proj->getAttributeNE("projection_proj4_params");
  if (cellsizeXattr == NULL || cellsizeYattr == NULL || offsetXattr == NULL || offsetYattr == NULL || proj4attr == NULL) {
    CDBError("geographic attributes incorrect");
    return 1;
  }
  CDF::Variable *overview = cdfObject->getVariableNE("overview");
  if (overview == NULL) {
    CDBError("variable overview not found");
    return 1;
  }
  CDF::Attribute *product_datetime_start = overview->getAttributeNE("product_datetime_start");
  if (product_datetime_start == NULL) {
    CDBError("attribute product_datetime_start not found");
    return 1;
  }

  CDF::Attribute *product_datetime_end = overview->getAttributeNE("product_datetime_end");
  if (product_datetime_end == NULL) {
    CDBError("attribute product_datetime_end not found");
    return 1;
  }

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Detecting time parameters");
#endif
  char szStartTime[100];
  char szEndTime[100];
  status = HDF5ToADAGUCTime(szStartTime, (char *)product_datetime_start->data);
  if (status != 0) {
    CDBError("Could not initialize time");
    return 1;
  }
  status = HDF5ToADAGUCTime(szEndTime, (char *)product_datetime_end->data);
  if (status != 0) {
    CDBError("Could not initialize time");
    return 1;
  }
  if (HDF5ToADAGUCTime(szStartTime, (char *)product_datetime_start->data) != 0) {
    CDBError("Could not initialize time");
    return 1;
  }
  if (HDF5ToADAGUCTime(szEndTime, (char *)product_datetime_end->data) != 0) {
    CDBError("Could not initialize time");
    return 1;
  }

  /* Add these obtained values in an ADAGUC way to the file */
  CDF::Variable *product = cdfObject->getVariableNE("product");
  if (product == NULL) {
    product = new CDF::Variable();
    product->name = "product";
    product->setType(CDF_BYTE);
    product->setSize(1);
    if (CDF::allocateData(product->currentType, &product->data, 1)) {
      throw(__LINE__);
    }
    cdfObject->addVariable(product);
  }
  product->addAttribute(new CDF::Attribute("validity_start", szStartTime));
  product->addAttribute(new CDF::Attribute("validity_stop", szEndTime));

  /* Try to get additional values*/
  variableName.print("image1%ssatellite", CCDFHDF5IO_GROUPSEPARATOR);
  CDF::Variable *image1_satellite = cdfObject->getVariableNE(variableName.c_str());
  if (image1_satellite != NULL) {
    CDF::Attribute *image_acquisition_time = image1_satellite->getAttributeNE("image_acquisition_time");
    CDF::Attribute *image_generation_time = image1_satellite->getAttributeNE("image_generation_time");
    if (image_acquisition_time != NULL) {
      char text[100];
      if (HDF5ToADAGUCTime(text, (char *)image_acquisition_time->data) == 0) {
        product->addAttribute(new CDF::Attribute("image1_acquisition_time", text));
      }
      if (HDF5ToADAGUCTime(text, (char *)image_generation_time->data) == 0) {
        product->addAttribute(new CDF::Attribute("image1_generation_time", text));
      }
    }
  }

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Detecting boundingbox from iso_dataset");
#endif

  /*Fill in bounding box in isodataset*/
  /*
   *                 iso_dataset:max-x = 10.9f ;
   *                iso_dataset:min-x = 0.f ;
   *                iso_dataset:max-y = 56.f ;
   *                iso_dataset:min-y = 48.8f ;
   */
  CDF::Variable *iso_dataset = cdfObject->getVariableNE("iso_dataset");
  if (iso_dataset == NULL) {
    iso_dataset = new CDF::Variable();
    iso_dataset->name = "iso_dataset";
    iso_dataset->setType(CDF_BYTE);
    if (CDF::allocateData(iso_dataset->currentType, &iso_dataset->data, 1)) {
      throw(__LINE__);
    }
    cdfObject->addVariable(iso_dataset);
  }

  try {

    CT::string productCornerString = (char *)geo->getAttribute("geo_product_corners")->toString().c_str();

    std::vector<CT::string> coords = productCornerString.trim().split(" ");

    if (coords.size() > 6) {
      iso_dataset->addAttribute(new CDF::Attribute("min-x", coords[0].c_str()));
      iso_dataset->addAttribute(new CDF::Attribute("min-y", coords[1].c_str()));
      iso_dataset->addAttribute(new CDF::Attribute("max-x", coords[4].c_str()));
      iso_dataset->addAttribute(new CDF::Attribute("max-y", coords[5].c_str()));
    } else {
#ifdef CCDFHDF5IO_DEBUG
      CDBDebug("geo_product_corners is invalid");
#endif
    }
  } catch (int e) {
  }

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Setting global values");
#endif

  /* Fill in global values */
  cdfObject->addAttribute(new CDF::Attribute("Conventions", "CF-1.6"));
  cdfObject->addAttribute(new CDF::Attribute("history", "Metadata adjusted by ADAGUC from KNMIHDF5 to NetCDF-CF"));
  try {
    cdfObject->addAttribute(new CDF::Attribute("institution", (char *)overview->getAttribute("dataset_org_descr")->data));
  } catch (int e) {
  }
  try {
    cdfObject->addAttribute(new CDF::Attribute("source", (char *)overview->getAttribute("hdf5_url")->data));
  } catch (int e) {
  }
  try {
    cdfObject->addAttribute(new CDF::Attribute("references", (char *)overview->getAttribute("hdftag_url")->data));
  } catch (int e) {
  }
  try {
    cdfObject->addAttribute(new CDF::Attribute("title", (char *)overview->getAttribute("product_group_title")->data));
  } catch (int e) {
  }

  if (var == NULL) return 0;
  CDF::Dimension *dimX = var->dimensionlinks[1];
  CDF::Dimension *dimY = var->dimensionlinks[0];
  CDF::Variable *varX = cdfObject->getVariableNE(dimX->name.c_str());
  CDF::Variable *varY = cdfObject->getVariableNE(dimY->name.c_str());

  if (CDF::allocateData(varX->currentType, &varX->data, dimX->length)) {
    throw(__LINE__);
  }
  if (CDF::allocateData(varY->currentType, &varY->data, dimY->length)) {
    throw(__LINE__);
  }

  varX->setName("x");
  varY->setName("y");
  dimX->setName("x");
  dimY->setName("y");

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Creating virtual dimensions x and y");
#endif

  double cellSizeX, cellSizeY, offsetX, offsetY;
  cellsizeXattr->getData(&cellSizeX, 1);
  cellsizeYattr->getData(&cellSizeY, 1);
  offsetXattr->getData(&offsetX, 1);
  offsetYattr->getData(&offsetY, 1);

  for (size_t j = 0; j < dimX->length; j = j + 1) {
    double x = (offsetX + double(j)) * cellSizeX + cellSizeX / 2;
    ((double *)varX->data)[j] = x;
  }

  for (size_t j = 0; j < dimY->length; j = j + 1) {
    double y = (offsetY + float(j)) * cellSizeY + cellSizeY / 2;
    ((double *)varY->data)[j] = y;
  }

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Detecting projection");
#endif

  CDF::Variable *projection = NULL;
  projection = cdfObject->getVariableNE("projection");
  if (projection == NULL) {
    projection = new CDF::Variable();
    cdfObject->addVariable(projection);
    projection->setName("projection");
    projection->currentType = CDF_CHAR;
    projection->nativeType = CDF_CHAR;
    if (CDF::allocateData(projection->currentType, &projection->data, 1)) {
      throw(__LINE__);
    }
    projection->isDimension = false;
  }

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("CProj4ToCF");
#endif
  CProj4ToCF proj4ToCF;
  proj4ToCF.convertProjToCF(projection, (char *)proj4attr->data);
#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("/CProj4ToCF");
#endif
  CDF::Attribute *proj4_params = projection->getAttributeNE("proj4_params");
  if (proj4_params == NULL) {
    proj4_params = new CDF::Attribute();
    projection->addAttribute(proj4_params);
    proj4_params->setName("proj4_params");
  }

  // Most KNMI files have wrong projection definition, replace nsper by geos,
  CT::string projectionString;
  projectionString.copy((char *)proj4attr->data, proj4attr->length);
  projectionString.replaceSelf("nsper", "geos");

  proj4_params->setData(CDF_CHAR, projectionString.c_str(), projectionString.length());

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Set time dimension");
#endif
  // Set time dimension
  CDF::Variable *time = new CDF::Variable();
  CDF::Dimension *timeDim = new CDF::Dimension();
  cdfObject->addDimension(timeDim);
  cdfObject->addVariable(time);
  time->setName("time");
  timeDim->setName("time");
  timeDim->length = 1;
  time->currentType = CDF_DOUBLE;
  time->nativeType = CDF_DOUBLE;
  time->isDimension = true;
  CDF::Attribute *time_units = new CDF::Attribute();
  time_units->setName("units");
  time_units->setData("minutes since 2000-01-01 00:00:00\0");
  time->addAttribute(time_units);

  CDF::Attribute *standard_name = new CDF::Attribute();
  standard_name->setName("standard_name");
  standard_name->setData("time");
  time->addAttribute(standard_name);

  time->dimensionlinks.push_back(timeDim);

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("CTime");
#endif
  // Set adaguc time
  CTime ctime;
  if (ctime.init((char *)time_units->data, "") != 0) {
    CDBError("Could not initialize CTIME: %s", (char *)time_units->data);
    return 1;
  }
  double offset;
  if (b_KNMIHDF5UseEndTime) {
    // Use product_datetime_end if explicitly configured
    try {
      offset = ctime.dateToOffset(ctime.stringToDate(szEndTime));
    } catch (int e) {
      CT::string message = CTime::getErrorMessage(e);
      CDBError("CTime Exception %s", message.c_str());

      return 1;
    }
  } else {
    try {
      offset = ctime.dateToOffset(ctime.stringToDate(szStartTime));
    } catch (int e) {
      CT::string message = CTime::getErrorMessage(e);
      CDBError("CTime Exception %s", message.c_str());

      return 1;
    }
  }

  bool isForecastData = false;

  // Try to detect if this is forecast data by checking if image1:image_datetime_valid  exists.
  try {
    cdfObject->getVariable("image1")->getAttribute("image_datetime_valid");
    CDBDebug("This is forecast data");
    isForecastData = true;

    // Create forecast_reference_time variable
    CDF::Variable *forecast_reference_time = new CDF::Variable();
    forecast_reference_time->setName("forecast_reference_time");
    forecast_reference_time->setSize(1);
    forecast_reference_time->setType(CDF_DOUBLE);
    CDF::allocateData(forecast_reference_time->currentType, &forecast_reference_time->data, forecast_reference_time->getSize());
    cdfObject->addVariable(forecast_reference_time);
    forecast_reference_time->setAttributeText("units", (char *)time_units->data);
    forecast_reference_time->setAttributeText("standard_name", "forecast_reference_time");
    ((double *)forecast_reference_time->data)[0] = offset;

  } catch (int e) {
  }

  int timeLength = 1;

  if (isForecastData) {
    CDF::Attribute *number_image_groups = overview->getAttributeNE("number_image_groups");
    if (number_image_groups != NULL) {
      number_image_groups->getData(&timeLength, 1);
    }
  }

  timeDim->setSize(timeLength);

  time->setSize(timeDim->getSize());
  if (CDF::allocateData(time->currentType, &time->data, time->getSize())) {
    throw(__LINE__);
  }
  for (size_t j = 0; j < timeDim->getSize(); j++) {
    ((double *)time->data)[j] = offset; // This will be filled correctly in at the image looping section
  }

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Time size = %d", time->getSize());
#endif

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("Set grid_mapping for all variables");
#endif

  for (size_t j = 0; j < cdfObject->variables.size(); j++) {
    cdfObject->variables[j]->setAttributeText("ADAGUC_SKIP", "true");
  }

  if (isForecastData) {
    CDF::Variable *forecast = new CDF::Variable();
    forecast->setName("forecast");
    forecast->setType(CDF_FLOAT); // Forces new type to float
    forecast->dimensionlinks.push_back(timeDim);
    forecast->dimensionlinks.push_back(dimY);
    forecast->dimensionlinks.push_back(dimX);
    forecast->setAttributeText("grid_mapping", "projection");
    forecastReader = new CustomForecastReader();
    forecast->setCustomReader(forecastReader);
    cdfObject->addVariable(forecast);
  }

  // Loop through all images and set grid_mapping name
  CT::string varName;
  size_t variableCounter = 1;

  // This is the image looping section
  {
    do {
      varName.print("image%d%simage_data", variableCounter, CCDFHDF5IO_GROUPSEPARATOR);
      var = cdfObject->getVariableNE(varName.c_str());
      if (var != NULL) {
        if (isForecastData == false) var->removeAttribute("ADAGUC_SKIP");

        CDF::Attribute *grid_mapping = new CDF::Attribute();
        grid_mapping->setName("grid_mapping");
        grid_mapping->setData(CDF_CHAR, (char *)"projection\0", 11);
        var->addAttribute(grid_mapping);
        var->dimensionlinks.insert(var->dimensionlinks.begin(), 1, timeDim);

        // Set units
        varName.print("image%d", variableCounter);
        CDF::Variable *imageN = cdfObject->getVariableNE(varName.c_str());
        if (imageN != NULL) {
          CDF::Attribute *image_geo_parameter = imageN->getAttributeNE("image_geo_parameter");
          if (image_geo_parameter != NULL) {
            CDF::Attribute *units = new CDF::Attribute();
            units->setName("units");
            CT::string unitString;
            image_geo_parameter->getDataAsString(&unitString);
            units->setData(unitString.c_str());
            var->addAttribute(units);
          }
        }

        // Set no data value
        // Get nodatavalue:
        // Get calibration group: First check if one is defined for specified image number, if not, use from image 1.

        varName.print("image%d%scalibration", variableCounter, CCDFHDF5IO_GROUPSEPARATOR);
        CDF::Variable *calibration = cdfObject->getVariableNE(varName.c_str());
        if (calibration == NULL) {
          varName.print("image%d%scalibration", 1, CCDFHDF5IO_GROUPSEPARATOR);
          calibration = cdfObject->getVariableNE(varName.c_str());
        }

        if (calibration != NULL) {
          CDF::Attribute *calibration_out_of_image = calibration->getAttributeNE("calibration_out_of_image");
          if (calibration_out_of_image != NULL) {
            double dfNodata = 0;
            calibration_out_of_image->getData(&dfNodata, 1);
            //          CDBDebug("Found calibration_out_of_image attribute value=%f, status = %d",dfNodata,status);
            // if(dfNodata!=0)
            {
              CDF::Attribute *noDataAttr = new CDF::Attribute();
              noDataAttr->setName("_FillValue");
              char attrType[256];
              CDF::getCDFDataTypeName(attrType, 255, var->currentType);
#ifdef CCDFHDF5IO_DEBUG
              CDBDebug("%s: Setting type %s", var->name.c_str(), attrType);
#endif

              switch (var->currentType) {
              case CDF_CHAR: {
                char nodata = (char)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              case CDF_BYTE: {
                char nodata = (char)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              case CDF_UBYTE: {
                unsigned char nodata = (unsigned char)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              case CDF_SHORT: {
                short nodata = (short)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              case CDF_USHORT: {
                unsigned short nodata = (unsigned short)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              case CDF_INT: {
                int nodata = (int)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              case CDF_UINT: {
                unsigned int nodata = (unsigned int)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              case CDF_FLOAT: {
                float nodata = (float)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              case CDF_DOUBLE: {
                double nodata = (double)dfNodata;
                noDataAttr->setData(var->currentType, &nodata, 1);
              }; break;
              }
              var->addAttribute(noDataAttr);
            }
          }

          // Try to detect calibration_formulas and convert them to scale_factor and add_offset attributes
          CDF::Attribute *calibration_formulas = calibration->getAttributeNE("calibration_formulas");
          if (calibration_formulas != NULL) {
            CT::string formula = calibration_formulas->getDataAsString();
            // CDBDebug("Formula: %s",formula.c_str());
            int rightPartFormulaPos = formula.indexOf("=");
            int multiplicationSignPos = formula.indexOf("*");
            int additionSignPos = formula.indexOf("+");
            if (rightPartFormulaPos != -1 && multiplicationSignPos != -1 && additionSignPos != -1) {

              float multiplicationFactor = formula.substring(rightPartFormulaPos + 1, multiplicationSignPos).trim().toFloat();
              float additionFactor = formula.substring(additionSignPos + 1, formula.length()).trim().toFloat();
              // CDBDebug("* = '%s' '%f' and + = '%s' '%f'",multiplicationFactorStr.c_str(),additionFactorStr.c_str(),multiplicationFactor,additionFactor);
              //                  CDBDebug("Formula %s provides y='%f'*x+'%f'",formula.c_str(),multiplicationFactor,additionFactor);
              CDF::Attribute *add_offset = new CDF::Attribute();
              add_offset->setName("add_offset");
              add_offset->setData(CDF_FLOAT, &additionFactor, 1);
              var->addAttribute(add_offset);

              CDF::Attribute *scale_factor = new CDF::Attribute();
              scale_factor->setName("scale_factor");
              scale_factor->setData(CDF_FLOAT, &multiplicationFactor, 1);
              var->addAttribute(scale_factor);
            }
          }
        }

        // Try to detect image_datetime_valid for forecast data
        CDF::Attribute *image_datetime_valid = imageN->getAttributeNE("image_datetime_valid");
        if (image_datetime_valid != NULL) {
          CT::string datetime_valid = image_datetime_valid->getDataAsString();

          char valid_time_iso[100];
          status = HDF5ToADAGUCTime(valid_time_iso, datetime_valid.c_str());
          if (status != 0) {
            CDBError("Could not initialize time");
            return 1;
          }
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("image%d:image_datetime_valid = [%s] is [%s]", variableCounter, datetime_valid.c_str(), valid_time_iso);
#endif

          double offset;
          try {
            offset = ctime.dateToOffset(ctime.stringToDate(valid_time_iso));
#ifdef CCDFHDF5IO_DEBUG
            CDBDebug("Setting time offset %f for image %d", offset, variableCounter);
#endif
            if (variableCounter - 1 >= time->getSize()) {
              CDBWarning("More images found than specified in overview:number_image_groups, number_image_groups is set to %d", time->getSize());
            } else {
              ((double *)time->data)[variableCounter - 1] = offset;
            }
          } catch (int e) {
            CT::string message = CTime::getErrorMessage(e);
            CDBError("CTime Exception %s", message.c_str());
            return 1;
          }
        }
      }
      variableCounter++;
    } while (var != NULL);
  }

#ifdef CCDFHDF5IO_DEBUG
  CDBDebug("convertKNMIHDF5toCF finished");
#endif

  return 0;
}
