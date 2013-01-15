#include "CDFObjectStore.h"
const char *CDFObjectStore::className="CDFObjectStore";
#include "CConvertASCAT.h"
#include "CConvertADAGUCVector.h"
#include "CConvertADAGUCPoint.h"
#include "CDataReader.h"

#define MAX_OPEN_FILES 128
extern CDFObjectStore cdfObjectStore;
CDFObjectStore cdfObjectStore;
bool EXTRACT_HDF_NC_VERBOSE = false;
/**
 * Get a CDFReader based on information in the datasource. In the Layer element this can be configured with <DataReader>HDF5</DataReader>
 * @param dataSource The configured datasource or NULL pointer. NULL pointer defaults to a NetCDF/OPeNDAP reader
 */
CDFReader *CDFObjectStore::getCDFReader(CDataSource *dataSource,const char *fileName){
  //Do we have a datareader defined in the configuration file?
  //if(cdfReader !=NULL){delete cdfReader;cdfReader = NULL;}
  CDFReader *cdfReader = NULL;
  
  // CDFObject *cdfObject=dataSource->dataObject[0]->cdfObject;
  if(dataSource!=NULL){
    if(dataSource->cfgLayer->DataReader.size()>0){
      if(dataSource->cfgLayer->DataReader[0]->value.equals("HDF5")){
        #ifdef CDATAREADER_DEBUG
        CDBDebug("Creating HDF5 reader");
        #endif
        cdfReader = new CDFHDF5Reader();
        CDFHDF5Reader * hdf5Reader = (CDFHDF5Reader*)cdfReader;
        hdf5Reader->enableKNMIHDF5toCFConversion();
      }
    }else{
      cdfReader=getCDFReader(fileName);
    }
  }
  //Defaults to the netcdf reader
  if(cdfReader==NULL){
    #ifdef CDATAREADER_DEBUG
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
CDFReader *CDFObjectStore::getCDFReader(const char *fileName){

  CDFReader *cdfReader = NULL;
  if(fileName!=NULL){
    CT::string name=fileName;
    int a=name.indexOf(".h5");
    if(a!=-1){
      if(a==int(name.length())-3){
        if(EXTRACT_HDF_NC_VERBOSE){
          CDBDebug("Creating HDF5 reader");
        }
        cdfReader = new CDFHDF5Reader();
        CDFHDF5Reader * hdf5Reader = (CDFHDF5Reader*)cdfReader;
        hdf5Reader->enableKNMIHDF5toCFConversion();
      }
    }
    if(cdfReader==NULL){
      a=name.indexOf(".he5");
      if(a!=-1){
        if(a==int(name.length())-4){
          if(EXTRACT_HDF_NC_VERBOSE){
            CDBDebug("Creating HDF EOS 5 reader");
          }
          cdfReader = new CDFHDF5Reader();
        }
      }
    }
    if(cdfReader==NULL){
      a=name.indexOf(".hdf");
      if(a!=-1){
        if(a==int(name.length())-4){
          if(EXTRACT_HDF_NC_VERBOSE){
            CDBDebug("Creating HDF reader");
          }
          cdfReader = new CDFHDF5Reader();
        }
      }
    }
  }
  //Defaults to the netcdf reader
  if(cdfReader==NULL){
    if(EXTRACT_HDF_NC_VERBOSE){
      CDBDebug("Creating NetCDF reader %s",fileName);
    }
    cdfReader = new CDFNetCDFReader();
    //((CDFNetCDFReader*)cdfReader)->enableLonWarp(true);
    
  }
  return cdfReader;
}

CDFObject *CDFObjectStore::getCDFObjectHeader(CServerParams *srvParams,const char *fileName){
  
    return getCDFObject(NULL,srvParams,fileName);
}


/**
* Get a CDFObject based with opened and configured CDF reader for a filename/OPeNDAP url and a dataSource.
* @param dataSource The configured datasource or NULL pointer. NULL pointer defaults to a NetCDF/OPeNDAP reader
* @param fileName The filename to read.
*/
CDFObject *CDFObjectStore::getCDFObject(CDataSource *dataSource,const char *fileName){
  return getCDFObject(dataSource,NULL,fileName);
}
  

CDFObject *CDFObjectStore::getCDFObject(CDataSource *dataSource,CServerParams *srvParams,const char *fileName){
  
  CT::string uniqueIDForFile = fileName;
  if(srvParams!=NULL){
    uniqueIDForFile.concat("_header_");
  }
  
  for(size_t j=0;j<fileNames.size();j++){
    if(fileNames[j]->equals(uniqueIDForFile.c_str())){
      #ifdef CDATAREADER_DEBUG                          
      CDBDebug("Found CDFObject with filename %s",uniqueIDForFile.c_str());
      #endif            
      return cdfObjects[j];
    }
  }
  if(cdfObjects.size()>MAX_OPEN_FILES){
    deleteCDFObject(&cdfObjects[0]);
  }
  #ifdef CDATAREADER_DEBUG              
  CDBDebug("Creating CDFObject with filename %s",fileName);
  #endif      
 
  
  //Open the object.
  #ifdef CDATAREADER_DEBUG           
  CDBDebug("Opening %s",fileName);
  #endif
  
  //Open header
  
  CT::string cacheName;
  bool hasCacheLocation = false;

  if(srvParams!=NULL){
    srvParams->getCacheDirectory(&cacheName);
    if(cacheName.length()>0)hasCacheLocation = true;
    //Do not build a cache of cachefiles
    if(cacheName.indexOf(fileName)!=-1){
      CDBDebug("This is already a cache file: %s",fileName);
      hasCacheLocation = false;
    }
  }
  
  
  Cache cache;
  
  if(hasCacheLocation){
    CT::string validFileName(fileName);
    validFileName.replaceSelf(":",""); validFileName.replaceSelf("/",""); 
    cacheName.concat("/");cacheName.concat(&validFileName);cacheName.concat("_ncheader.nc");
    cache.check(cacheName.c_str());
  }

  
  const char *fileLocationToOpen = fileName;

  if(cache.cacheIsAvailable()){
    fileLocationToOpen = cacheName.c_str();
    CDBDebug("Opening from cache: %s",fileLocationToOpen );
  }else{
    fileLocationToOpen = fileName;
    CDBDebug("Opening from file: %s",fileLocationToOpen );
  }
  
  
   //CDFObject not found: Create one
  CDFObject *cdfObject = new CDFObject();
  CDFReader *cdfReader = NULL;
  
  if(dataSource!=NULL){
    cdfReader = CDFObjectStore::getCDFReader(dataSource,fileLocationToOpen);
  }else{
    //Get a reader based on file extension
    cdfReader = CDFObjectStore::getCDFReader(fileLocationToOpen);
  }
  
  if(cdfReader==NULL){
    if(dataSource!=NULL){
      CDBError("Unable to get a reader for source %s",dataSource->cfgLayer->Name[0]->value.c_str());
    }
    throw(1);
  }
  
  cdfObject->attachCDFReader(cdfReader);
  
  
  int status = cdfObject->open(fileLocationToOpen);
  
  if(status!=0){
    //TODO in case of basic/digest authentication, username and password is currently also listed....
    CDBError("Unable to open file '%s'",fileLocationToOpen);
    delete cdfObject;
    delete cdfReader;
    return NULL;
  }
  
  if(cache.saveCacheFile()){
    //Save cache
    CDFNetCDFWriter netCDFWriter(cdfObject);
    netCDFWriter.disableVariableWrite();
    netCDFWriter.setNetCDFMode(4);
    try{
      //TODO disable compression
      CDBDebug("Saving cache file: %s",cache.getCacheFileNameToWrite() );
      netCDFWriter.write(cache.getCacheFileNameToWrite());
    }catch(int e){
      CDBError("Exception %d in writing cache file",e);
      return NULL;
    }
    cache.releaseCacheFile();
  }
  
  
  //Push everything into the store
  fileNames.push_back(new CT::string(uniqueIDForFile.c_str()));
  cdfObjects.push_back(cdfObject);
  cdfReaders.push_back(cdfReader);

  bool level2CompatMode = false;
  
  if(!level2CompatMode)if(CConvertASCAT::convertASCATHeader(cdfObject)==0){level2CompatMode=true;};
  if(!level2CompatMode)if(CConvertADAGUCVector::convertADAGUCVectorHeader(cdfObject)==0){level2CompatMode=true;};
  if(!level2CompatMode)if(CConvertADAGUCPoint::convertADAGUCPointHeader(cdfObject)==0){level2CompatMode=true;};
  
  /*  try{
    cdfObject->getVariable("natural")->setAttributeText("grid_mapping","projection");
    cdfObject->getVariable("time")->setAttributeText("units","seconds since 2012");
  }catch(int e){
  }*/
   
  
  
  return cdfObject;
}
CDFObjectStore *CDFObjectStore::getCDFObjectStore(){return &cdfObjectStore;};

CDFObject *CDFObjectStore::deleteCDFObject(CDFObject **cdfObject){
  for(size_t j=0;j<cdfObjects.size();j++){
    if(cdfObjects[j]==(*cdfObject)){
      //CDBDebug("Closing %s",fileNames[j]->c_str());
      delete cdfObjects[j];cdfObjects[j]=NULL;(*cdfObject)=NULL;
      delete fileNames[j]; fileNames[j] = NULL;
      delete cdfReaders[j];cdfReaders[j] = NULL;
      cdfReaders.erase(cdfReaders.begin()+j);
      cdfObjects.erase(cdfObjects.begin()+j);
      fileNames.erase(fileNames.begin()+j);
      return NULL;
    }
  }
  
  return NULL;
}

/**
 * Clean the CDFObject store and throw away all readers and objects
 */
void CDFObjectStore::clear(){
  
  for(size_t j=0;j<fileNames.size();j++){
    delete fileNames[j]; fileNames[j] = NULL;
    delete cdfObjects[j];cdfObjects[j] = NULL;
    delete cdfReaders[j];cdfReaders[j] = NULL;
  }
  fileNames.clear();
  cdfReaders.clear();
  cdfObjects.clear();
}
