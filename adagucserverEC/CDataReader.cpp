#include "CDataReader.h"
#include <math.h>
const char *CDataReader::className="CDataReader";


#define uchar unsigned char
#define MAX_STR_LEN 8191



void writeLogFile2(const char * msg){
  char * logfile=getenv("ADAGUC_LOGFILE");
  if(logfile!=NULL){
    FILE * pFile = NULL;
    pFile = fopen (logfile , "a" );
    if(pFile != NULL){
      fputs  (msg, pFile );
      if(strncmp(msg,"[D:",3)==0||strncmp(msg,"[W:",3)==0||strncmp(msg,"[E:",3)==0){
        time_t myTime = time(NULL);
        tm *myUsableTime = localtime(&myTime);
        char szTemp[128];
        snprintf(szTemp,127,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ ",
                 myUsableTime->tm_year+1900,myUsableTime->tm_mon+1,myUsableTime->tm_mday,
                 myUsableTime->tm_hour,myUsableTime->tm_min,myUsableTime->tm_sec
                );
        fputs  (szTemp, pFile );
      }
      fclose (pFile);
    }//else CDBError("Unable to write logfile %s",logfile);
  }
}

/*void printStatus(const char *status,const char *a,...){
  va_list ap;
  char message[8192+1];
  va_start (ap, a);
  vsnprintf (message, 8192, a, ap);
  va_end (ap);
  message[8192]='\0';

  size_t s=strlen(message);
  size_t statuslen=strlen(status);
  char outMessage[80];
  char newStatus[statuslen+4];
  snprintf(newStatus,statuslen+3,"[%s]",status);
  statuslen=strlen(newStatus);
  int m=s;if(m>79)m=79;
  strncpy(outMessage,message,m);
  for(int j=m;j<80;j++)outMessage[j]=' ';
  
  strncpy(outMessage+79-statuslen,newStatus,statuslen);
  outMessage[79]='\0';
  printf("%s\n",outMessage);
}*/






int CDataReader::getCacheFileName(CDataSource *dataSource,CT::string *uniqueIDFor2DField){
  if(dataSource==NULL)return 1;
  if(dataSource->srvParams==NULL)return 1;
#ifdef CDATAREADER_DEBUG    
  CDBDebug("GetCacheFileName");
#endif
  
  CT::string cacheLocation;dataSource->srvParams->getCacheDirectory(&cacheLocation);
  if(cacheLocation.c_str()==NULL)return 1;else if(cacheLocation.length()==0)return 1;  
#ifdef CDATAREADER_DEBUG  
  CDBDebug("/GetCacheFileName");
#endif  
  uniqueIDFor2DField->copy(cacheLocation.c_str());
  uniqueIDFor2DField->concat("/");
  struct stat stFileInfo;
  int timeStep = dataSource->getCurrentTimeStep();
  int intStat = stat(uniqueIDFor2DField->c_str(),&stFileInfo);
  //Directory structure needs to be created for all the cache files.
  if(intStat != 0){
    CDBDebug("making dir %s",uniqueIDFor2DField->c_str());
    mode_t permissions = S_IRWXU|S_IRWXG|S_IRWXO;
    mkdir (uniqueIDFor2DField->c_str(),permissions);
    chmod(uniqueIDFor2DField->c_str(),0777);
  }
  
  if(dataSource->getFileName()==NULL){
     CDBError("No filename for datasource");
    return 1;
  }
  
  //Make the cache unique directory name, based on the filename
  CT::string validFileName(dataSource->getFileName());
  //Replace : and / by nothing, so we can use the string as a directory name
  validFileName.replace(":",""); 
  validFileName.replace("/",""); 
  //Concat the filename to the cache directory
  uniqueIDFor2DField->concat(&validFileName);
  uniqueIDFor2DField->concat("cache");

  //Check wether the specific cache file directory exists
  intStat = stat(uniqueIDFor2DField->c_str(),&stFileInfo);
  if(intStat != 0){
    CDBDebug("making dir %s",uniqueIDFor2DField->c_str());
    mode_t permissions = S_IRWXU|S_IRWXG|S_IRWXO;
    mkdir (uniqueIDFor2DField->c_str(),permissions);
    chmod(uniqueIDFor2DField->c_str(),0777);
  }
  
  
  //Now make the filename, based on variable name and dimension properties
  uniqueIDFor2DField->concat("/");
  uniqueIDFor2DField->concat(dataSource->dataObject[0]->variableName.c_str());
#ifdef CDATAREADER_DEBUG    
  CDBDebug("Add dimension properties to the filename");
#endif  
  //Add dimension properties to the filename
  if(dataSource->timeSteps[timeStep]->dims.dimensions.size()>0){
    for(size_t j=0;j<dataSource->timeSteps[timeStep]->dims.dimensions.size();j++){
      uniqueIDFor2DField->printconcat("_[%s=%d]", 
                                      dataSource->timeSteps[timeStep]->dims.dimensions[j]->name.c_str(),
                                      dataSource->timeSteps[timeStep]->dims.dimensions[j]->index);
    }
  }
  
  
  return 0;
}

/**
 * Function to apply quickly scale and offset to a float data array
 * Is static so it can be used in a multithreaded way.
 */

/*class ApplyScaleAndOffsetFloatSettings{
  public:
    size_t start;size_t stop;float *data;float scale;float offset;
};

void *applyScaleAndOffsetFloatThread(void *vsettings){

  ApplyScaleAndOffsetFloatSettings *settings=(ApplyScaleAndOffsetFloatSettings *)vsettings;
  size_t start = settings->start;
  size_t stop = settings->stop;
  float *data = settings->data;
  float scale = settings ->scale;
  float offset = settings->offset;
  char msg[256];
  sprintf(msg,"sf %d - %d == %d\n",start,stop,stop-start);
  writeLogFile2(msg);
          
  for(size_t j=start;j<stop;j++){
    data[j]=data[j]*scale+offset;
  }
  return NULL;
}

int applyScaleAndOffsetFloat(size_t start,size_t stop,float *data,float scale,float offset){
  size_t numThreads=2;
  int errcode;
  int errorOccured = 0;
  int blocksDone = 0;
  int blockSize = (stop-start)/numThreads;
  if(blockSize <=0 )return 1;
  
  pthread_t threads[numThreads];
  ApplyScaleAndOffsetFloatSettings settings[numThreads];
  try{
    for(int j=0;j<numThreads;j++){
      settings[j].start=start+blocksDone;
      settings[j].stop=start+blocksDone+blockSize;
      blocksDone+=blockSize;
      if(j==numThreads-1)settings[j].stop=stop;
      settings[j].data=data;
      settings[j].scale=scale;
      settings[j].offset=offset;
      errcode=pthread_create(&threads[j],NULL,applyScaleAndOffsetFloatThread,(void*)(&settings[j]));
      if(errcode){throw(__LINE__);}
    }
  }catch(int line){
    errorOccured=1;
  }
  for (size_t j=0; j<numThreads; j++) {
    errcode=pthread_join(threads[j],NULL);
    if(errcode){ return 1;}
  }
  return errorOccured;
}*/





int CDataReader::open(CDataSource *_dataSource, int mode){
  //Perform some checks on pointers
  
  if(_dataSource==NULL){CDBError("Invalid dataSource");return 1;}
  if(_dataSource->getFileName()==NULL){CDBError("Invalid NetCDF filename (NULL)");return 1;}
  dataSource=_dataSource;
  FileName.copy(dataSource->getFileName());

  
  int status=0;
  bool enableDataCache=false;
  if(dataSource->cfgLayer->Cache.size()>0){
    if(dataSource->cfgLayer->Cache[0]->attr.enabled.equals("true")){
      enableDataCache=true;
    }
  }
 
  bool cacheAvailable=false;
  bool workingOnCache=false;
  bool saveFieldFile = false;
  CT::string uniqueIDFor2DField;
  CT::string uniqueIDFor2DFieldTmp;
  
  //Get cachefilename
  if(enableDataCache==true){
    if(getCacheFileName(dataSource,&uniqueIDFor2DField)!=0){
      enableDataCache=false;
    }
  }

 
  
  //Check wether we should use cache or not (in case of OpenDAP, this speeds up things a lot)
  if(enableDataCache==true){
    //Create a temporary filename, which we can move in other to avoid read/write conflicts.
    uniqueIDFor2DFieldTmp.copy(&uniqueIDFor2DField);  
    uniqueIDFor2DFieldTmp.concat("_tmp");

    
    struct stat stFileInfo;
    //Test wether the cache file is already available (in this case we do not need above tmp name)
    int intStat = stat(uniqueIDFor2DField.c_str(),&stFileInfo);if(intStat != 0)saveFieldFile = true;else {
      cacheAvailable=true;
    }
    
    //Test wether the tmp file already exists, in this case another process is working on the cache
    intStat = stat(uniqueIDFor2DFieldTmp.c_str(),&stFileInfo);
    if(intStat == 0){workingOnCache=true;}
    //This means that cache is not yet available, but also we do not need to create a cache file.
    if(workingOnCache==true){
      CDBDebug("*** %s exists: Another process is now working on the cache file. The cache can not be used for the moment.",uniqueIDFor2DFieldTmp.c_str());
      saveFieldFile=false;
      cacheAvailable=false;
    }
  }
  
#ifdef MEASURETIME
  StopWatch_Stop("opening file");
#endif
  CDFObject *cdfObject = NULL;
  if(cacheAvailable==true){
#ifdef CDATAREADER_DEBUG        
    CDBDebug("Reading from Cache file");
#endif
    cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,uniqueIDFor2DField.c_str());
    if(cdfObject==NULL){return 1;}
    status = cdfObject->open(uniqueIDFor2DField.c_str());
    int timeStep = dataSource->getCurrentTimeStep();
    for(size_t j=0;j<dataSource->timeSteps[timeStep]->dims.dimensions.size();j++){
      dataSource->timeSteps[timeStep]->dims.dimensions[0]->index=0;
    }
    
    
    

  }else{
    //We just open the file in the standard way, without cache
    cdfObject=CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,FileName.c_str());
    if(cdfObject==NULL){return 1;}
#ifdef CDATAREADER_DEBUG            
    CDBDebug("Reading directly without Cache: %s",FileName.c_str());
#endif    
    status = cdfObject->open(FileName.c_str());
  }
   if(status != 0){CDBError("Unable to read file %s",FileName.c_str());return 1;}
#ifdef MEASURETIME
  StopWatch_Stop("file opened");
#endif
  //Used for internal access:
  thisCDFObject=cdfObject;
/* CDFObject *cdfObject=CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,FileName.c_str(),false);
  thisCDFObject=cdfObject;
  if(cdfObject==NULL){return 1;}*/

   
 // dataSource->cdfObject=cdfObject;
  
  CDF::Variable *var[dataSource->dataObject.size()+1];

  if(dataSource->attachCDFObject(cdfObject)!=0)return 1;
  for(size_t varNr=0;varNr<dataSource->dataObject.size();varNr++){
    
    var[varNr] = dataSource->dataObject[varNr]->cdfVariable;//cdfObject->getVariableNE(dataSource->dataObject[varNr]->variableName.c_str());
    
    //if(var[varNr]==NULL){
     // CDBError("Variable %s does not exist",dataSource->dataObject[varNr]->variableName.c_str());
      //return 1;
      /*CT::string dumpString;
      CDF::dump(cdfObject,&dumpString);
      CDBError("dump:%s\n",dumpString.c_str());
      return 1;*/
    //}
    //Fill in CDF pointers
    
    //dataSource->dataObject[varNr]->cdfVariable=var[varNr];
    
    if(varNr==0){
      if(dataSource->cfgLayer->Dimension.size()==0){
        CDBDebug("Auto configuring dims");
        autoConfigureDimensions(dataSource,true);
      }
    }
  

    //Check if our variable has a statusflag
    std::vector<CDataSource::StatusFlag*> *statusFlagList=&dataSource->dataObject[0]->statusFlagList;
   
    
    CDF::Variable * var=dataSource->dataObject[0]->cdfVariable;
    CDataSource::readStatusFlags(var,statusFlagList);
    
    bool hasStatusFlag=false;
    if(statusFlagList->size()>0)hasStatusFlag=true;
    dataSource->dataObject[0]->hasStatusFlag=hasStatusFlag;
    //CDBDebug("Getting info for variable %s",dataSource->dataObject[varNr]->variableName.c_str());
  }

  // It is possible to skip every N cell in x and y. When set to 1, all data is displayed.
  // When set to 2, every second datacell is displayed, etc...
  
 
  // Retrieve X, Y Dimensions and Width, Height
  dataSource->dNetCDFNumDims = var[0]->dimensionlinks.size();
  int dimXIndex=dataSource->dNetCDFNumDims-1;
  int dimYIndex=dataSource->dNetCDFNumDims-2;
  
  bool swapXYDimensions = false;

  //If our X dimension has a character y/lat in it, XY dims are probably swapped.
  CT::string dimensionXName=var[0]->dimensionlinks[dimXIndex]->name.c_str();
  dimensionXName.toLowerCase();
  if(dimensionXName.indexOf("y")!=-1||dimensionXName.indexOf("lat")!=-1)swapXYDimensions=true;
  //swapXYDimensions=true;
  if(swapXYDimensions){
    dimXIndex=dataSource->dNetCDFNumDims-2;
    dimYIndex=dataSource->dNetCDFNumDims-1;
  }
  
  CDF::Dimension *dimX=var[0]->dimensionlinks[dimXIndex];
  CDF::Dimension *dimY=var[0]->dimensionlinks[dimYIndex];
  if(dimX==NULL||dimY==NULL){CDBError("X and or Y dims not found...");return 1;}
  
  int stride2DMap=1;
  while(dimX->length/stride2DMap>360){
    stride2DMap++;
  }
  
  //When we are reading from cache, the file has been written based on strided data
  if(cacheAvailable){
    stride2DMap=1;
  }
  stride2DMap=1;
  
  dataSource->dWidth=dimX->length/stride2DMap;
  dataSource->dHeight=dimY->length/stride2DMap;
  size_t start[dataSource->dNetCDFNumDims+1];
  size_t count[dataSource->dNetCDFNumDims+1];
  ptrdiff_t stride[dataSource->dNetCDFNumDims+1];
  
  //Set X and Y dimensions start, count and stride
  for(int j=0;j<dataSource->dNetCDFNumDims;j++){start[j]=0; count[j]=1;stride[j]=1;}
  count[dimXIndex]=dataSource->dWidth;
  count[dimYIndex]=dataSource->dHeight;
  stride[dimXIndex]=stride2DMap;
  stride[dimYIndex]=stride2DMap;
  //Set other dimensions than X and Y.
  if(dataSource->dNetCDFNumDims>2){
    for(int j=0;j<dataSource->dNetCDFNumDims-2;j++){
      start[j]=dataSource->getDimensionIndex(var[0]->dimensionlinks[j]->name.c_str());//dOGCDimValues[0];// time dim
      //CDBDebug("%s==%d",var[0]->dimensionlinks[j]->name.c_str(),start[j]);
    }
  }
  
  
  //Read X and Y dimension data completely.
  CDF::Variable *varX=cdfObject->getVariableNE(dimX->name.c_str());
  CDF::Variable *varY=cdfObject->getVariableNE(dimY->name.c_str());
  if(varX==NULL||varY==NULL){CDBError("X and or Y vars not found...");return 1;}

  size_t sta[1],sto[1];ptrdiff_t str[1];
  sta[0]=0;str[0]=stride2DMap; sto[0]=dataSource->dWidth;
  //status = cdfReader->readVariableData(varX,CDF_DOUBLE,sta,sto,str);
  //if(varX->data==NULL){
    status = varX->readData(CDF_DOUBLE,sta,sto,str);
    if(status!=0){
      CDBError("Unable to read x dimension");
      return 1;
    }
  //}
  //if(varY->data==NULL){
    sto[0]=dataSource->dHeight;
    status = varY->readData(CDF_DOUBLE,sta,sto,str);if(status!=0){
      CDBError("Unable to read y dimension");
      return 1;
    }
 // }
  
  // Calculate cellsize based on read X,Y dims
  double *dfdim_X=(double*)varX->data;
  double *dfdim_Y=(double*)varY->data;
  dataSource->dfCellSizeX=(dfdim_X[dataSource->dWidth-1]-dfdim_X[0])/double(dataSource->dWidth-1);
  dataSource->dfCellSizeY=(dfdim_Y[dataSource->dHeight-1]-dfdim_Y[0])/double(dataSource->dHeight-1);
  // Calculate BBOX
  dataSource->dfBBOX[0]=dfdim_X[0]-dataSource->dfCellSizeX/2.0f;
  dataSource->dfBBOX[1]=dfdim_Y[dataSource->dHeight-1]+dataSource->dfCellSizeY/2.0f;
  dataSource->dfBBOX[2]=dfdim_X[dataSource->dWidth-1]+dataSource->dfCellSizeX/2.0f;
  dataSource->dfBBOX[3]=dfdim_Y[0]-dataSource->dfCellSizeY/2.0f;;
  
  if(dimensionXName.equals("col")){
    dataSource->dfBBOX[2]=dfdim_X[0]-dataSource->dfCellSizeX/2.0f;
    dataSource->dfBBOX[3]=dfdim_Y[dataSource->dHeight-1]+dataSource->dfCellSizeY/2.0f;
    dataSource->dfBBOX[0]=dfdim_X[dataSource->dWidth-1]+dataSource->dfCellSizeX/2.0f;
    dataSource->dfBBOX[1]=dfdim_Y[0]-dataSource->dfCellSizeY/2.0f;;
  }
  
  //CDBDebug("BBOX:%f %f %f %f",dfdim_X[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
#ifdef MEASURETIME
  StopWatch_Stop("dimensions read");
#endif

  // Retrieve CRS
  
  //Check if projection is overidden in the config file
  if(dataSource->cfgLayer->Projection.size()==1){
    //Read projection information from configuration
    if(dataSource->cfgLayer->Projection[0]->attr.id.c_str()!=NULL){
      dataSource->nativeEPSG.copy(dataSource->cfgLayer->Projection[0]->attr.id.c_str());
    }else{
      dataSource->nativeEPSG.copy("EPSG:4326");
      //dataSource->nativeEPSG.copy("unknown");
    }
    //Read proj4 string
    if(dataSource->cfgLayer->Projection[0]->attr.proj4.c_str()!=NULL){
      dataSource->nativeProj4.copy(dataSource->cfgLayer->Projection[0]->attr.proj4.c_str());
    }
    else {
      dataSource->nativeProj4.copy("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
      //dataSource->nativeProj4.copy("unknown");
    }
  }else{
    // If undefined, set standard lat lon projection
    dataSource->nativeProj4.copy("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
    //dataSource->nativeEPSG.copy("EPSG:4326");
    //dataSource->nativeProj4.copy("unknown");
    dataSource->nativeEPSG.copy("EPSG:4326");
    //Read projection attributes from the file
    CDF::Attribute *projvarnameAttr = var[0]->getAttributeNE("grid_mapping");
    if(projvarnameAttr!=NULL){
      CDF::Variable * projVar = cdfObject->getVariableNE((char*)projvarnameAttr->data);
      if(projVar==NULL){CDBWarning("projection variable '%s' not found",(char*)projvarnameAttr->data);}
      else {
        //Get proj4_params
        CDF::Attribute *proj4Attr = projVar->getAttributeNE("proj4_params");
        if (proj4Attr==NULL) proj4Attr = projVar->getAttributeNE("proj4");
        if(proj4Attr!=NULL)dataSource->nativeProj4.copy((char*)proj4Attr->data);
        //else {CDBWarning("proj4_params not found in variable %s",(char*)projvarnameAttr->data);}
        //Get EPSG_code
        CDF::Attribute *epsgAttr = projVar->getAttributeNE("EPSG_code");
        if(epsgAttr!=NULL){dataSource->nativeEPSG.copy((char*)epsgAttr->data);}else
        {
          //Make a projection code based on PROJ4: namespace
          dataSource->nativeEPSG.print("PROJ4:%s",dataSource->nativeProj4.c_str());
          dataSource->nativeEPSG.encodeURL();
        }
        //else {CDBWarning("EPSG_code not found in variable %s",(char*)projvarnameAttr->data);}
      }
    }
  }
  //CDBDebug("proj4=%s",dataSource->nativeProj4.c_str());
  //Determine dataSource->dataObject[0]->dataType of the variable we are going to read
  for(size_t varNr=0;varNr<dataSource->dataObject.size();varNr++){
    dataSource->dataObject[varNr]->dataType=var[varNr]->type;
    /*if(var[varNr]->type==CDF_CHAR||var[varNr]->type==CDF_BYTE)dataSource->dataObject[varNr]->dataType=CDF_CHAR;
    if(var[varNr]->type==CDF_UBYTE)dataSource->dataObject[varNr]->dataType=CDF_UBYTE;
    
    if(var[varNr]->type==CDF_SHORT||var[varNr]->type==CDF_USHORT)dataSource->dataObject[varNr]->dataType=CDF_SHORT;
    if(var[varNr]->type==CDF_INT||var[varNr]->type==CDF_UINT)dataSource->dataObject[varNr]->dataType=CDF_INT;
    if(var[varNr]->type==CDF_FLOAT)dataSource->dataObject[varNr]->dataType=CDF_FLOAT;
    if(var[varNr]->type==CDF_DOUBLE)dataSource->dataObject[varNr]->dataType=CDF_DOUBLE;
    if(dataSource->dataObject[varNr]->dataType==CDF_NONE){
      CDBError("Invalid dataSource->dataObject[varNr]->dataType");
      return 1;
    }*/
    //Get Unit
    CDF::Attribute *varUnits=var[varNr]->getAttributeNE("units");
    if(varUnits!=NULL){
      dataSource->dataObject[varNr]->units.copy((char*)varUnits->data);
    }else dataSource->dataObject[varNr]->units.copy("");
  
    // Check for packed data / scaleOffsetIsApplied
    CDF::Attribute *scale_factor = var[varNr]->getAttributeNE("scale_factor");
    if(scale_factor!=NULL){
      //Scale and offset will be applied further downwards in this function.
      //We already set it to true, so we know we have to process it.
      dataSource->dataObject[varNr]->scaleOffsetIsApplied=true;
      // Currently two unpacked data types are supported (CF-1.4): float and double
      if(scale_factor->type==CDF_FLOAT){
        dataSource->dataObject[varNr]->dataType=CDF_FLOAT;
      }
      if(scale_factor->type==CDF_DOUBLE){
        dataSource->dataObject[varNr]->dataType=CDF_DOUBLE;
      }
    
      //char dataTypeName[256];
      //CDF::getCDFDataTypeName(dataTypeName,255, dataSource->dataObject[varNr]->dataType);
      //CDBDebug("Dataset datatype for reading = %s sizeof(short)=%d",dataTypeName,sizeof(short));
    
      //Internally we use always double for scale and offset parameters:
      scale_factor->getData(&dataSource->dataObject[varNr]->dfscale_factor,1);
      CDF::Attribute *add_offset = var[varNr]->getAttributeNE("add_offset");
      if(add_offset!=NULL){
        add_offset->getData(&dataSource->dataObject[varNr]->dfadd_offset,1);
      }else dataSource->dataObject[varNr]->dfadd_offset=0;
    }else dataSource->dataObject[varNr]->scaleOffsetIsApplied=false;
    
    //DataPostProc: Here our datapostprocessor comes into action!
    for(size_t dpi=0;dpi<dataSource->cfgLayer->DataPostProc.size();dpi++){
      CServerConfig::XMLE_DataPostProc * proc = dataSource->cfgLayer->DataPostProc[dpi];
       //Algorithm ax+b:
      if(proc->attr.algorithm.equals("ax+b")){
        dataSource->dataObject[varNr]->scaleOffsetIsApplied=true;
        dataSource->dataObject[varNr]->dataType=CDF_DOUBLE;
       
        //Apply offset
        if(proc->attr.b.c_str()!=NULL){
          CT::string b;
          b.copy(proc->attr.b.c_str());
          dataSource->dataObject[varNr]->dfadd_offset+=b.toDouble();
        }
        //Apply scale
        if(proc->attr.a.c_str()!=NULL){
          CT::string a;
          a.copy(proc->attr.a.c_str());
          dataSource->dataObject[varNr]->dfscale_factor*=a.toDouble();
        }
      }
      //Apply units:
      if(proc->attr.units.c_str()!=NULL){
         dataSource->dataObject[varNr]->units=proc->attr.units.c_str();
      }
    }
    
    //Important!: The CDF datamodel needs to now the new datatype as well, in order to do right lon warping.
    var[varNr]->type=dataSource->dataObject[varNr]->dataType;
  }

  if(mode==CNETCDFREADER_MODE_GET_METADATA){
   
    dataSource->nrMetadataItems=0;
    char szTemp[MAX_STR_LEN+1];
    // Find out how many attributes we have
    dataSource->nrMetadataItems=cdfObject->attributes.size();
    //CDBDebug("NC_GLOBAL: %d",dataSource->nrMetadataItems);
    for(size_t i=0;i<cdfObject->variables.size();i++){
      dataSource->nrMetadataItems+=cdfObject->variables[i]->attributes.size();
      //CDBDebug("%s: %d",cdfObject->variables[i]->name.c_str(),cdfObject->variables[i]->attributes.size());
    }
    //Allocate data for the attributes
    if(dataSource->metaData != NULL)delete[] dataSource->metaData;
    dataSource->metaData = new CT::string[dataSource->nrMetadataItems+1];
    // Loop through each variable
    int nrAttributes = 0;
    CT::string *variableName;
    CT::string AttrValue;
    std::vector<CDF::Attribute *> attributes;
    char szType[3];
    for(size_t i=0;i<cdfObject->variables.size()+1;i++){
      if(i==0){
        attributes=cdfObject->attributes;
        variableName=&cdfObject->name;
      }else {
        attributes=cdfObject->variables[i-1]->attributes;
        variableName=&cdfObject->variables[i-1]->name;
      }

      // Loop through each attribute
      szType[1]='\0';
      for(size_t j=0;j<attributes.size();j++){
        CDF::Attribute *attribute;
        size_t m;
        attribute=attributes[j];
        szType[1]='\0';
        AttrValue.copy("");
        switch (attribute->type){
          case CDF_CHAR:{
            szType[0]='C';//char
            AttrValue.copy((char*)attribute->data);
          }
          break;
          case CDF_SHORT:{
            szType[0]='S';//short
            for(m=0; m < attribute->length-1; m++) {
              snprintf( szTemp, MAX_STR_LEN,"%d, ",((short*)attribute->data)[m] );
              AttrValue.concat(szTemp);
            }
            snprintf( szTemp, MAX_STR_LEN,"%d",((short*)attribute->data)[m]);
            AttrValue.concat(szTemp);
          }
          break;
          case CDF_INT:{
            szType[0]='I';//Int
            for(m=0; m < attribute->length-1; m++) {
              snprintf( szTemp, MAX_STR_LEN,"%d, ",((int*)attribute->data)[m] );
              AttrValue.concat(szTemp);
            }
            snprintf( szTemp, MAX_STR_LEN,"%d",((int*)attribute->data)[m]);
            AttrValue.concat(szTemp);
          }
          break;
          case CDF_FLOAT:{
            szType[0]='F';//Float
            for(m=0; m < attribute->length-1; m++) {
              snprintf( szTemp, MAX_STR_LEN,"%f, ",((float*)attribute->data)[m] );
              AttrValue.concat(szTemp);
            }
            snprintf( szTemp, MAX_STR_LEN,"%f",((float*)attribute->data)[m]);
            AttrValue.concat(szTemp);
          }
          break;
          case CDF_DOUBLE:{
            szType[0]='D';//Double
            for(m=0; m < attribute->length-1; m++) {
              snprintf( szTemp, MAX_STR_LEN,"%f, ",((double*)attribute->data)[m] );
              AttrValue.concat(szTemp);
            }
            snprintf( szTemp, MAX_STR_LEN,"%f",((double*)attribute->data)[m]);
            AttrValue.concat(szTemp);
          }
          break;
        }
        snprintf(szTemp,MAX_STR_LEN,"%s#[%s]%s>%s",variableName->c_str(),szType,attribute->name.c_str(),AttrValue.c_str());
        //CDBDebug("variableName %d==%d %s",nrAttributes,dataSource->nrMetadataItems,szTemp);
        dataSource->metaData[nrAttributes++].copy(szTemp);
      }
    }
    return 0;
  }


  if(mode==CNETCDFREADER_MODE_OPEN_ALL){
    // Read the variable
    //char szTemp[1024];
    //CDF::getCDFDataTypeName(szTemp,1023, dataSource->dataObject[0]->dataType);
    //CDBDebug("Datatype: %s",szTemp);
#ifdef MEASURETIME
  StopWatch_Stop("start reading image data");
#endif
  //for(size_t varNr=0;varNr<dataSource->dataObject.size();varNr++)
    
  double dfNoData = 0;
  bool hasNodataValue =false;;
  
  for(size_t varNr=0;varNr<dataSource->dataObject.size();varNr++)
  {
    
    #ifdef MEASURETIME
    StopWatch_Stop("Reading _FillValue");
    #endif
    CDF::Attribute *fillValue = var[varNr]->getAttributeNE("_FillValue");
    if(fillValue!=NULL){
      hasNodataValue=true;
      fillValue->getData(&dfNoData,1);
    }else hasNodataValue=false;
    
    #ifdef CDATAREADER_DEBUG   
    /*CDBDebug("--- varNR [%d], name=\"%s\"",varNr,var[varNr]->name.c_str());
    for(size_t d=0;d<var[varNr]->dimensionlinks.size();d++){
      CDBDebug("%s  \tstart: %d\tcount %d\tstride %d",var[varNr]->dimensionlinks[d]->name.c_str(),start[d],count[d],stride[d]);
    }*/
    #endif
    
    
    #ifdef MEASURETIME
    StopWatch_Stop("Freeing data");
    #endif
    
    //Read variable data
    var[varNr]->freeData();

    #ifdef MEASURETIME
    StopWatch_Stop("start reading data");
    #endif
    
    var[varNr]->readData(dataSource->dataObject[varNr]->dataType,start,count,stride);
    
    #ifdef MEASURETIME
    StopWatch_Stop("data read");
    #endif
    
    
    //Swap X, Y dimensions so that pointer x+y*w works correctly
    if(cacheAvailable!=true){
      //Cached data was already swapped. Because we have stored the result of this object in the cache.
      if(swapXYDimensions){
        size_t imgSize=dataSource->dHeight*dataSource->dWidth;
        size_t w=dataSource->dWidth;size_t h=dataSource->dHeight;size_t x,y;
        void *vd=NULL;                 //destination data
        void *vs=var[varNr]->data;     //source data
        CDFType dataType=dataSource->dataObject[varNr]->dataType;
        //Allocate data for our new memory block
        CDF::allocateData(dataType,&vd,imgSize);
        //TODO This could also be solved using a template. But this works fine.
        switch (dataType){
          case CDF_CHAR  : {char   *s=(char  *)vs;char   *d=(char  *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_BYTE  : {char   *s=(char  *)vs;char   *d=(char  *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_UBYTE : {uchar  *s=(uchar *)vs;uchar  *d=(uchar *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_SHORT : {short  *s=(short *)vs;short  *d=(short *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_USHORT: {ushort *s=(ushort*)vs;ushort *d=(ushort*)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_INT   : {int    *s=(int   *)vs;int    *d=(int   *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_UINT  : {uint   *s=(uint  *)vs;uint   *d=(uint  *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_FLOAT : {float  *s=(float *)vs;float  *d=(float *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_DOUBLE: {double *s=(double*)vs;double *d=(double*)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          default: CDBError("Unknown data type"); return 1;
        }
        //We will replace our old memory block with the new one, but we have to free our old one first.
        free(var[varNr]->data);
        //Replace the memory block.
        var[varNr]->data=vd;
      }
    }
    
    //Apply the scale and offset factor on the data
    if(dataSource->dataObject[varNr]->scaleOffsetIsApplied == true && cacheAvailable == false ){
      double dfscale_factor = dataSource->dataObject[varNr]->dfscale_factor;
      double dfadd_offset = dataSource->dataObject[varNr]->dfadd_offset;
      //CDBDebug("dfNoData=%f",dfNoData*dfscale_factor);

      /*if(dataSource->dataObject[varNr]->dataType==CDF_FLOAT){
        //Preserve the original nodata value, so it remains a nice short rounded number.
        float fNoData=dfNoData;
        // packed data to be unpacked to FLOAT:
        float *_data=(float*)var[varNr]->data;
        for(size_t j=0;j<var[varNr]->getSize();j++){
          //Only apply scale and offset when this actual data (do not touch the nodata)
          if(_data[j]!=fNoData){
            _data[j]*=dfscale_factor;
            _data[j]+=dfadd_offset;
          }
        }
      }*/
      if(dataSource->dataObject[varNr]->dataType==CDF_FLOAT){
        //Preserve the original nodata value, so it remains a nice short rounded number.
        float fNoData=(float)dfNoData;
        float fscale_factor=(float)dfscale_factor;
        float fadd_offset=(float)dfadd_offset;
        // packed data to be unpacked to FLOAT:
        if(fscale_factor!=1.0f||fadd_offset!=0.0f){
          float *_data=(float*)var[varNr]->data;
          size_t l=var[varNr]->getSize();
          for(size_t j=0;j<l;j++){
            _data[j]=_data[j]*fscale_factor+fadd_offset;
          }
        }
        //Convert the nodata type
        dfNoData=fNoData*fscale_factor+fadd_offset;
      }
      
      if(dataSource->dataObject[varNr]->dataType==CDF_DOUBLE){
        // packed data to be unpacked to DOUBLE:
        double *_data=(double*)var[varNr]->data;
        for(size_t j=0;j<var[varNr]->getSize();j++){
          _data[j]=_data[j]*dfscale_factor+dfadd_offset;
        }
        //Convert the nodata type
        dfNoData=dfNoData*dfscale_factor+dfadd_offset;
      }
    }
    
    #ifdef MEASURETIME
    StopWatch_Stop("Scale and offset applied");
    #endif
    
    //Set the pointers to our dataSource
    dataSource->dataObject[varNr]->data=var[varNr]->data;
    dataSource->dataObject[varNr]->dfNodataValue=dfNoData;
    dataSource->dataObject[varNr]->hasNodataValue=hasNodataValue;
    
    /*In order to write good cache files we need to modify
      our cdfobject. Data is now unpacked, so we need to remove
      scale_factor and add_offset attributes and change datatypes 
      of the data and _FillValue 
    */
    //remove scale_factor and add_offset attributes, otherwise they are stored in the cachefile again and reapplied over and over again.
    var[varNr]->removeAttribute("scale_factor");
    var[varNr]->removeAttribute("add_offset");
    
    //Set original var datatype correctly for the cdfobject 
    var[varNr]->type=dataSource->dataObject[varNr]->dataType;
    
    //Reset _FillValue to correct datatype and adjust scale and offset values.
    if(hasNodataValue){
      CDF::Attribute *fillValue = var[varNr]->getAttributeNE("_FillValue");
      if(fillValue!=NULL){
        if(var[varNr]->type==CDF_FLOAT){float fNoData=(float)dfNoData;fillValue->setData(CDF_FLOAT,&fNoData,1);}
        if(var[varNr]->type==CDF_DOUBLE)fillValue->setData(CDF_DOUBLE,&dfNoData,1);
      }
    }
  }
  
  //Use autoscale of legendcolors when the scale factor has been set to zero.
  if(dataSource->legendScale==0.0f)dataSource->stretchMinMax=true;else dataSource->stretchMinMax=false;
  if(dataSource->stretchMinMax){
    if(dataSource->statistics==NULL){
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->calculate(dataSource);
      #ifdef MEASURETIME
      StopWatch_Stop("Calculated statistics");
      #endif
    }
    float min=(float)dataSource->statistics->getMinimum();
    float max=(float)dataSource->statistics->getMaximum();
#ifdef CDATAREADER_DEBUG    
    CDBDebug("Min = %f, Max = %f",min,max);
#endif    
    
    //Make sure that there is always a range in between the min and max.
    if(max==min)max=min+0.1;
    //Calculate legendOffset legendScale
    float ls=240/(max-min);
    float lo=-(min*ls);
    dataSource->legendScale=ls;
    dataSource->legendOffset=lo;
  }
#ifdef MEASURETIME
  StopWatch_Stop("all read");
#endif

  
  //DataPostProc: Here our datapostprocessor comes into action!
  for(size_t dpi=0;dpi<dataSource->cfgLayer->DataPostProc.size();dpi++){
    CServerConfig::XMLE_DataPostProc * proc = dataSource->cfgLayer->DataPostProc[dpi];
    
    //Algorithm msgcppmask
    if(proc->attr.algorithm.equals("msgcppmask")){
      if(dataSource->dataObject.size()!=2){
        CDBError("3 variables are needed for msgcppmask");
      }
      CDBDebug("Applying msgcppmask");
      float *data1=(float*)dataSource->dataObject[0]->data;//sunz
      float *data2=(float*)dataSource->dataObject[1]->data;
      //float *data3=(float*)dataSource->dataObject[2]->data;//azidiff
      float fNodata1=(float)dataSource->dataObject[0]->dfNodataValue;
      //float fNodata3=(float)dataSource->dataObject[2]->dfNodataValue;
      size_t l=(size_t)dataSource->dHeight*(size_t)dataSource->dWidth;
      
      float fa=72,fb=75; 
      
      if(proc->attr.b.c_str()!=NULL){CT::string b;b.copy(proc->attr.b.c_str());fb=b.toDouble();}
      if(proc->attr.a.c_str()!=NULL){CT::string a;a.copy(proc->attr.a.c_str());fa=a.toDouble();}
      
      for(size_t j=0;j<l;j++){
        //if((data2[j]<72&&data1[j]<72&&data3[j]!=fNodata3)||(data2[j]>75))data1[j]=fNodata1;//else data1[j]=1;
        if((data2[j]<fa&&data1[j]<fa)||(data2[j]>fb))data1[j]=fNodata1; else data1[j]=1;
      //  if(data1[j]<43||data1[j]>65)data1[j]=fNodata;//else data1[j]=1;
        //if(data3[j]+data1[j]<60)data1[j]=fNodata;;
       // if(data1[j]<data2[j]-25)data1[j]=fNodata;;
      //  if(data1[j]>data2[j]+25)data1[j]=fNodata;;
       // if((data1[j]*data1[j]*data1[j]+data2[j]*data2[j]*data2[j])>55*55*data2[j])data1[j]=fNodata;
        //data1[j]=sqrt(data1[j]*data1[j]+data2[j]*data2[j]);
      }
      delete dataSource->dataObject[1];
      //delete dataSource->dataObject[2];
      //dataSource->dataObject.erase(dataSource->dataObject.begin()+1);
      dataSource->dataObject.erase(dataSource->dataObject.begin()+1);
    }
    #ifdef MEASURETIME
    StopWatch_Stop("Data postprocessing completed");
    #endif
  }
 

  //Write the cache file if neccessary.
    if(enableDataCache==true&&saveFieldFile==true){

   //   CT::string dumpString;
    //     CDF::dump(cdfObject,&dumpString);
         //CDBDebug("\nSTART\n%s\nEND\n",dumpString.c_str());
    //writeLogFile2(dumpString.c_str());
      
  // CT::string dumpString;
  // CDF::dump(cdfObject,&dumpString);
  // CDBDebug(dumpString.c_str());
  
      //CDFObject newCDFObject
      
  //CDBDebug("Writing data cache file");
      /*if(saveMetadataFile){
  CDBDebug("Writing to %s",uniqueIDFor2DField.c_str());
  CDFNetCDFWriter netCDFWriter(cdfObject);
  netCDFWriter.disableVariableWrite();
  netCDFWriter.write(uniqueIDForMetadata.c_str());
    }*/
  //int keepVariable[dataSource->dataObject.size()];
  int keepVariable[cdfObject->variables.size()];
  int keepDimension[cdfObject->dimensions.size()];
  //By default throw away everything
  for(size_t v=0;v<cdfObject->variables.size();v++)keepVariable[v]=0;
  for(size_t v=0;v<cdfObject->dimensions.size();v++)keepDimension[v]=0;        
  
   //if(saveFieldFile)
  {
    CDBDebug("Writing to %s",uniqueIDFor2DField.c_str());
    
  

    //keep the concerning variable itself
    for(size_t v=0;v<dataSource->dataObject.size();v++){
      CDF::Variable * var=dataSource->dataObject[v]->cdfVariable;
      try{
        keepVariable[cdfObject->getVariableIndex(var->name.c_str())]=1;
      }catch(int e){}
    }
    //Make a list of required dimensions
    
    for(size_t v=0;v<cdfObject->variables.size();v++){
      try{
        if(keepVariable[v]==1){
          //Loop through dim links
          CDF::Variable *var = cdfObject->variables[v];
          for(size_t d=0;d<var->dimensionlinks.size();d++){
            try{
              keepDimension[cdfObject->getDimensionIndex(var->dimensionlinks[d]->name.c_str())]=1;
            }catch(int e){
              CT::string errMsg;
              CDF::getErrorMessage(&errMsg,e);
              CDBDebug("Dim %s %s",var->dimensionlinks[d]->name.c_str(),errMsg.c_str());
              
            }
            try{
              keepVariable[cdfObject->getVariableIndex(var->dimensionlinks[d]->name.c_str())]=1;
            }catch(int e){
              CT::string errMsg;
              CDF::getErrorMessage(&errMsg,e);
              CDBDebug("Var %s %s",var->dimensionlinks[d]->name.c_str(),errMsg.c_str());
              
            }                  
          }
        }
      }catch(int e){}
    }
  //Make a list of required variables
    for(size_t v=0;v<dataSource->dataObject.size();v++){
      for(size_t w=0;w<cdfObject->variables.size();w++){
        //Metadata variables often have no dimensions. We want to keep them to show info to the user.
        if(keepVariable[w]==0){
          if(cdfObject->variables[w]->dimensionlinks.size()<2){
            keepVariable[w]=1;
            /*try{
              keepDimension[cdfObject->getDimensionIndex(cdfObject->variables[w]->name.c_str())]=1;
            }catch(int e){}*/
             cdfObject->variables[w]->dimensionlinks.resize(0);

          }
        }
      }
    }
    std::vector<CT::string *> deleteVarNames;
    //Remove variables
    for(size_t v=0;v<cdfObject->variables.size();v++){
      if(keepVariable[v]==0)deleteVarNames.push_back(new CT::string(&cdfObject->variables[v]->name));
    }
    for(size_t i=0;i<deleteVarNames.size();i++){
      CDBDebug("Removing variable %s",deleteVarNames[i]->c_str());
      cdfObject->removeVariable(deleteVarNames[i]->c_str());
      delete deleteVarNames[i];
    }
    deleteVarNames.clear();
    //Remove dimensions
    for(size_t d=0;d<cdfObject->dimensions.size();d++){
      if(keepDimension[d]==0)deleteVarNames.push_back(new CT::string(&cdfObject->dimensions[d]->name));
    }
    for(size_t i=0;i<deleteVarNames.size();i++){
      CDBDebug("Removing dimensions %s",deleteVarNames[i]->c_str());
      cdfObject->removeDimension(deleteVarNames[i]->c_str());
      delete deleteVarNames[i];
    }
    
    //Adjust dimensions for the single object:
        
    varX->type=CDF_DOUBLE;
    varY->type=CDF_DOUBLE;
    try{
      
      //Reduce dimension sizes because of striding
      cdfObject->getDimension(varX->name.c_str())->setSize(cdfObject->getDimension(varX->name.c_str())->getSize()/stride2DMap);
      cdfObject->getDimension(varY->name.c_str())->setSize(cdfObject->getDimension(varY->name.c_str())->getSize()/stride2DMap);
      CDBDebug("dfdim_X[0] %f-%f",dfdim_X[0],dfdim_X[dataSource->dWidth-1]);
      for(size_t j=0;j<cdfObject->getDimension(varX->name.c_str())->getSize();j++)((double*)varX->data)[j]=dfdim_X[j];
      for(size_t j=0;j<cdfObject->getDimension(varY->name.c_str())->getSize();j++)((double*)varY->data)[j]=dfdim_Y[j];
      
      /*//Calculate cellsize based on read X,Y dims
      double *dfdim_X=(double*)varX->data;
      double *dfdim_Y=(double*)varY->data;
      dataSource->dfCellSizeX=(dfdim_X[dataSource->dWidth-1]-dfdim_X[0])/double(dataSource->dWidth-1);
      dataSource->dfCellSizeY=(dfdim_Y[dataSource->dHeight-1]-dfdim_Y[0])/double(dataSource->dHeight-1);
      // Calculate BBOX
      dataSource->dfBBOX[0]=dfdim_X[0]-dataSource->dfCellSizeX/2.0f;
      dataSource->dfBBOX[1]=dfdim_Y[dataSource->dHeight-1]+dataSource->dfCellSizeY/2.0f;
      dataSource->dfBBOX[2]=dfdim_X[dataSource->dWidth-1]+dataSource->dfCellSizeX/2.0f;
      dataSource->dfBBOX[3]=dfdim_Y[0]-dataSource->dfCellSizeY/2.0f;;*/
    }catch(int e){}
    
    
    int timeStep = dataSource->getCurrentTimeStep();

        /*for(size_t j=0;j<cdfObject->variables.size();j++){
    cdfObject->variables[j]->type=CDF_DOUBLE;
  }*/
    CDBDebug("DONE");  
   
    //Read the dimensions
    for(size_t j=0;j<dataSource->timeSteps[timeStep]->dims.dimensions.size();j++){
      CDF::Dimension *dim = cdfObject->getDimensionNE(dataSource->timeSteps[timeStep]->dims.dimensions[j]->name.c_str());
         
      if(dim!=NULL){
        CDF::Variable *var = cdfObject->getVariableNE(dataSource->timeSteps[timeStep]->dims.dimensions[j]->name.c_str());
        
        if(var==NULL){CDBError("var is null");return 1;}
        CDBDebug("Cache Reading variable %s"    ,var->name.c_str());
        
        //if(var->data==NULL){
        if(var->readData(CDF_DOUBLE)!=0){
          CDBError("Unable to read variable %s",var->name.c_str());
          return 1;
        }
       //}
              
        double dimValue[var->getSize()];
              //dataSource->timeSteps[0]->dims.dimensions[0]->index*CDF::getTypeSize(var->type)
        CDF::dataCopier.copy(dimValue,var->data,var->type,var->getSize());
        size_t index=dataSource->timeSteps[timeStep]->dims.dimensions[0]->index;
              //CDBDebug("dimvalue %s at index %d = %f",var->name.c_str(),index,dimValue[index]);
              
        var->setSize(1);
        dim->length=1;
        ((double*)var->data)[0]=dimValue[index];
      }
            //dataSource->timeSteps[0]->dims.dimensions[0]->index
            //var->setData(var->type,const void *dataToSet,1){
             //dataSource->timeSteps[0]->dims.dimensions[0]->index);
    }

#ifdef CDATAREADER_DEBUG
    
    
       CT::string dumpString;
         CDF::dump(cdfObject,&dumpString);
         //CDBDebug("\nSTART\n%s\nEND\n",dumpString.c_str());
    CDBDebug("DUMP For file to write:");
    writeLogFile2(dumpString.c_str());
#endif
    CDFNetCDFWriter netCDFWriter(cdfObject);
    netCDFWriter.disableReadData();
        
        //netCDFWriter.disableVariableWrite();
        //uniqueIDFor2DField.concat("nep");
    //Is some kind of process working on any of the cache files?
    FILE * pFile = NULL;  
    pFile = fopen ( uniqueIDFor2DFieldTmp.c_str() , "r" );
    if(pFile != NULL){
      fclose (pFile);
      workingOnCache=true;
    }
        
    pFile = fopen ( uniqueIDFor2DField.c_str() , "r" );
    if(pFile != NULL){
      fclose (pFile);
      workingOnCache=true;
    }
        
    if(workingOnCache==false){
      //Imediately claim this file!
      CDBDebug("*** [1/4] Claiming cache file %s",uniqueIDFor2DFieldTmp.c_str());
      const char buffer[] = { "temp_data\n" };
      pFile = fopen ( uniqueIDFor2DFieldTmp.c_str() , "wb" );
      if(pFile==NULL){
        CDBDebug("*** Unable to write to cache file");
        return 1;
      }
      size_t bytesWritten = fwrite (buffer , sizeof(char),10 , pFile );
      fflush (pFile);   
      fclose (pFile);
      if(bytesWritten!=10){
        CDBDebug("*** Failed to Claim the cache  file %d",bytesWritten);
        workingOnCache=true;
      }else {
        CDBDebug("*** [2/4] Succesfully claimed the cache file");
      }
    }
    
    if(workingOnCache==true){
      CDBDebug("*** Another process is working on cache file %s",uniqueIDFor2DFieldTmp.c_str());
      saveFieldFile=false;
      cacheAvailable=false;
    }else{
          
      CDBDebug("*** [3/4] Writing cache file %s",uniqueIDFor2DFieldTmp.c_str());
          
          
      netCDFWriter.setNetCDFMode(4);
      //TODO disable compression
      //netCDFWriter.setDeflate(0);
      try{
        netCDFWriter.write(uniqueIDFor2DFieldTmp.c_str());
        
        if(chmod(uniqueIDFor2DFieldTmp.c_str(),0777)<0){
          CDBError("Unable to change permissions of netcdf cachefile %s",uniqueIDFor2DFieldTmp.c_str());
          return 1;
        }
        
        
      }catch(int e){
        CDBError("Exception %d in writing cache file",e);
        return 1;
      }
          //Move the temp file to the final name
      CDBDebug("Renaming cache file...");
      rename(uniqueIDFor2DFieldTmp.c_str(),uniqueIDFor2DField.c_str());
      CDBDebug("*** [4/4] Writing cache file complete!");

    }
       // return 0;
        
  }
  return 0;
    }
    
    
  }
  
  return 0;
}

int CDataReader::close(){

//  if(cdfReader!=NULL){cdfReader->close();delete cdfReader;cdfReader=NULL;}
//  if(cdfObject!=NULL){delete cdfObject;cdfObject=NULL;}
  /*if(dataSource!=NULL){
    //dataSource->detachCDFObject();
    for(size_t varNr=0;varNr<dataSource->dataObject.size();varNr++){
      ((CDFReader*)dataSource->dataObject[varNr]->cdfObject)->close();
    }
  }*/
  return 0;
}
int CDataReader::getTimeUnit(char * pszTime){
  CDF::Variable *timeVar = getTimeVariable();
  if(timeVar==NULL)return 1;
  CDF::Attribute *timeUnits = timeVar->getAttributeNE("units");
  if(timeUnits ==NULL){return 1;}
  snprintf(pszTime,MAX_STR_LEN,"%s",(char*)timeUnits->data);
  return 0;
  /*
  //TODO We assume that the first configured DIM is always time. This might be not the case!
  if(dataSource->isConfigured==false){
    CDBError("dataSource is not configured");
    return 1;
  }
  CDFObject *cdfObject=dataSource->dataObject[0]->cdfObject;
  if(cdfObject==NULL){
    CDBError("CNetCDFReader is not opened");
    return 1;
  }
  if(dataSource->cfgLayer->Dimension.size()==0){
    pszTime[0]='\0';
    return 0;
  }
  pszTime[0]='\0';
  CDF::Variable *time = cdfObject->getVariableNE(dataSource->cfgLayer->Dimension[0]->attr.name.c_str());
  if(time==NULL){return 1;}
  CDF::Attribute *timeUnits = time->getAttributeNE("units");
  if(timeUnits ==NULL){return 1;}
  snprintf(pszTime,MAX_STR_LEN,"%s",(char*)timeUnits->data);
  return 0;*/
}

int CDataReader::getTimeDimIndex( CDFObject *cdfObject, CDF::Variable * var){
  if(var == NULL || cdfObject == NULL)return -1;
  if(var->dimensionlinks.size()==0){return -1;}
  CT::string standard_name;
  //First try to retrieve the time dimension by standard_name
  for(size_t j=0;j<var->dimensionlinks.size();j++){
    try{
      cdfObject->getVariable(var->dimensionlinks[j]->name.c_str())->getAttribute("standard_name")->getDataAsString(&standard_name);
      if(standard_name.equals("time"))return j;
    }catch(int e){
    }
  }
  //Second, if failed try to find it based on variable name.
  for(size_t j=0;j<var->dimensionlinks.size();j++){
    if(var->dimensionlinks[j]->name.equals("time"))return j;
  }
  //Third, try to find it based on indexof method.
  for(size_t j=0;j<var->dimensionlinks.size();j++){
    if(var->dimensionlinks[j]->name.indexOf("time")>=0)return j;
  }
  return -1;
}
CDF::Variable *CDataReader::getTimeVariable( CDFObject *cdfObject, CDF::Variable * var){
  int timeDimIndex = getTimeDimIndex(cdfObject,var);
  if(timeDimIndex == -1) return NULL;
  return cdfObject->getVariableNE(var->dimensionlinks[timeDimIndex]->name.c_str());
}

CDF::Variable *CDataReader::getTimeVariable(){
  if(dataSource->isConfigured==false){
    CDBError("dataSource is not configured");
    return NULL;
  }
  CDFObject *cdfObject=dataSource->dataObject[0]->cdfObject;
  CDF::Variable * cdfVariable=dataSource->dataObject[0]->cdfVariable;
  if(cdfObject==NULL||cdfVariable==NULL){
    CDBError("CNetCDFReader is not opened");
    return NULL;
  }
  return getTimeVariable(cdfObject,cdfVariable);
}


int CDataReader::getTimeString(char * pszTime){
  //TODO We assume that the first configured DIM is always time. This might be not the case!
  pszTime[0]='\0';
  if(dataSource->isConfigured==false){
    CDBError("dataSource is not configured");
    return 1;
  }
  if(dataSource->cfgLayer->Dimension.size()==0){
    pszTime[0]='\0';
    return 0;
  }
  CDF::Variable *time = getTimeVariable();
  if(time==NULL){return 1;}
  CDF::Attribute *timeUnits = time->getAttributeNE("units");
  if(timeUnits ==NULL){return 1;}
  time->readData(CDF_DOUBLE);
  if(dataSource->dNetCDFNumDims>2){
    size_t currentTimeIndex=dataSource->getDimensionIndex(time->name.c_str());
    if(currentTimeIndex>=0&&currentTimeIndex<time->getSize()){
      CADAGUC_time *ADTime = NULL;
      try{
        ADTime = new CADAGUC_time((char*)timeUnits->data);
        stADAGUC_time timest;
        int status = ADTime->OffsetToAdaguc(timest,((double*)time->data)[currentTimeIndex]);
        if(status == 0){
          char ISOTime[MAX_STR_LEN+1];
          ADTime->PrintISOTime(ISOTime,MAX_STR_LEN,timest);
          ISOTime[19]='Z';ISOTime[20]='\0';
          snprintf(pszTime,MAX_STR_LEN,"%s",ISOTime);
        }
      }catch(int e){
        CDBError("Unable to initialize CADAGUC_time");
        delete ADTime;
        return 1;
      }
      delete ADTime;
    }
  }else{
    snprintf(pszTime,MAX_STR_LEN,"(time missing: dims = %d)",dataSource->dNetCDFNumDims);
  }
  return 0;
}

int CDataReader::justLoadAFileHeader(CDataSource *dataSource){
  if(dataSource==NULL){CDBError("datasource == NULL");return 1;}
  if(dataSource->cfgLayer==NULL){CDBError("datasource->cfgLayer == NULL");return 1;}
  if(dataSource->dataObject.size()==0){CDBError("dataSource->dataObject.size()==0");return 1;}
  if(dataSource->dataObject[0]->cdfVariable!=NULL){CDBDebug("already loaded: dataSource->dataObject[0]->cdfVariable!=NULL");return 0;}
  //if(dataSource->dataObject[0]->cdfObject!=NULL){CDBError("datasource->dataObject[0]->cdfObject != NULL");return 1;}

  CDirReader dirReader;
  if(CDBFileScanner::searchFileNames(&dirReader,dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),NULL)!=0){CDBError("Could not find any filename");return 1; }
  if(dirReader.fileList.size()==0){CDBError("dirReader.fileList.size()==0");return 1; }
  //Open a file
  try{
    /*CT::string cachefileName;
    if(getCacheFileName(dataSource,&cachefileName)!=0){
      CDBDebug("cachefileName: %s",cachefileName.c_str());
      dirReader.fileList[0]->fullName.copy(cachefileName.c_str());
    }*/
    
    CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,dirReader.fileList[0]->fullName.c_str());
    if(cdfObject == NULL)throw(__LINE__);

    
       CDBDebug("Opening %s",dirReader.fileList[0]->fullName.c_str());
    int status = cdfObject->open(dirReader.fileList[0]->fullName.c_str());
    
 
    if(status!=0){
      CDBError("Unable to open file '%s'",dirReader.fileList[0]->fullName.c_str());
      throw(__LINE__);
    }
    
    
   dataSource->attachCDFObject(cdfObject);
   
  }catch(int linenr){
    CDBError("Returning from line %d");
    dataSource->detachCDFObject();
    return 1;        
  }
  
  return 0;
}


int CDataReader::autoConfigureDimensions(CDataSource *dataSource,bool tryToFindInterval){
  

  // Auto configure dimensions, in case they are not configured by the user.
  // Dimension configuration is added to the internal XML configuration structure.
  if(dataSource->cfgLayer->Dimension.size()>0){
#ifdef CDATAREADER_DEBUG        
        CDBDebug("dataSource->cfgLayer->Dimension.size()>0: return 0;");
#endif     
    return 0;
  }
  if(dataSource==NULL){CDBDebug("datasource == NULL");return 1;}
  if(dataSource->cfgLayer==NULL){CDBDebug("datasource->cfgLayer == NULL");return 1;}
  if(dataSource->cfgLayer->DataBaseTable.size()==0){CDBDebug("dataSource->cfgLayer->DataBaseTable.size()==0");return 1;}
  
#ifdef CDATAREADER_DEBUG
 CDBDebug("autoConfigureDimensions %s",dataSource->getLayerName());
#endif  
  //Try to load the auto dimension information from the database
  CT::string query;
  CT::string tableName = "autoconfigure_dimensions";
  CT::string layerTableId = dataSource->cfgLayer->DataBaseTable[0]->value.c_str();
  layerTableId.concat("_");layerTableId.concat(dataSource->getLayerName());
  query.print("SELECT * FROM %s where layerid=E'%s'",tableName.c_str(),layerTableId.c_str());
  CPGSQLDB db;

  if(db.connect(dataSource->cfg->DataBase[0]->attr.parameters.c_str())!=0){CDBError("Error Could not connect to the database");return 1; }
  CDB::Store *store = db.queryToStore(query.c_str());
  db.close(); 
  if(store!=NULL){
    
    try{
      
      for(size_t j=0;j<store->size();j++){
        CServerConfig::XMLE_Dimension *xmleDim=new CServerConfig::XMLE_Dimension();
        xmleDim->value.copy(store->getRecord(j)->get("ogcname")->c_str());
        xmleDim->attr.name.copy(store->getRecord(j)->get("ncname")->c_str());
        xmleDim->attr.units.copy(store->getRecord(j)->get("units")->c_str());
#ifdef CDATAREADER_DEBUG        
        CDBDebug("Retrieved auto dim %s-%s from db for layer %s", xmleDim->value.c_str(),xmleDim->attr.name.c_str(),layerTableId.c_str());
#endif        
        dataSource->cfgLayer->Dimension.push_back(xmleDim);
      }
      delete store;
#ifdef CDATAREADER_DEBUG        
        CDBDebug("Found auto dims.");
#endif        
      //return 0;
    }catch(int e){
      delete store;
      CDBError("DB Exception: %s\n",db.getErrorMessage(e));
    }
  }
#ifdef CDATAREADER_DEBUG     
  CDBDebug("autoConfigureDimensions information not in DB");
#endif  
  //Information is not available in the database. We need to load it from a file.
  int status=justLoadAFileHeader(dataSource);
  if(status!=0){
    CDBError("justLoadAFileHeader failed");
    return 1;
  }
  
  try{
    if(dataSource->dataObject.size()==0){CDBDebug("dataSource->dataObject.size()==0");throw(__LINE__);}
    if(dataSource->dataObject[0]->cdfVariable==NULL){CDBDebug("dataSource->dataObject[0]->cdfVariable==NULL");throw(__LINE__);}
    //CDBDebug("OK %d",dataSource->cfgLayer->Dimension.size());
    if(dataSource->cfgLayer->Dimension.size()==0){
    
      
      //No dimensions are configured by the user, try to configure them automatically here.
      if(dataSource->dataObject[0]->cdfVariable->dimensionlinks.size()>=2){
        
        try{
          //Create the database table
          CT::string tableColumns("layerid varchar (255), ncname varchar (255), ogcname varchar (255), units varchar (255)");
          int status;
          CPGSQLDB DB;
          status = DB.connect(dataSource->cfg->DataBase[0]->attr.parameters.c_str());if(status!=0){CDBError("Error Could not connect to the database");throw(__LINE__);}
          status = DB.checkTable(tableName.c_str(),tableColumns.c_str());
          if(status == 1){CDBError("\nFAIL: Table %s could not be created: %s",tableName.c_str(),tableColumns.c_str()); DB.close();throw(__LINE__);  }

          CDF::Variable *variable=dataSource->dataObject[0]->cdfVariable;
          CDBDebug("OK %d",variable->dimensionlinks.size()-2);
          
          // When there are no extra dims besides x and y: make a table anyway so autoconfigure_dimensions is not run 
          // Each time to find the non existing dims.
          if(variable->dimensionlinks.size()==2){
            CDBDebug("Creating an empty table, because variable %s has only x and y dims",variable->name.c_str());
            return 0;
          }
          
          for(size_t d=0;d<variable->dimensionlinks.size()-2;d++){
            CDF::Dimension *dim=variable->dimensionlinks[d];
            if(dim!=NULL){
              CDF::Variable *dimVar=dataSource->dataObject[0]->cdfObject->getVariable(dim->name.c_str());
              
              CT::string units="";
              CT::string standard_name=dim->name.c_str();  
              CT::string OGCDimName;
              try{dimVar->getAttribute("units")->getDataAsString(&units);}catch(int e){}
              //try{dimVar->getAttribute("standard_name")->getDataAsString(&standard_name);}catch(int e){}
              OGCDimName.copy(&standard_name);
              CDBDebug("Datasource %s: Dim %s; units %s; standard_name %s",dataSource->layerName.c_str(),dim->name.c_str(),units.c_str(),standard_name.c_str());
              CServerConfig::XMLE_Dimension *xmleDim=new CServerConfig::XMLE_Dimension();
              dataSource->cfgLayer->Dimension.push_back(xmleDim);
              xmleDim->value.copy(OGCDimName.c_str());
              xmleDim->attr.name.copy(standard_name.c_str());
              xmleDim->attr.units.copy(units.c_str());
              
              
              //Store the data in the db for quick access.
              query.print("INSERT INTO %s values (E'%s',E'%s',E'%s',E'%s')",tableName.c_str(),layerTableId.c_str(),standard_name.c_str(),OGCDimName.c_str(),units.c_str());
              status = DB.query(query.c_str()); if(status!=0){CDBError("Unable to insert records: \"%s\"",query.c_str());DB.close();throw(__LINE__); }
                      
            }else{
              CDBDebug("variable->dimensionlinks[d]");
            }
          }
        }catch(int e){
          CT::string errorMessage;CDF::getErrorMessage(&errorMessage,e); CDBDebug("Unable to configure dims automatically: %s (%d)",errorMessage.c_str(),e);    
          throw(e);
        }
      }
      
     
    }
  }catch(int linenr){
    CDBDebug("Exp at line %d",linenr);

    return 1;
  }

  
  return 0;
}


int CDataReader::autoConfigureStyles(CDataSource *dataSource){
#ifdef CDATAREADER_DEBUG     
  CDBDebug("autoConfigureStyles");
#endif  
  if(dataSource==NULL){CDBDebug("datasource == NULL");return 1;}
  if(dataSource->cfgLayer==NULL){CDBDebug("datasource->cfgLayer == NULL");return 1;}
  if(dataSource->cfgLayer->Styles.size()!=0){return 0;};//Configured by user, auto styles is not required

  if(dataSource->cfgLayer->Legend.size()!=0){return 0;};//Configured by user, auto styles is not required

  if(dataSource->cfgLayer->DataBaseTable.size()==0){CDBDebug("dataSource->cfgLayer->DataBaseTable.size()==0");return 1;}
  
  // Try to find a style corresponding the the standard_name or long_name attribute of the file.
  CServerConfig::XMLE_Styles *xmleStyle=new CServerConfig::XMLE_Styles();
  dataSource->cfgLayer->Styles.push_back(xmleStyle);
  CT::string tableName = "autoconfigure_styles";
  CT::string layerTableId = dataSource->cfgLayer->DataBaseTable[0]->value.c_str();
  CT::string query;
  query.print("SELECT * FROM %s where layerid=E'%s'",tableName.c_str(),layerTableId.c_str());
  CPGSQLDB db;
  if(db.connect(dataSource->cfg->DataBase[0]->attr.parameters.c_str())!=0){CDBError("Error Could not connect to the database");return 1; }
  CDB::Store *store = db.queryToStore(query.c_str());
  db.close(); 
  if(store!=NULL){
    if(store->size()!=0){
      try{
        xmleStyle->value.copy(store->getRecord(0)->get("styles")->c_str());
        delete store;store=NULL;
      }catch(int e){
        delete store;
        CDBError("autoConfigureStyles: DB Exception: %s for query %s",db.getErrorMessage(e),query.c_str());
        return 1;
      }
#ifdef CDATAREADER_DEBUG           
      CDBDebug("Retrieved auto styles \"%s\" from db",xmleStyle->value.c_str());
#endif     
      //OK!
      return 0;
    }
    delete store;
  }
    //Finished!
    
    // Auto style is not available in the database, so look it up in the file.
    CT::string searchName=dataSource->dataObject[0]->variableName.c_str();

    try{
      //If the file header is not yet loaded, load it.
      if(dataSource->dataObject[0]->cdfVariable==NULL){
        int status=CDataReader::justLoadAFileHeader(dataSource);
        if(status!=0){CDBError("unable to load datasource headers");return 1;}
      }
      dataSource->dataObject[0]->cdfVariable->getAttribute("long_name")->getDataAsString(&searchName);
      dataSource->dataObject[0]->cdfVariable->getAttribute("standard_name")->getDataAsString(&searchName);        
    }catch(int e){}
  /*  if(cdfObject!=NULL){
      //TODO CHECK
      //Decouple references
      dataSource->dataObject[0]->cdfObject=NULL;
      dataSource->dataObject[0]->cdfVariable=NULL;
      for(size_t varNr=0;varNr<dataSource->dataObject.size();varNr++){dataSource->dataObject[varNr]->cdfVariable = NULL;}
    }
    delete cdfObject;*/
    CDBDebug("Retrieving auto styles by using fileinfo \"%s\"",searchName.c_str());
    // We now have the keyword searchname, with this keyword we are going to lookup all StandardName's in the server configured Styles
    CT::string styles="auto";
    for(size_t j=0;j<dataSource->cfg->Style.size();j++){
      const char *styleName=dataSource->cfg->Style[j]->attr.name.c_str();
      CDBDebug("Searching Style \"%s\"",styleName);
      if(styleName!=NULL){
        for(size_t i=0;i<dataSource->cfg->Style[j]->StandardNames.size();i++){
          const char *cfgStandardNames=dataSource->cfg->Style[j]->StandardNames[i]->value.c_str();
          CDBDebug("Searching StandardNames \"%s\"",cfgStandardNames);
          if(cfgStandardNames!=NULL){
            CT::string t=cfgStandardNames;
            CT::stringlist *standardNameList=t.splitN(",");
            for(size_t n=0;n<(*standardNameList).size();n++){
              if(searchName.indexOf((*standardNameList)[n]->c_str())!=-1){
                CDBDebug("*** Match: \"%s\" ~~ \"%s\"",searchName.c_str(),(*standardNameList)[n]->c_str());
                if(styles.length()!=0)styles.concat(",");
                styles.concat(dataSource->cfg->Style[j]->attr.name.c_str());
              }
            }
            delete standardNameList;
          }
        }
      }      
    }
    CDBDebug("Found styles \"%s\"",styles.c_str());
    try{
      CT::string tableColumns("layerid varchar (255), styles varchar (255)");
      int status;
      CPGSQLDB DB;
      status = DB.connect(dataSource->cfg->DataBase[0]->attr.parameters.c_str());if(status!=0){CDBError("Error Could not connect to the database");throw(__LINE__);}
      status = DB.checkTable(tableName.c_str(),tableColumns.c_str());
      if(status == 1){CDBError("\nFAIL: Table %s could not be created: %s",tableName.c_str(),tableColumns.c_str()); DB.close();throw(__LINE__);  }
      query.print("INSERT INTO %s values (E'%s',E'%s')",tableName.c_str(),layerTableId.c_str(),styles.c_str());
      status = DB.query(query.c_str()); if(status!=0){CDBError("Unable to insert records: \"%s\"",query.c_str());DB.close();throw(__LINE__); }
    }catch(int linenr){
      CDBError("Exception at line %d",linenr);
      return 1;
    }
    xmleStyle->value.copy(styles.c_str());
  
  return 0;
}
