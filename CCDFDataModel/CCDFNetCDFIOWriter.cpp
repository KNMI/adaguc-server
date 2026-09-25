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

#include "CCDFNetCDFIOWriter.h"
#include <algorithm>

static const bool CCDFNETCDFWRITER_DEBUG = false;

nc_type CDFNetCDFWriter::NCtypeConversion(CDFType type) {
  if (type == CDF_BYTE) return NC_BYTE;
  if (type == CDF_UBYTE) return NC_UBYTE;
  if (type == CDF_CHAR) return NC_CHAR;
  if (type == CDF_SHORT) return NC_SHORT;
  if (type == CDF_USHORT) return NC_USHORT;
  if (type == CDF_INT) return NC_INT;
  if (type == CDF_UINT) return NC_UINT;
  if (type == CDF_INT64) return NC_INT64;
  if (type == CDF_UINT64) return NC_UINT64;
  if (type == CDF_FLOAT) return NC_FLOAT;
  if (type == CDF_DOUBLE) return NC_DOUBLE;
  if (type == CDF_STRING) return NC_STRING;
  return NC_DOUBLE;
}

std::string CDFNetCDFWriter::NCtypeConversionToString(CDFType type) {
  std::string r;
  r = "NC_DOUBLE";
  if (type == CDF_BYTE) r = "NC_BYTE";
  if (type == CDF_UBYTE) r = "NC_UBYTE";
  if (type == CDF_CHAR) r = "NC_CHAR";
  if (type == CDF_SHORT) r = "NC_SHORT";
  if (type == CDF_USHORT) r = "NC_USHORT";
  if (type == CDF_INT) r = "NC_INT";
  if (type == CDF_UINT) r = "NC_UINT";
  if (type == CDF_INT64) r = "NC_INT64";
  if (type == CDF_UINT64) r = "NC_UINT64";
  if (type == CDF_FLOAT) r = "NC_FLOAT";
  if (type == CDF_DOUBLE) r = "NC_DOUBLE";
  if (type == CDF_STRING) r = "NC_STRING";
  if (type == CDF_UNKNOWN) r = "NC_DOUBLE";
  return r;
}

CDFNetCDFWriter::CDFNetCDFWriter(CDFObject *cdfObject) {
  this->cdfObject = cdfObject;
  writeData = true;
  readData = true;
  listNCCommands = false;
  netcdfMode = 4;
  shuffle = 0;
  deflate = 1;
  deflate_level = 2;
};

CDFNetCDFWriter::~CDFNetCDFWriter() {
  for (auto *dimension: dimensions) {
    delete dimension;
  }
};

void CDFNetCDFWriter::setDeflateShuffle(int deflate, int deflate_level, int shuffle) {
  this->deflate = deflate;
  this->shuffle = shuffle;
  this->deflate_level = deflate_level;
}

void CDFNetCDFWriter::setNetCDFMode(int mode) {
  if (mode != 3 && mode != 4) {
    CDBError("Illegal netcdf mode %d: keeping mode %d", mode, netcdfMode);
    return;
  }
  netcdfMode = mode;
};

void CDFNetCDFWriter::disableVariableWrite() { writeData = false; };

void CDFNetCDFWriter::disableReadData() { readData = false; };

void CDFNetCDFWriter::recordNCCommands(bool enable) { listNCCommands = enable; }

std::string CDFNetCDFWriter::getNCCommands() { return NCCommands; };

int CDFNetCDFWriter::write(const char *fileName) { return write(fileName, NULL); }

int CDFNetCDFWriter::write(const char *fileName, void (*progress)(const char *message, float percentage)) {
  NCCommands = "";
  if (listNCCommands) {
    CT::printfconcat(NCCommands, "int root_id;\n");
    CT::printfconcat(NCCommands, "size_t start[];\n");
    CT::printfconcat(NCCommands, "size_t count[];\n");
    CT::printfconcat(NCCommands, "int dimIDArray[];\n");
    CT::printfconcat(NCCommands, "int shuffle=%d;\n", shuffle);
    CT::printfconcat(NCCommands, "int deflate=%d;\n", deflate);
    CT::printfconcat(NCCommands, "int deflate_level=%d;\n", deflate_level);
    CT::printfconcat(NCCommands, "int numDims=%d;\n", 0);
    CT::printfconcat(NCCommands, "void *variable_data=NULL;\n");

    for (size_t j = 0; j < cdfObject->dimensions.size(); j++) {
      CT::printfconcat(NCCommands, "int dim_id_%zu;\n", j);
    }
    for (size_t j = 0; j < cdfObject->variables.size(); j++) {
      CT::printfconcat(NCCommands, "int var_id_%zu;\n", j);
    }
  }

  this->fileName = fileName;
  if (CCDFNETCDFWRITER_DEBUG) {
    CDBDebug("Writing to file %s", fileName);
  }
  if (netcdfMode > 3) {
    status = nc_create(fileName, NC_NETCDF4 | NC_CLOBBER, &root_id);
    if (listNCCommands) {
      CT::printfconcat(NCCommands, "nc_create(\"%s\" ,NC_NETCDF4|NC_CLOBBER , &root_id);\n", fileName);
    }
  } else {
    status = nc_create(fileName, NC_CLOBBER | NC_64BIT_OFFSET, &root_id);
    if (listNCCommands) {
      CT::printfconcat(NCCommands, "nc_create(\"%s\" ,NC_CLOBBER|NC_64BIT_OFFSET , &root_id);\n", fileName);
    }
  }
  if (status != NC_NOERR) {
    CDBError("Unable to create %s", fileName);
    CDBError("[%s]: %s %d", nc_strerror(status), "nc_create: ", status);
    nc_close(root_id);
    root_id = -1;

    return 1;
  }
  status = _write(progress);
  if (CCDFNETCDFWRITER_DEBUG) {
    CDBDebug("Finished writing to file %s", fileName);
  }

  nc_close(root_id);
  root_id = -1;
  if (listNCCommands) {
    CT::printfconcat(NCCommands, "nc_close(root_id);\n");
  }

  return status;
};

int CDFNetCDFWriter::_write(void (*progress)(const char *message, float percentage)) {
  if (CCDFNETCDFWRITER_DEBUG) {
    CDBDebug("Writing global attributes");
  }

  // Write global attributes
  for (size_t i = 0; i < cdfObject->attributes.size(); i++) {
    status =
        nc_put_att(root_id, NC_GLOBAL, cdfObject->attributes[i]->name.c_str(), NCtypeConversion(cdfObject->attributes[i]->getType()), cdfObject->attributes[i]->length, cdfObject->attributes[i]->data);
    if (listNCCommands) {

      void *data = cdfObject->attributes[i]->data;
      const size_t length = cdfObject->attributes[i]->length;
      const CDFType type = cdfObject->attributes[i]->getType();
      if (type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE) {
        std::string out = "";
        out.append((const char *)data, length);
        CT::printfconcat(NCCommands, "nc_put_att(root_id, NC_GLOBAL, \"%s\",%s,%zu,\"%s\");\n", cdfObject->attributes[i]->name.c_str(), NCtypeConversionToString(type).c_str(),
                         cdfObject->attributes[i]->length, out.c_str());
      } else {
        if (type == CDF_INT || type == CDF_UINT) {
          CT::printfconcat(NCCommands, "int attrData_%zu[]={", i);
          for (size_t n = 0; n < length; n++) {
            CT::printfconcat(NCCommands, "%d", ((int *)data)[n]);
            if (n < length - 1) {
              CT::printfconcat(NCCommands, ",");
            }
            CT::printfconcat(NCCommands, "};\n");
          }
        }

        if (type == CDF_INT64 || type == CDF_UINT64) {
          CT::printfconcat(NCCommands, "int64 attrData_%zu[]={", i);
          for (size_t n = 0; n < length; n++) {
            CT::printfconcat(NCCommands, "%ld", ((long *)data)[n]);
            if (n < length - 1) {
              CT::printfconcat(NCCommands, ",");
            }
            CT::printfconcat(NCCommands, "};\n");
          }
        }

        if (type == CDF_SHORT || type == CDF_USHORT) {
          CT::printfconcat(NCCommands, "short attrData_%zu[]={", i);
          for (size_t n = 0; n < length; n++) {
            CT::printfconcat(NCCommands, "%d", ((short *)data)[n]);
            if (n < length - 1) {
              CT::printfconcat(NCCommands, ",");
            }
            CT::printfconcat(NCCommands, "};\n");
          }
        }

        if (type == CDF_FLOAT) {
          CT::printfconcat(NCCommands, "float attrData_%zu[]={", i);
          for (size_t n = 0; n < length; n++) {
            CT::printfconcat(NCCommands, "%f", ((float *)data)[n]);
            if (n < length - 1) {
              CT::printfconcat(NCCommands, ",");
            }
            CT::printfconcat(NCCommands, "};\n");
          }
        }

        if (type == CDF_DOUBLE) {
          CT::printfconcat(NCCommands, "float attrData_%zu[]={", i);
          for (size_t n = 0; n < length; n++) {
            CT::printfconcat(NCCommands, "%f", ((double *)data)[n]);
            if (n < length - 1) {
              CT::printfconcat(NCCommands, ",");
            }
            CT::printfconcat(NCCommands, "};\n");
          }
        }
        CT::printfconcat(NCCommands, "nc_put_att(root_id, NC_GLOBAL, \"%s\",%s,%zu,attrData_%zu);\n", cdfObject->attributes[i]->name.c_str(), NCtypeConversionToString(type).c_str(),
                         cdfObject->attributes[i]->length, i);
      }
    }

    if (status != NC_NOERR) {
      CDBError("For attribute NC_GLOBAL::%s of type %s:", cdfObject->attributes[i]->name.c_str(), CDF::getCDFDataTypeName(cdfObject->attributes[i]->getType()).c_str());
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_put_att: ", status);
      return 1;
    }
  }
  if (CCDFNETCDFWRITER_DEBUG) {
    CDBDebug("Define dimensions");
  }

  // Define dimensions
  for (size_t j = 0; j < cdfObject->dimensions.size(); j++) {
    CDF::Dimension *dim = new CDF::Dimension();
    dim->setName(cdfObject->dimensions[j]->name);
    dim->length = cdfObject->dimensions[j]->length;

    status = nc_def_dim(root_id, dim->name.c_str(), dim->length, &dim->id);
    if (CCDFNETCDFWRITER_DEBUG) {
      CDBDebug("DEF DIM %s %zu %d", dim->name.c_str(), dim->length, dim->id);
    }
    if (listNCCommands) {
      CT::printfconcat(NCCommands, "nc_def_dim(root_id,\"%s\" , %zu, &dim_id_%zu);\n", dim->name.c_str(), dim->length, j);
    }
    if (status != NC_NOERR) {
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_def_dim: ", status);
      delete dim;
      return 1;
    }
    dimensions.push_back(dim);
  }

  int nrVarsWritten = 0;
  // Write the variables
  // First write the variables connected to dimensions and later the variables itself
  // writeDimsFirst==0: dimension variables
  // writeDimsFirst==1: variables
  for (int writeDimsFirst = 0; writeDimsFirst < 2; writeDimsFirst++) {
    if (CCDFNETCDFWRITER_DEBUG) {
      if (writeDimsFirst == 0) {
        CDBDebug("Write dimensions");
      }
      if (writeDimsFirst == 1) {
        CDBDebug("Write variables");
      }
    }

    // Write all different variables.
    for (size_t j = 0; j < cdfObject->variables.size(); j++) {
      // Get the variable names with these dimensions
      CDF::Variable *variable = cdfObject->variables[j];
      const char *name = variable->name.c_str();
      if (CCDFNETCDFWRITER_DEBUG) {
        if (writeDimsFirst == 0) {
          CDBDebug("Writing %s", name);
        }
      }

      const int numDims = variable->dimensionlinks.size();
      if ((variable->isDimension == true && writeDimsFirst == 0) || (variable->isDimension == false && writeDimsFirst == 1)) {
        {
          std::vector<int> dimIDS(numDims);
          std::vector<int> NCCommandID(numDims);

          size_t totalVariableSize = 0;
          // Find dim and chunk info
          std::string variableInfo(name);
          variableInfo += "\t(";
          for (int i = 0; i < numDims; i++) {
            const auto it = std::find_if(dimensions.begin(), dimensions.end(), [&](const CDF::Dimension *dim) { return dim->name == variable->dimensionlinks[i]->name; });
            if (it != dimensions.end()) {
              CDF::Dimension *dim = *it;
              dimIDS[i] = dim->id;
              NCCommandID[i] = it - dimensions.begin();
              if (totalVariableSize == 0) totalVariableSize = 1;
              totalVariableSize *= dim->length;
              CT::printfconcat(variableInfo, "%s=%zu", dim->name.c_str(), dim->length);
              if (i + 1 < numDims) variableInfo += ",";
            }
          }
          variableInfo += ")";
          int nc_var_id;
          status = nc_redef(root_id);
          if (listNCCommands) {
            CT::printfconcat(NCCommands, "nc_redef(root_id);\n");
          }
          status = nc_def_var(root_id, name, NCtypeConversion(variable->currentType), numDims, dimIDS.data(), &nc_var_id);
          if (status != NC_NOERR) {
            CDBError("Unable to define variable %s", name);
            CDBError("[%s]: %s %d", nc_strerror(status), "nc_def_var: ", status);
            return 1;
          }
          if (listNCCommands) {

            CT::printfconcat(NCCommands, "numDims=%d;\n", numDims);
            for (int k = 0; k < numDims; k++) {
              CT::printfconcat(NCCommands, "dimIDArray[%d]=dim_id_%d;\n", k, NCCommandID[k]);
            }
            CT::printfconcat(NCCommands, "nc_def_var(root_id, \"%s\",%s,numDims, dimIDArray,&var_id_%zu);\n", name, NCtypeConversionToString(variable->currentType).c_str(), j);
          }

          // Set chunking and deflate options

          if (netcdfMode >= 4 && numDims > 0 && 1 == 1) {

            if (variable->dimensionlinks.size() > 2) {
              std::vector<size_t> chunkSizes;
              chunkSizes.reserve(variable->dimensionlinks.size());
              for (auto *dimensionlink: variable->dimensionlinks) {
                size_t chunkSize = dimensionlink->getSize();
                CDF::Variable *dimVar = cdfObject->getVar(dimensionlink->name);
                if (dimVar != nullptr) {
                  CDF::Attribute *standardNameAttr = dimVar->getAttributeNE("standard_name");
                  if (standardNameAttr != nullptr && standardNameAttr->toString() == "time") {
                    chunkSize = 1;
                  }
                }
                chunkSizes.push_back(chunkSize);
              }

              status = nc_def_var_chunking(root_id, nc_var_id, 0, chunkSizes.data());
              if (status != NC_NOERR) {
                CDBError("[%s]: %s %d", nc_strerror(status), "nc_def_var_chunking: ", status);
                return 1;
              }
            }
          }

          if (netcdfMode >= 4) {
            /* Only set deflate settings on non-scalar variables */
            /* Compression on variable length variables is no longer supported: https://github.com/Unidata/netcdf-c/pull/2231 */
            if (variable->dimensionlinks.size() > 0 && variable->currentType != CDF_STRING) {
              status = nc_def_var_deflate(root_id, nc_var_id, shuffle, deflate, deflate_level);
              if (status != NC_NOERR) {
                CDBError("[%s]: %s %d", nc_strerror(status), "nc_def_var_deflate: ", status);
                return 1;
              }
              if (listNCCommands) {
                CT::printfconcat(NCCommands, "nc_def_var_deflate(root_id,var_id_%zu,shuffle ,deflate, deflate_level);\n", j);
              }
            }
          }

          // copy data
          if (CCDFNETCDFWRITER_DEBUG) {
            std::string message;
            message = CT::printf("%d/%zu Copying data for variable %s: total %d bytes", nrVarsWritten + 1, cdfObject->variables.size(), variableInfo.c_str(),
                                 int(totalVariableSize) * CDF::getTypeSize(variable->getType()));
            CDBDebug("%s", message.c_str());
          }
          // Copy attributes for this specific variable
          for (size_t i = 0; i < variable->attributes.size(); i++) {
            if (variable->attributes[i]->name != "CLASS" && variable->attributes[i]->name != "_Netcdf4Dimid") {
              nc_type type = NCtypeConversion(variable->attributes[i]->getType());
              if (variable->attributes[i]->name == "_FillValue") {
                type = variable->getType();
              }
              status = nc_put_att(root_id, nc_var_id, variable->attributes[i]->name.c_str(), type, variable->attributes[i]->length, variable->attributes[i]->data);
              if (listNCCommands) {

                void *data = variable->attributes[i]->data;
                const size_t length = variable->attributes[i]->length;
                if (type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE) {
                  std::string out = "";
                  out.append((const char *)data, length);
                  CT::printfconcat(NCCommands, "nc_put_att(root_id, var_id_%zu, \"%s\",%s,%zu,\"%s\");\n", j, variable->attributes[i]->name.c_str(), NCtypeConversionToString(type).c_str(),
                                   variable->attributes[i]->length, out.c_str());
                } else {
                  if (type == CDF_INT || type == CDF_UINT) {
                    CT::printfconcat(NCCommands, "int attrData_%zu_%zu[]={", j, i);
                    for (size_t n = 0; n < length; n++) {
                      CT::printfconcat(NCCommands, "%d", ((int *)data)[n]);
                      if (n < length - 1) {
                        CT::printfconcat(NCCommands, ",");
                      }
                      CT::printfconcat(NCCommands, "};\n");
                    }
                  }

                  if (type == CDF_INT64 || type == CDF_UINT64) {
                    CT::printfconcat(NCCommands, "int attrData_%zu_%zu[]={", j, i);
                    for (size_t n = 0; n < length; n++) {
                      CT::printfconcat(NCCommands, "%ld", ((long *)data)[n]);
                      if (n < length - 1) {
                        CT::printfconcat(NCCommands, ",");
                      }
                      CT::printfconcat(NCCommands, "};\n");
                    }
                  }

                  if (type == CDF_SHORT || type == CDF_USHORT) {
                    CT::printfconcat(NCCommands, "short attrData_%zu_%zu[]={", j, i);
                    for (size_t n = 0; n < length; n++) {
                      CT::printfconcat(NCCommands, "%d", ((short *)data)[n]);
                      if (n < length - 1) {
                        CT::printfconcat(NCCommands, ",");
                      }
                      CT::printfconcat(NCCommands, "};\n");
                    }
                  }

                  if (type == CDF_FLOAT) {
                    CT::printfconcat(NCCommands, "float attrData_%zu_%zu[]={", j, i);
                    for (size_t n = 0; n < length; n++) {
                      CT::printfconcat(NCCommands, "%f", ((float *)data)[n]);
                      if (n < length - 1) {
                        CT::printfconcat(NCCommands, ",");
                      }
                      CT::printfconcat(NCCommands, "};\n");
                    }
                  }

                  if (type == CDF_DOUBLE) {
                    CT::printfconcat(NCCommands, "double attrData_%zu_%zu[]={", j, i);
                    for (size_t n = 0; n < length; n++) {
                      CT::printfconcat(NCCommands, "%f", ((double *)data)[n]);
                      if (n < length - 1) {
                        CT::printfconcat(NCCommands, ",");
                      }
                      CT::printfconcat(NCCommands, "};\n");
                    }
                  }
                  CT::printfconcat(NCCommands, "nc_put_att(root_id, var_id_%zu, \"%s\",%s,%zu,attrData_%zu_%zu);\n", j, variable->attributes[i]->name.c_str(), NCtypeConversionToString(type).c_str(),
                                   variable->attributes[i]->length, j, i);
                }
              }

              if (status != NC_NOERR) {
                CDBError("Trying to write attribute %s with type %s for variable %s with type %s\nnc_put_att: %s", variable->attributes[i]->name.c_str(),
                         CDF::getCDFDataTypeName(variable->attributes[i]->getType()).c_str(), variable->name.c_str(), CDF::getCDFDataTypeName(variable->currentType).c_str(), nc_strerror(status));
                return 1;
              }
            }
          }
          if ((numDims > 0 && writeData == true)) {
            bool needsDimIteration = false;
            const int iterativeDimIndex = variable->getIterativeDimIndex();
            if (iterativeDimIndex != -1) needsDimIteration = true;
            if (variable->isDimension) needsDimIteration = false;
            std::vector<size_t> start(variable->dimensionlinks.size()), count(variable->dimensionlinks.size());
            for (size_t j = 0; j < variable->dimensionlinks.size(); j++) {
              start[j] = 0;
              count[j] = variable->dimensionlinks[j]->getSize();
            }
            status = nc_enddef(root_id);
            if (status != NC_NOERR) {
              CDBError("For variable %s:", variable->name.c_str());
              CDBError("[%s]: %s %d", nc_strerror(status), "nc_enddef: ", status);
              return 1;
            }
            if (listNCCommands) {
              CT::printfconcat(NCCommands, "nc_enddef(root_id);\n");
            }

            if (CCDFNETCDFWRITER_DEBUG) {
              CDBDebug("--- Copying Variable %s. needsDimIteration = %d---", variable->name.c_str(), needsDimIteration);
            }
            if (needsDimIteration == false) {
              const int status = copyVar(variable, nc_var_id, start.data(), count.data());
              if (status != 0) return status;
            } else {

              for (size_t id = 0; id < variable->dimensionlinks[iterativeDimIndex]->getSize(); id++) {

                std::string progressMessage;
                progressMessage = CT::printf("\"%d/%zu iterating dim %s with index %zu/%zu for variable %s\"", nrVarsWritten + 1, cdfObject->variables.size(),
                                             variable->dimensionlinks[iterativeDimIndex]->name.c_str(), id, variable->dimensionlinks[iterativeDimIndex]->getSize(), variable->name.c_str());

                const float varPercentage = float(nrVarsWritten) / float(cdfObject->variables.size());
                const float dimPercentage = (float(id) / float(variable->dimensionlinks[iterativeDimIndex]->getSize())) / float(cdfObject->variables.size());
                const float percentage = (varPercentage + dimPercentage) * 100;

                if (CCDFNETCDFWRITER_DEBUG) {
                  CDBDebug("%s", progressMessage.c_str());
                }
                (*progress)(progressMessage.c_str(), percentage);
                start[iterativeDimIndex] = id;
                count[iterativeDimIndex] = 1;

                const int status = copyVar(variable, nc_var_id, start.data(), count.data());

                if (status != 0) return status;
              }
            }
            nc_sync(root_id);
            if (listNCCommands) {
              CT::printfconcat(NCCommands, "nc_sync(root_id);\n");
            }
          }

          nrVarsWritten++;
        }
      }
    }
  }
  return 0;
};

int CDFNetCDFWriter::copyVar(CDF::Variable *variable, int nc_var_id, size_t *start, size_t *count) {
  std::vector<ptrdiff_t> stride(variable->dimensionlinks.size(), 1);

  if (readData == true) {

    // TODO should read iterative for iterative dims.

    status = variable->readData(variable->currentType, start, count, stride.data());
    if (status != 0) {
      CDBError("Reading of variable %s failed", variable->name.c_str());
      return 1;
    }
    if (variable->data == NULL) {
      CDBError("variable->data == NULL for variable %s", variable->name.c_str());
      return 1;
    }
    if (CCDFNETCDFWRITER_DEBUG) {
      CDBDebug("Variable %s read", variable->name.c_str());
    }

    // Apply longitude warping of the data
    // EG 0-360 to -180 till -180
  }
  if (status == 0) {
    if (variable->data == NULL) {
      CDBError("variable->data==NULL for %s", variable->name.c_str());
      return 1;
    }
    if (CCDFNETCDFWRITER_DEBUG) {
      for (size_t i = 0; i < variable->dimensionlinks.size(); i++) {
        CDBDebug("Writing %s,%zu: %zu %zu\t\t[%zu]", variable->name.c_str(), i, start[i], count[i], variable->getSize());
      }
    }

    status = nc_put_vara(root_id, nc_var_id, start, count, variable->data);
    if (listNCCommands) {
      CT::printfconcat(NCCommands, "//");
      for (auto *dimensionlink: variable->dimensionlinks) {
        CT::printfconcat(NCCommands, "%s\t", dimensionlink->name.c_str());
      }
      CT::printfconcat(NCCommands, "\n");
      for (size_t j = 0; j < variable->dimensionlinks.size(); j++) {
        CT::printfconcat(NCCommands, "start[%zu]=%zu;\t", j, start[j]);
      }
      CT::printfconcat(NCCommands, "\n");
      for (size_t j = 0; j < variable->dimensionlinks.size(); j++) {
        CT::printfconcat(NCCommands, "count[%zu]=%zu;\t", j, count[j]);
      }
      CT::printfconcat(NCCommands, "\n");
      CT::printfconcat(NCCommands, "//variable_data should be defined here\n");
      CT::printfconcat(NCCommands, "//nc_put_vara(root_id,var_id_%d,start,count,variable_data);\n", nc_var_id);
    }
    if (status != NC_NOERR) {
      CDBError("For variable %s:", variable->name.c_str());
      CDBError("[%s]: %s %d", nc_strerror(status), "nc_put_var: ", status);
      return 1;
    }
  }
  // Free the variable data
  if (readData == true) {
    if (CCDFNETCDFWRITER_DEBUG) {
      CDBDebug("Free variable %s", variable->name.c_str());
    }
    if (!variable->isDimension) variable->freeData();
  }
  return 0;
};
