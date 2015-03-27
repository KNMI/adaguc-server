#include <algorithm>
#include <stdio.h>
#include <netcdf.h>
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CDirReader.h"
#include "CTime.h"

#define VERSION "ADAGUC aggregator 1.6"

DEF_ERRORMAIN()


class NCFileObject{
    public:
    NCFileObject(){
      cdfObject=NULL;
      cdfReader=NULL;
      cdfObject = new CDFObject();
      cdfReader = new CDFNetCDFReader();
      cdfObject->attachCDFReader(cdfReader);
      keep=true;
    }
    ~NCFileObject(){
      delete cdfObject;cdfObject=NULL;
      delete cdfReader;cdfReader=NULL;
    }
    bool keep;
    CDFObject *cdfObject;
    CDFNetCDFReader *cdfReader;
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



  
int main( int argc, const char* argv[]){
  int status = 0;
  //Chunk cache needs to be set to zero, otherwise netcdf runs out of its memory....
  status = nc_set_chunk_cache(0,0,0);
  if(status!=NC_NOERR){
    CDBError("Unable to set nc_set_chunk_cache to zero");
    return 1;
  }
  
  if(argc!=3){
    CDBDebug("Argument count is wrong, please specifiy an input dir and output file");
    return 1;
  }
  
  CT::string inputDir = argv[1];
  CT::string outputFile = argv[2];

  CDirReader dirReader;
  CT::string dirFilter="^.*.*\\.nc";

  dirReader.listDirRecursive(inputDir.c_str(),dirFilter.c_str());
  if(dirReader.fileList.size()==0){
    CDBError("No netcdf files in input directory %s",inputDir.c_str());
    return 1;
  }

  // Create a vector which holds information for all the inputfiles.
  std::vector<NCFileObject *> fileObjects;
  vector<NCFileObject *>::iterator fileObjectsIt;
  
  /* Loop through all files and gather information */
  try{
    for(size_t j=0;j<dirReader.fileList.size();j++){
      NCFileObject *fileObject=new NCFileObject();
      fileObjects.push_back(fileObject);
      fileObject->fullName = dirReader.fileList[j]->fullName.c_str();
      fileObject->baseName = dirReader.fileList[j]->baseName.c_str();
      status = fileObject->cdfObject->open(fileObject->fullName.c_str());
      if(status != 0){CDBError("Unable to read file %s",fileObject->fullName.c_str());throw(__LINE__);}
      CDF::Variable* timeVar = fileObject->cdfObject->getVariable("time");
      timeVar->readData(CDF_DOUBLE);
      double value=((double*)(timeVar->data))[0];
      CT::string units = timeVar->getAttribute("units")->getDataAsString().c_str();
      CTime time;
      time.init(units.c_str());
      CTime::Date date = time.getDate(value);
      CTime epochCTime;
      epochCTime.init("seconds since 1970-01-01 0:0:0");
      fileObject->timeValue=epochCTime.dateToOffset(date);
      CT::string message;
      //message.print("[\"status\":\"Checking files\",\"currentfile\":\"%d\",\"totalfiles\":\"%d\",\"filename\":\"%s\"]",j,dirReader.fileList.size(),fileObject->baseName.c_str());
      message.print("\"Checking file %s\"",fileObject->baseName.c_str());
      progress(message.c_str(),(float(j)/float(dirReader.fileList.size()))*50);
      
      fileObject->keep=true;
      fileObject->cdfObject->close();
    }
  }catch(int linenr){ throw(linenr);}
  
  //Sort the dates according the timeValue
  std::sort (fileObjects.begin(), fileObjects.end(), NCFileObject::sortFunction);  
  
  //CDBDebug("Found %d files",dirReader.fileList.size());

  CT::string netcdfFile=fileObjects[0]->fullName.c_str();
  CT::string netcdfFileBase=fileObjects[0]->baseName.c_str();
  //CDBDebug("Reading %s",netcdfFile.c_str());
  //CDFObject *destCDFObject=fileObjects[0]->cdfObject;
  CDFObject *destCDFObject=new CDFObject();
  CDFNetCDFReader *netcdfReader = new CDFNetCDFReader ();

  destCDFObject->attachCDFReader(netcdfReader);
  status = destCDFObject->open(netcdfFile.c_str());
  if(status != 0){CDBError("Unable to read file %s",netcdfFile.c_str());throw(__LINE__);}
  
   for(size_t j=0;j<fileObjects.size();j++){
     if(destCDFObject->aggregateDim(fileObjects[j]->cdfObject,"time")!=0)throw(__LINE__);
   }
//destCDFObject->aggregateDim(fileObjects[0]->cdfObject,"time");
//destCDFObject->aggregateDim(fileObjects[1]->cdfObject,"time");
//destCDFObject->aggregateDim(fileObjects[2]->cdfObject,"time");

  CDF::Variable * timeVar = destCDFObject->getVariable("time");
  //CDBDebug("timeVar->getSize() %d",timeVar->getSize());

  
  CDFNetCDFWriter *netCDFWriter = new CDFNetCDFWriter(destCDFObject);
  status = netCDFWriter->write(outputFile.c_str(),&progresswrite);
  delete netCDFWriter;
  
  for(size_t j=0;j<fileObjects.size();j++)delete fileObjects[j];
   delete netcdfReader;
   delete destCDFObject;
//   
   

  return 0;
}
