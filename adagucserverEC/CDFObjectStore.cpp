/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#include "CDFObjectStore.h"
const char *CDFObjectStore::className = "CDFObjectStore";
#include "CConvertASCAT.h"
#include "CConvertUGRIDMesh.h"
#include "CConvertADAGUCVector.h"
#include "CConvertADAGUCPoint.h"
#include "CConvertKNMIH5EchoToppen.h"
#include "CConvertCurvilinear.h"
#include "CConvertKNMIH5VolScan.h"
#include "CConvertHexagon.h"
#include "CConvertGeoJSON.h"
#include "CConvertEProfile.h"
#include "CConvertTROPOMI.h"
#include "CConvertLatLonGrid.h"
#include "CConvertLatLonBnds.h"
#include "CDataReader.h"
#include "CCDFCSVReader.h"
// #define CDFOBJECTSTORE_DEBUG
#define MAX_OPEN_FILES 500
extern CDFObjectStore cdfObjectStore;
CDFObjectStore cdfObjectStore;
bool EXTRACT_HDF_NC_VERBOSE = false;
/**
 * Get a CDFReader based on information in the datasource. In the Layer element this can be configured with <DataReader>HDF5</DataReader>
 * @param dataSource The configured datasource or NULL pointer. NULL pointer defaults to a NetCDF/OPeNDAP reader
 */
CDFReader *CDFObjectStore::getCDFReader(CDataSource *dataSource, const char *fileName) {
  // Do we have a datareader defined in the configuration file?
  // if(cdfReader !=NULL){delete cdfReader;cdfReader = NULL;}
  CDFReader *cdfReader = NULL;

  // CDFObject *cdfObject=dataSource->dataObject[0]->cdfObject;
  if (dataSource != NULL) {
    if (dataSource->cfgLayer->DataReader.size() > 0) {
      if (dataSource->cfgLayer->DataReader[0]->value.equals("HDF5")) {
#ifdef CDFOBJECTSTORE_DEBUG
        CDBDebug("Creating HDF5 reader");
#endif
        cdfReader = new CDFHDF5Reader();
        CDFHDF5Reader *hdf5Reader = (CDFHDF5Reader *)cdfReader;
        hdf5Reader->enableKNMIHDF5toCFConversion();
        if (!dataSource->cfgLayer->DataReader[0]->attr.useendtime.empty()) {
          if (dataSource->cfgLayer->DataReader[0]->attr.useendtime.equalsIgnoreCase("true")) {
            hdf5Reader->enableKNMIHDF5UseEndTime();
          }
        }
      } else if (dataSource->cfgLayer->DataReader[0]->value.equals("GEOJSON")) {
#ifdef CDFOBJECTSTORE_DEBUG
        CDBDebug("Creating GEOJSON reader");
#endif
        cdfReader = new CDFGeoJSONReader();
      } else if (dataSource->cfgLayer->DataReader[0]->value.equals("PNG")) {
#ifdef CDFOBJECTSTORE_DEBUG
        CDBDebug("Creating PNG reader");
#endif
        cdfReader = new CDFPNGReader();
      } else if (dataSource->cfgLayer->DataReader[0]->value.equals("CSV")) {
#ifdef CDFOBJECTSTORE_DEBUG
        CDBDebug("Creating CSV reader");
#endif
        cdfReader = new CDFCSVReader();
      }
    } else {
      cdfReader = getCDFReader(fileName);
    }
  }
  // Defaults to the netcdf reader
  if (cdfReader == NULL) {
#ifdef CDFOBJECTSTORE_DEBUG
    CDBDebug("Creating NetCDF reader");
#endif
    cdfReader = new CDFNetCDFReader();
  }
  return cdfReader;
}

/**
 * Get a CDFReader based on fileName information, currently based on extension.
 * @param fileName The fileName
 * @return The CDFReader
 */
CDFReader *CDFObjectStore::getCDFReader(const char *fileName) {

  CDFReader *cdfReader = NULL;
  if (fileName != NULL) {
    CT::string name = fileName;
    if (name.indexOf("http") != 0) {
      int a = name.indexOf(".h5");
      if (a != -1) {
        if (a == int(name.length()) - 3) {
          if (EXTRACT_HDF_NC_VERBOSE) {
            CDBDebug("Creating HDF5 reader");
          }
          cdfReader = new CDFHDF5Reader();
          CDFHDF5Reader *hdf5Reader = (CDFHDF5Reader *)cdfReader;
          hdf5Reader->enableKNMIHDF5toCFConversion();
        }
      }
      if (cdfReader == NULL) {
        a = name.indexOf(".he5");
        if (a != -1) {
          if (a == int(name.length()) - 4) {
            if (EXTRACT_HDF_NC_VERBOSE) {
              CDBDebug("Creating HDF EOS 5 reader");
            }
            cdfReader = new CDFHDF5Reader();
          }
        }
      }
      if (cdfReader == NULL) {
        a = name.indexOf(".hdf");
        if (a != -1) {
          if (a == int(name.length()) - 4) {
            if (EXTRACT_HDF_NC_VERBOSE) {
              CDBDebug("Creating HDF reader");
            }
            cdfReader = new CDFHDF5Reader();
          }
        }
      }
      if (cdfReader == NULL) {
        a = name.indexOf(".json");
        if (a != -1) {
          if (a == int(name.length()) - 5) {
            if (EXTRACT_HDF_NC_VERBOSE) {
              CDBDebug("Creating GeoJSON reader");
            }
            cdfReader = new CDFGeoJSONReader();
          }
        }
      }
      if (cdfReader == NULL) {
        a = name.indexOf(".geojson");
        if (a != -1) {
          if (a == int(name.length()) - 8) {
            if (EXTRACT_HDF_NC_VERBOSE) {
              CDBDebug("Creating GeoJSON reader");
            }
            cdfReader = new CDFGeoJSONReader();
          }
        }
      }
      if (cdfReader == NULL) {
        a = name.indexOf(".png");
        if (a != -1) {
          if (a == int(name.length()) - 4) {
            if (EXTRACT_HDF_NC_VERBOSE) {
              CDBDebug("Creating PNG reader");
            }
            cdfReader = new CDFPNGReader();
          }
        }
      }
      if (cdfReader == NULL) {
        a = name.indexOf(".csv");
        if (a != -1) {
          if (a == int(name.length()) - 4) {
            if (EXTRACT_HDF_NC_VERBOSE) {
              CDBDebug("Creating CSV reader");
            }
            cdfReader = new CDFCSVReader();
          }
        }
      }
    }
  }
  // Defaults to the netcdf reader
  if (cdfReader == NULL) {
    if (EXTRACT_HDF_NC_VERBOSE) {
      CDBDebug("Creating NetCDF reader %s", fileName);
    }
    cdfReader = new CDFNetCDFReader();
    //((CDFNetCDFReader*)cdfReader)->enableLonWarp(true);
  }
  return cdfReader;
}

CDFObject *CDFObjectStore::getCDFObjectHeader(CDataSource *dataSource, CServerParams *srvParams, const char *fileName, bool cached) {
  if (srvParams == NULL) {
    CDBError("srvParams == NULL");
    return NULL;
  }

  return getCDFObject(dataSource, srvParams, fileName, false, cached);
}

CDFObject *CDFObjectStore::getCDFObjectHeaderPlain(CDataSource *dataSource, CServerParams *srvParams, const char *fileName, bool cached) {
  if (srvParams == NULL) {
    CDBError("srvParams == NULL");
    return NULL;
  }

  return getCDFObject(dataSource, srvParams, fileName, true, cached);
}

/**
 * Get a CDFObject based with opened and configured CDF reader for a filename/OPeNDAP url and a dataSource.
 * @param dataSource The configured datasource or NULL pointer. NULL pointer defaults to a NetCDF/OPeNDAP reader
 * @param fileName The filename to read.
 */
CDFObject *CDFObjectStore::getCDFObject(CDataSource *dataSource, const char *fileName, bool cached) { return getCDFObject(dataSource, NULL, fileName, false, cached); }

CDFObject *CDFObjectStore::getCDFObject(CDataSource *dataSource, CServerParams *srvParams, const char *fileName, bool plain, bool cached) {
  CT::string uniqueIDForFile = fileName;

  if (srvParams == NULL && dataSource != NULL) {
    srvParams = dataSource->srvParams;
  }
  if (srvParams == NULL) {
    CDBError("getCDFObject:: srvParams is not set");
    throw(__LINE__);
  }
  if (cached) {
    for (size_t j = 0; j < fileNames.size(); j++) {
      if (fileNames[j]->equals(uniqueIDForFile.c_str())) {
#ifdef CDFOBJECTSTORE_DEBUG
        CDBDebug("Found CDFObject with filename %s", uniqueIDForFile.c_str());
#endif
        return cdfObjects[j];
      }
    }
  }
  if (cdfObjects.size() > MAX_OPEN_FILES) {
    deleteCDFObject(&cdfObjects[0]);
  }
#ifdef CDFOBJECTSTORE_DEBUG
  CDBDebug("Creating CDFObject with id %s", uniqueIDForFile.c_str());
#endif

// Open the object.
#ifdef CDFOBJECTSTORE_DEBUG
  CDBDebug("Opening %s", fileName);
#endif

  // Open header

  const char *fileLocationToOpen = fileName;

  if (EXTRACT_HDF_NC_VERBOSE) {
    CDBDebug("Opening from file: %s", fileLocationToOpen);
  }

  // CDFObject not found: Create one
  CDFObject *cdfObject = new CDFObject();
  CDFReader *cdfReader = NULL;

  if (dataSource != NULL) {
    cdfReader = CDFObjectStore::getCDFReader(dataSource, fileLocationToOpen);
  } else {
    // Get a reader based on file extension
    cdfReader = CDFObjectStore::getCDFReader(fileLocationToOpen);
  }

  if (cdfReader == NULL) {
    if (dataSource != NULL) {
      CDBError("Unable to get a reader for source %s", dataSource->cfgLayer->Name[0]->value.c_str());
    }
    throw(1);
  }

  cdfObject->attachCDFReader(cdfReader);

  int status = 0;
  try {
    status = cdfObject->open(fileLocationToOpen);
  } catch (int e) {
    CDBError("Exception thrown during opening of %s", fileLocationToOpen);
  }

  if (dataSource) {
    if (dataSource->cfgLayer) {
      // Apply NCML file to the datamodel */
      if (dataSource->cfgLayer->FilePath.size() == 1) {
        CT::string ncmlFileName = dataSource->cfgLayer->FilePath[0]->attr.ncml;
        if (!ncmlFileName.empty()) {
          CDBDebug("NCML: Applying NCML file %s", ncmlFileName.c_str());
          cdfObject->applyNCMLFile(ncmlFileName.c_str());
        }
      }
      if (dataSource->cfgLayer->Variable.size() > 0) {
        // Shorthand to variable configuration in the layer.
        auto *cfgVar = dataSource->cfgLayer->Variable[0];
        // Rename variable, if requested
        if (!cfgVar->attr.orgname.empty()) {
          CDF::Variable *var = cdfObject->getVariable(cfgVar->attr.orgname);
          var->name.copy(cfgVar->value);
          // This cdfobject should not be cached, as the variable
          // can no longer be found by other layers using the same cdfobject.
          cached = false;
        }

        // Set long_name
        if (!cfgVar->attr.long_name.empty()) {
          CDF::Variable *var = cdfObject->getVariable(cfgVar->value);
          var->setAttributeText("long_name", cfgVar->attr.long_name);
        }

        // Set units
        if (!cfgVar->attr.units.empty()) {
          CDF::Variable *var = cdfObject->getVariable(cfgVar->value);
          var->setAttributeText("units", cfgVar->attr.units);
        }

        // Set standard_name
        if (!cfgVar->attr.standard_name.empty()) {
          CDF::Variable *var = cdfObject->getVariable(cfgVar->value);
          var->setAttributeText("standard_name", cfgVar->attr.standard_name);
        }
      }
    }
  }

  // CDBDebug("opened");
  if (status != 0) {
    // TODO in case of basic/digest authentication, username and password is currently also listed....
    CDBError("Unable to open file '%s'", fileLocationToOpen);
    delete cdfObject;
    delete cdfReader;
    return NULL;
  }

  // CDBDebug("PUSHING %s",uniqueIDForFile.c_str());
  // Push everything into the store
  if (cached) {
    fileNames.push_back(new CT::string(uniqueIDForFile.c_str()));
    cdfObjects.push_back(cdfObject);
    cdfReaders.push_back(cdfReader);
  }

  if (plain == false) {
    bool formatConverterActive = false;

    if (!formatConverterActive)
      if (CConvertUGRIDMesh::convertUGRIDMeshHeader(cdfObject) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertASCAT::convertASCATHeader(cdfObject) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertADAGUCVector::convertADAGUCVectorHeader(cdfObject) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertADAGUCPoint::convertADAGUCPointHeader(cdfObject) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertKNMIH5EchoToppen::convertKNMIH5EchoToppenHeader(cdfObject) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertCurvilinear::convertCurvilinearHeader(cdfObject, srvParams) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertHexagon::convertHexagonHeader(cdfObject, srvParams) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertGeoJSON::convertGeoJSONHeader(cdfObject) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertEProfile::convertEProfileHeader(cdfObject, srvParams) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertTROPOMI::convertTROPOMIHeader(cdfObject, srvParams) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertLatLonBnds::convertLatLonBndsHeader(cdfObject, srvParams) == 0) {
        formatConverterActive = true;
        CDBDebug("LatLonBnds found!");
      };

    if (!formatConverterActive)
      if (CConvertKNMIH5VolScan::convertKNMIH5VolScanHeader(cdfObject, srvParams) == 0) {
        formatConverterActive = true;
      };

    if (!formatConverterActive)
      if (CConvertLatLonGrid::convertLatLonGridHeader(cdfObject, srvParams) == 0) {
        formatConverterActive = true;
      };
  }

  return cdfObject;
}
CDFObjectStore *CDFObjectStore::getCDFObjectStore() { return &cdfObjectStore; };

void CDFObjectStore::deleteCDFObject(CDFObject **cdfObject) {
  // CDBDebug("Deleting CDFObject  %d", (*cdfObject));
  int numDeleted = 0;
  for (size_t j = 0; j < cdfObjects.size(); j++) {
    if (cdfObjects[j] == (*cdfObject)) {
      // CDBDebug("Closing %s",fileNames[j]->c_str());
      delete cdfObjects[j];
      cdfObjects[j] = NULL;
      delete fileNames[j];
      fileNames[j] = NULL;
      delete cdfReaders[j];
      cdfReaders[j] = NULL;

      cdfObjects.erase(cdfObjects.begin() + j);
      fileNames.erase(fileNames.begin() + j);
      cdfReaders.erase(cdfReaders.begin() + j);

      numDeleted++;
    }
  }
  // CDBDebug("Deleted [%d] CDFObjects",numDeleted);
  if (numDeleted != 0) {
    deleteCDFObject(cdfObject);
  } else {
    (*cdfObject) = NULL;
  }
}

void CDFObjectStore::deleteCDFObject(const char *fileName) {
  // CDBDebug("Deleting CDFObject");
  int numDeleted = 0;
  for (size_t j = 0; j < cdfObjects.size(); j++) {
    if (fileNames[j]->equals(fileName)) {
      // CDBDebug("Closing %s",fileNames[j]->c_str());
      delete cdfObjects[j];
      cdfObjects[j] = NULL;
      delete fileNames[j];
      fileNames[j] = NULL;
      delete cdfReaders[j];
      cdfReaders[j] = NULL;
      cdfReaders.erase(cdfReaders.begin() + j);
      fileNames.erase(fileNames.begin() + j);
      cdfObjects.erase(cdfObjects.begin() + j);
      numDeleted++;
    }
  }
  // CDBDebug("Deleted [%d] CDFObjects",numDeleted);
  if (numDeleted != 0) {
    deleteCDFObject(fileName);
  }
}

/**
 * Clean the CDFObject store and throw away all readers and objects
 */
void CDFObjectStore::clear() {
  for (size_t j = 0; j < fileNames.size(); j++) {
    delete fileNames[j];
    fileNames[j] = NULL;
    delete cdfObjects[j];
    cdfObjects[j] = NULL;
    delete cdfReaders[j];
    cdfReaders[j] = NULL;
  }
  fileNames.clear();
  cdfReaders.clear();
  cdfObjects.clear();
}

CT::StackList<CT::string> CDFObjectStore::getListOfVisualizableVariables(CDFObject *cdfObject) {
  CT::StackList<CT::string> variableList;

  if (cdfObject != NULL) {
    for (size_t j = 0; j < cdfObject->variables.size(); j++) {
      if (cdfObject->variables[j]->dimensionlinks.size() >= 2) {
        if (cdfObject->variables[j]->getAttributeNE("ADAGUC_SKIP") == NULL) {
          if (!cdfObject->variables[j]->name.equals("lon") && !cdfObject->variables[j]->name.equals("lat") && !cdfObject->variables[j]->name.equals("lon_bounds") &&
              !cdfObject->variables[j]->name.equals("lat_bounds") && !cdfObject->variables[j]->name.equals("time_bounds") && !cdfObject->variables[j]->name.equals("lon_bnds") &&
              !cdfObject->variables[j]->name.equals("lat_bnds") && !cdfObject->variables[j]->name.equals("time_bnds") && !cdfObject->variables[j]->name.equals("time")) {
            variableList.push_back(cdfObject->variables[j]->name.c_str());
          }
        }
      }
    }
  }
  return variableList;
}

int CDFObjectStore::getNumberOfOpenObjects() { return cdfObjects.size(); }

int CDFObjectStore::getMaxNumberOfOpenObjects() { return MAX_OPEN_FILES; }
