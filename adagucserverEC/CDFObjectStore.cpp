#include "CDFObjectStore.h"
const char *CDFObjectStore::className="CDFObjectStore";



extern CDFObjectStore cdfObjectStore;
CDFObjectStore cdfObjectStore;

CDFReader *CDFObjectStore::getCDFReader(CDataSource *dataSource){
  //Do we have a datareader defined in the configuration file?
  //if(cdfReader !=NULL){delete cdfReader;cdfReader = NULL;}
  CDFReader *cdfReader = NULL;
  
  // CDFObject *cdfObject=dataSource->dataObject[0]->cdfObject;
  if(dataSource->cfgLayer->DataReader.size()>0){
    if(dataSource->cfgLayer->DataReader[0]->value.equals("HDF5")){
      #ifdef CDATAREADER_DEBUG
      CDBDebug("Creating HDF5 reader");
      #endif
      cdfReader = new CDFHDF5Reader();
      CDFHDF5Reader * hdf5Reader = (CDFHDF5Reader*)cdfReader;
      hdf5Reader->enableKNMIHDF5toCFConversion();
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


CDFObject *CDFObjectStore::getCDFObject(CDataSource *dataSource,const char *fileName){
  bool returnNew=false;
  if(returnNew==false){
    for(size_t j=0;j<fileNames.size();j++){
      if(fileNames[j]->equals(fileName)){
        #ifdef CDATAREADER_DEBUG                          
        CDBDebug("Found CDFObject with filename %s",fileName);
        #endif            
        return cdfObjects[j];
      }
    }
  }
  #ifdef CDATAREADER_DEBUG              
  CDBDebug("Creating CDFObject with filename %s",fileName);
  #endif      
  //CDFObject not found: Create one
  CDFObject *cdfObject = new CDFObject();
  CDFReader *cdfReader = CDFObjectStore::getCDFReader(dataSource);
  if(cdfReader==NULL){
    CDBError("Unable to get a reader for source %s",dataSource->cfgLayer->Name[0]->value.c_str());
    throw(1);
  }
  cdfObject->attachCDFReader(cdfReader);
  
  if(returnNew==false){
    //Push everything into the store
    fileNames.push_back(new CT::string(fileName));
    cdfObjects.push_back(cdfObject);
    cdfReaders.push_back(cdfReader);
  }
  return cdfObject;
}
CDFObjectStore *CDFObjectStore::getCDFObjectStore(){return &cdfObjectStore;};

CDFObject *CDFObjectStore::deleteCDFObject(CDFObject **cdfObject){
  for(size_t j=0;j<cdfObjects.size();j++){
    if(cdfObjects[j]==(*cdfObject)){
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
