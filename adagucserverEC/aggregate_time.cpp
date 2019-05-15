#include <vector>
#include <iterator>
#include <algorithm>
#include <stdio.h>
#include <netcdf.h>
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CDirReader.h"
#include "CTime.h"

#define VERSION "ADAGUC aggregator 1.6"

DEF_ERRORMAIN()


class NCFileObject{
    public:
    NCFileObject(const char *filename){
      cdfObject=NULL;
      cdfReader=NULL;
      cdfObject = new CDFObject();
      CT::string f=filename;
      if(f.endsWith(".h5")){
        cdfReader = new CDFHDF5Reader();
        ((CDFHDF5Reader*)cdfReader)->enableKNMIHDF5toCFConversion();
      }else{
        cdfReader = new CDFNetCDFReader();
      }
      cdfObject->attachCDFReader(cdfReader);
      keep=true;
    }
    ~NCFileObject(){
      delete cdfObject;cdfObject=NULL;
      delete cdfReader;cdfReader=NULL;
    }
    bool keep;
    CDFObject *cdfObject;
    CDFReader *cdfReader;
    CT::string fullName;
    CT::string baseName;
    double timeValue;
    static bool sortFunction (NCFileObject * i,NCFileObject * j) {return (i->timeValue<j->timeValue); }
  };

void progress(const char*message,float percentage){
  printf("{\"message\":%s,\"percentage\":\"%d\"}\n",message,(int)(percentage));
}

void progresswrite(const char*message,float percentage){
  progress(message,percentage/2.+50);
}

void applyChangesToCDFObject(CDFObject *cdfObject,CT::StackList<CT::string> variablesToDo){
  for(size_t j=0;j<variablesToDo.size();j++){
    CDF::Variable *varWithoutTime = cdfObject->getVariableNE(variablesToDo[j].c_str());
    if(varWithoutTime!=NULL){
      varWithoutTime->dimensionlinks.insert(varWithoutTime->dimensionlinks.begin(),cdfObject->getDimension("time"));
    }else{
      CDBWarning("Variable %s not found");
    }
  }
}

  
int main( int argc, const char* argv[]){
  int status = 0;
  //Chunk cache needs to be set to zero, otherwise netcdf runs out of its memory....
  status = nc_set_chunk_cache(0,0,0);
  if(status!=NC_NOERR){
    CDBError("Unable to set nc_set_chunk_cache to zero");
    return 1;
  }
  
  if(argc!=3&&argc!=4){
    CDBDebug("Argument count is wrong, please specifiy an input dir and output file, plus optionally a comma separated list of variable names to add the time variable to.");
    return 1;
  }
  
  CT::string inputDir = argv[1];
  CT::string outputFile = argv[2];

  CDirReader dirReader;
  CT::string dirFilter="^.*.*\\.nc";

  dirReader.listDirRecursive(inputDir.c_str(),dirFilter.c_str());
  if(dirReader.fileList.size()==0){
    dirFilter="^.*.*\\.h5";
    dirReader.listDirRecursive(inputDir.c_str(),dirFilter.c_str());
      if(dirReader.fileList.size()==0){
      CDBError("No netcdf (*.nc) or hdf5 (*.h5) files files in input directory %s",inputDir.c_str());
      return 1;
    }
  }
  
  
  CT::StackList<CT::string> variablesToAddTimeTo;
  if(argc == 4){
    CT::string variableList = argv[3];
    variablesToAddTimeTo=variableList.splitToStack(",");    
  }
  

  // Create a vector which holds information for all the inputfiles.
  std::vector<NCFileObject *> fileObjects;
  std::vector<NCFileObject *>::iterator fileObjectsIt;
  
  /* Loop through all files and gather information */
  try{
    for(size_t j=0;j<dirReader.fileList.size();j++){
      NCFileObject *fileObject=new NCFileObject(CT::string(dirReader.fileList[j].c_str()).basename().c_str());
      fileObjects.push_back(fileObject);
      fileObject->fullName = dirReader.fileList[j].c_str();
      fileObject->baseName = CT::string(dirReader.fileList[j].c_str()).basename().c_str();

      status = fileObject->cdfObject->open(fileObject->fullName.c_str());
      
      if(status != 0){CDBError("Unable to read file %s",fileObject->fullName.c_str());throw(__LINE__);}
      
      CDF::Variable* timeVar = fileObject->cdfObject->getVariableNE("time");
      if(timeVar == NULL){
        CDBError("Unable to find time variable");
        throw(__LINE__);
      }
      
      timeVar->readData(CDF_DOUBLE);
      double value=((double*)(timeVar->data))[0];
      CT::string units;
      try{
        units = timeVar->getAttribute("units")->getDataAsString().c_str();
      }catch(int e){
        CDBError("Unable to find units attribute for time variable");
        throw(__LINE__);
      }
      
      CTime time;
      time.init(timeVar);
      CTime::Date date = time.getDate(value);
      CTime epochCTime;
      epochCTime.init("seconds since 1970-01-01 0:0:0",NULL);
      
      fileObject->timeValue=epochCTime.dateToOffset(date);
      
      CT::string message;
      //message.print("[\"status\":\"Checking files\",\"currentfile\":\"%d\",\"totalfiles\":\"%d\",\"filename\":\"%s\"]",j,dirReader.fileList.size(),fileObject->baseName.c_str());
      message.print("\"Checking file (%d/%d) %s, has start date %s\"",j,dirReader.fileList.size(),fileObject->baseName.c_str(),epochCTime.dateToISOString(date).c_str());
      progress(message.c_str(),(float(j)/float(dirReader.fileList.size()))*50);
      

         
      fileObject->keep=true;
      fileObject->cdfObject->close();
    }
  }catch(int linenr){ 

    CDBError("Exception at line %d",linenr);
    for(size_t j=0;j<fileObjects.size();j++)delete fileObjects[j];
    return 1;
    
  }

  //Sort the dates according the timeValue
  std::sort (fileObjects.begin(), fileObjects.end(), NCFileObject::sortFunction);  
  
 

  CT::string netcdfFile=fileObjects[0]->fullName.c_str();
  CT::string netcdfFileBase=fileObjects[0]->baseName.c_str();
  //CDBDebug("Reading %s",netcdfFile.c_str());
  //CDFObject *destCDFObject=fileObjects[0]->cdfObject;
  CDFObject *destCDFObject=new CDFObject();
  CDFReader *cdfReader;
  if(netcdfFile.endsWith(".h5")){
    cdfReader = new CDFHDF5Reader();
    ((CDFHDF5Reader*)cdfReader)->enableKNMIHDF5toCFConversion();
  }else{
    cdfReader = new CDFNetCDFReader();
  }
  destCDFObject->attachCDFReader(cdfReader);
  status = destCDFObject->open(netcdfFile.c_str());
  applyChangesToCDFObject(destCDFObject,variablesToAddTimeTo);
  if(status != 0){CDBError("Unable to read file %s",netcdfFile.c_str());throw(__LINE__);}
  
  
  try{
   for(size_t j=0;j<fileObjects.size();j++){
     try{
       //CT::string data = dump(fileObjects[j]->cdfObject); 
//       // printf("%s",data.c_str());
     applyChangesToCDFObject(fileObjects[j]->cdfObject,variablesToAddTimeTo);
     if(destCDFObject->aggregateDim(fileObjects[j]->cdfObject,"time")!=0)throw(__LINE__);
     }catch(int e){
       CDBError("Unable to aggregate dimension for %s",fileObjects[j]->baseName.c_str());
       throw(__LINE__);
     }
   }
  }catch(int e){
    for(size_t j=0;j<fileObjects.size();j++)delete fileObjects[j];
    delete cdfReader;
    delete destCDFObject;
    return 1;
  }
// destCDFObject->aggregateDim(fileObjects[0]->cdfObject,"time");
// destCDFObject->aggregateDim(fileObjects[1]->cdfObject,"time");
// destCDFObject->aggregateDim(fileObjects[2]->cdfObject,"time");

//  CDF::Variable * timeVar = destCDFObject->getVariable("time");
  //CDBDebug("timeVar->getSize() %d",timeVar->getSize());

  
  CDFNetCDFWriter *netCDFWriter = new CDFNetCDFWriter(destCDFObject);
  netCDFWriter->setNetCDFMode(4);
  status = netCDFWriter->write(outputFile.c_str(),&progresswrite);
  delete netCDFWriter;
  
  for(size_t j=0;j<fileObjects.size();j++)delete fileObjects[j];
  delete cdfReader;
  delete destCDFObject;
  
  CCachedDirReader::free();
  CTime::cleanInstances();

  return 0;
}
