#include "CDataReader.h"
#include <sys/stat.h>
const char *CDataReader::className="CDataReader";

void printStatus(const char *status,const char *a,...){
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
}

CDFReader *CDataReader::getCDFReader(CDataSource *sourceImage){
   //Do we have a datareader defined in the configuration file?
  if(cdfReader !=NULL){delete cdfReader;cdfReader = NULL;}
  if(sourceImage->cfgLayer->DataReader.size()>0){
    if(sourceImage->cfgLayer->DataReader[0]->value.equals("HDF5")){
      cdfReader = new CDFHDF5Reader(cdfObject);
      CDFHDF5Reader * hdf5Reader = (CDFHDF5Reader*)cdfReader;
      hdf5Reader->enableKNMIHDF5toCFConversion();
    }
  }
  //Defaults to the netcdf reader
  if(cdfReader==NULL)cdfReader = new CDFNetCDFReader(cdfObject);
  return cdfReader;
}



int CDataReader::open(CDataSource *_sourceImage, int mode,const char *cacheLocation){
  
  //Some checks
  if(cdfObject!=NULL){CDBError("CDataReader already in use");return 1;}
  if(_sourceImage==NULL){CDBError("Invalid sourceImage");return 1;}
  if(_sourceImage->getFileName()==NULL){CDBError("Invalid NetCDF filename");return 1;}
  sourceImage=_sourceImage;
  FileName.copy(sourceImage->getFileName());
  //sourceImage->stretchMinMax=true;
  //CDBDebug("Reading %s",sourceImage->getFileName());
  //Create CDF object
  cdfObject=new CDFObject();
  //Get a reader:
  cdfReader=getCDFReader(sourceImage);
  if(cdfReader==NULL){
    CDBError("Unable to get a reader for source %s",sourceImage->cfgLayer->Name[0]->value.c_str());
    return 1;
  }
  
  bool enableDataCache=false;
  if(sourceImage->cfgLayer->Cache.size()>0){
    if(sourceImage->cfgLayer->Cache[0]->attr.enabled.equals("true")){
      enableDataCache=true;
    }
  }
  
  if(cacheLocation==NULL)enableDataCache=false;else if(strlen(cacheLocation)==0)enableDataCache=false;
  bool cacheAvailable=false;
  bool workingOnCache=false;
  //bool saveMetadataFile = false;
  bool saveFieldFile = false;
  CT::string uniqueIDFor2DField;
  CT::string uniqueIDFor2DFieldTmp;
  //CT::string uniqueIDForMetadata;
  
  if(enableDataCache==true){
    int intStat;
    struct stat stFileInfo;
    
    int timeStep = sourceImage->getCurrentTimeStep();
    //uniqueIDFor2DField.copy("file_");
    uniqueIDFor2DField.copy(cacheLocation);
    uniqueIDFor2DField.concat("/");
    intStat = stat(uniqueIDFor2DField.c_str(),&stFileInfo);
    if(intStat != 0){
      CDBDebug("making dir %s",uniqueIDFor2DField.c_str());
      mode_t permissions = S_IRWXU|S_IRWXG|S_IRWXO;
      mkdir (uniqueIDFor2DField.c_str(),permissions);
    }
    CT::string validFileName(sourceImage->getFileName());
    //int b=validFileName.lastIndexOf("/");
    //if(b>0)validFileName.substring(b,validFileName.length());
    validFileName.replace(":","");
    validFileName.replace("/","");
    uniqueIDFor2DField.concat(&validFileName);
    uniqueIDFor2DField.concat("cache");
    //uniqueIDFor2DField.copy(&uniqueIDForMetadata);
    
    
    
    intStat = stat(uniqueIDFor2DField.c_str(),&stFileInfo);
    if(intStat != 0){
      CDBDebug("making dir %s",uniqueIDFor2DField.c_str());
      mode_t permissions = S_IRWXU|S_IRWXG|S_IRWXO;
      mkdir (uniqueIDFor2DField.c_str(),permissions);
    }
    uniqueIDFor2DField.concat("/");
    
    uniqueIDFor2DField.concat(sourceImage->dataObject[0]->variableName.c_str());
    uniqueIDFor2DField.concat("_");
    
    
    if(sourceImage->timeSteps[timeStep]->dims.dimensions.size()>0){
      for(size_t j=0;j<sourceImage->timeSteps[timeStep]->dims.dimensions.size();j++){
        uniqueIDFor2DField.printconcat("%s=%d", 
                                       sourceImage->timeSteps[timeStep]->dims.dimensions[0]->name.c_str(),
                                       sourceImage->timeSteps[timeStep]->dims.dimensions[0]->index);
      }
    }
    uniqueIDFor2DFieldTmp.copy(&uniqueIDFor2DField);
    uniqueIDFor2DFieldTmp.concat("_tmp");
    //uniqueIDForMetadata.concat(".ncmeta");
    //uniqueIDFor2DField.concat(".ncfield");
    
//    CDBDebug("uniqueIDFor2DField = %s",uniqueIDFor2DField.c_str());
    
    
    //intStat = stat(uniqueIDForMetadata.c_str(),&stFileInfo);if(intStat != 0)saveMetadataFile = true;
    intStat = stat(uniqueIDFor2DField.c_str(),&stFileInfo);if(intStat != 0)saveFieldFile = true;else {
      //CDBDebug("Cache file available");
      cacheAvailable=true;
    }
    
    intStat = stat(uniqueIDFor2DFieldTmp.c_str(),&stFileInfo);
    if(intStat == 0){workingOnCache=true;}
    
    //intStat = stat(uniqueIDFor2DField.c_str(),&stFileInfo);
    //if(intStat == 0){workingOnCache=true;}
    
    if(workingOnCache==true){
      CDBDebug("*** Another process is working on the cache file");
      saveFieldFile=false;
      cacheAvailable=false;
    }
    
    
  }
  
//  CDBDebug("uniqueIDFor2DField = %s",uniqueIDFor2DField.c_str());
#ifdef MEASURETIME
  StopWatch_Stop("opening file");
#endif

  if(cacheAvailable==true){
    if(cdfReader!=NULL)delete cdfReader;
    
    cdfReader = new CDFNetCDFReader(cdfObject);
  
    //CDBDebug("Reading from Cache file");
    status = cdfReader->open(uniqueIDFor2DField.c_str());
    int timeStep = sourceImage->getCurrentTimeStep();
    for(size_t j=0;j<sourceImage->timeSteps[timeStep]->dims.dimensions.size();j++){
      sourceImage->timeSteps[timeStep]->dims.dimensions[0]->index=0;
    }
  }else{
    status = cdfReader->open(FileName.c_str());
  }
  
#ifdef MEASURETIME
  StopWatch_Stop("file opened");
#endif

  if(status != 0){CDBError("Unable to read file %s",FileName.c_str());return 1;}
  CDF::Variable *var[sourceImage->dataObject.size()+1];
  if(sourceImage->dataObject.size()<=0){
    CDBError("No variables found for datasource %s",sourceImage->cfgLayer->Name[0]->value.c_str());
    return 1;
  }
  for(size_t varNr=0;varNr<sourceImage->dataObject.size();varNr++){
    var[varNr] = cdfObject->getVariable(sourceImage->dataObject[varNr]->variableName.c_str());
    if(var[varNr]==NULL){
      CDBError("Variable %s does not exist",sourceImage->dataObject[varNr]->variableName.c_str());
      return 1;
      /*CT::string dumpString;
      CDF::dump(cdfObject,&dumpString);
      CDBError("dump:%s\n",dumpString.c_str());
      return 1;*/
    }
    //CDBDebug("Getting info for variable %s",sourceImage->dataObject[varNr]->variableName.c_str());
    
  }
  
  // Retrieve Width and Height
  sourceImage->dNetCDFNumDims = var[0]->dimensionlinks.size();
//  CDBDebug("GRead X and Y dimension data %d",sourceImage->dNetCDFNumDims );
  CDF::Dimension *dimX=var[0]->dimensionlinks[sourceImage->dNetCDFNumDims-1];
  CDF::Dimension *dimY=var[0]->dimensionlinks[sourceImage->dNetCDFNumDims-2];
  if(dimX==NULL||dimY==NULL){CDBError("X and or Y dims not found...");return 1;}
  
  sourceImage->dWidth=dimX->length;
  sourceImage->dHeight=dimY->length;
  size_t start[sourceImage->dNetCDFNumDims+1];
  size_t count[sourceImage->dNetCDFNumDims+1];
  ptrdiff_t stride[sourceImage->dNetCDFNumDims+1];
  for(int j=0;j<sourceImage->dNetCDFNumDims;j++){start[j]=0; count[j]=1;stride[j]=1;}
  count[sourceImage->dNetCDFNumDims-1]=sourceImage->dWidth;
  count[sourceImage->dNetCDFNumDims-2]=sourceImage->dHeight;
  if(sourceImage->dNetCDFNumDims>2){
    for(int j=0;j<sourceImage->dNetCDFNumDims-2;j++){
      start[j]=sourceImage->getDimensionIndex(var[0]->dimensionlinks[j]->name.c_str());//dOGCDimValues[0];// time dim
      //CDBDebug("%s==%d",var[0]->dimensionlinks[j]->name.c_str(),start[j]);
    }
  }
  //Read X and Y dimension data
  
  CDF::Variable *varX=cdfObject->getVariable(dimX->name.c_str());
  CDF::Variable *varY=cdfObject->getVariable(dimY->name.c_str());
  if(varX==NULL||varY==NULL){CDBError("X and or Y vars not found...");return 1;}
  status = cdfReader->readVariableData(varX,CDF_DOUBLE);if(status!=0){
    CDBError("Unable to read x dimension");
    return 1;
  }
  status = cdfReader->readVariableData(varY,CDF_DOUBLE);if(status!=0){
    CDBError("Unable to read y dimension");
    return 1;
  }
  
// Calculate cellsize
  double *dfdim_X=(double*)varX->data;
  double *dfdim_Y=(double*)varY->data;
  sourceImage->dfCellSizeX=(dfdim_X[sourceImage->dWidth-1]-dfdim_X[0])/double(sourceImage->dWidth-1);
  sourceImage->dfCellSizeY=(dfdim_Y[sourceImage->dHeight-1]-dfdim_Y[0])/double(sourceImage->dHeight-1);
  // Calculate BBOX
  sourceImage->dfBBOX[0]=dfdim_X[0]-sourceImage->dfCellSizeX/2.0f;
  sourceImage->dfBBOX[1]=dfdim_Y[sourceImage->dHeight-1]+sourceImage->dfCellSizeY/2.0f;
  sourceImage->dfBBOX[2]=dfdim_X[sourceImage->dWidth-1]+sourceImage->dfCellSizeX/2.0f;
  sourceImage->dfBBOX[3]=dfdim_Y[0]-sourceImage->dfCellSizeY/2.0f;;
  //CDBDebug("BBOX:%f %f %f %f",dfdim_X[0],sourceImage->dfBBOX[1],sourceImage->dfBBOX[2],sourceImage->dfBBOX[3]);
#ifdef MEASURETIME
  StopWatch_Stop("dimensions read");
#endif

  // Retrieve CRS
  
  //Check if projection is overidden in the config file
  if(sourceImage->cfgLayer->Projection.size()==1){
    //Read projection information from configuration
    if(sourceImage->cfgLayer->Projection[0]->attr.id.c_str()!=NULL)
      sourceImage->nativeEPSG.copy(sourceImage->cfgLayer->Projection[0]->attr.id.c_str());
    else sourceImage->nativeEPSG.copy("EPSG:4326");
    //Read proj4 string
    if(sourceImage->cfgLayer->Projection[0]->attr.proj4.c_str()!=NULL)
      sourceImage->nativeProj4.copy(sourceImage->cfgLayer->Projection[0]->attr.proj4.c_str());
    else sourceImage->nativeProj4.copy("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
  }else{
    // If undefined, set standard lat lon projection
    sourceImage->nativeProj4.copy("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
    sourceImage->nativeEPSG.copy("EPSG:4326");
    //Read projection attributes from the file
    CDF::Attribute *projvarnameAttr = var[0]->getAttribute("grid_mapping");
    if(projvarnameAttr!=NULL){
      CDF::Variable * projVar = cdfObject->getVariable((char*)projvarnameAttr->data);
      if(projVar==NULL){CDBWarning("projection variable '%s' not found",(char*)projvarnameAttr->data);}
      else {
        //Get proj4_params
        CDF::Attribute *proj4Attr = projVar->getAttribute("proj4_params");
        if(proj4Attr!=NULL)sourceImage->nativeProj4.copy((char*)proj4Attr->data);
        //else {CDBWarning("proj4_params not found in variable %s",(char*)projvarnameAttr->data);}
        //Get EPSG_code
        CDF::Attribute *epsgAttr = projVar->getAttribute("EPSG_code");
        if(epsgAttr!=NULL)sourceImage->nativeEPSG.copy((char*)epsgAttr->data);
        //else {CDBWarning("EPSG_code not found in variable %s",(char*)projvarnameAttr->data);}
      }
    }
  }
  //CDBDebug("proj4=%s",sourceImage->nativeProj4.c_str());
  //Determine sourceImage->dataObject[0]->dataType of the variable we are going to read
  for(size_t varNr=0;varNr<sourceImage->dataObject.size();varNr++){
    sourceImage->dataObject[varNr]->dataType=CDF_NONE;
    if(var[varNr]->type==CDF_CHAR||var[varNr]->type==CDF_BYTE)sourceImage->dataObject[varNr]->dataType=CDF_CHAR;
    if(var[varNr]->type==CDF_UBYTE)sourceImage->dataObject[varNr]->dataType=CDF_UBYTE;
    
    if(var[varNr]->type==CDF_SHORT||var[varNr]->type==CDF_USHORT)sourceImage->dataObject[varNr]->dataType=CDF_SHORT;
    if(var[varNr]->type==CDF_INT||var[varNr]->type==CDF_UINT)sourceImage->dataObject[varNr]->dataType=CDF_INT;
    if(var[varNr]->type==CDF_FLOAT)sourceImage->dataObject[varNr]->dataType=CDF_FLOAT;
    if(var[varNr]->type==CDF_DOUBLE)sourceImage->dataObject[varNr]->dataType=CDF_DOUBLE;
    if(sourceImage->dataObject[varNr]->dataType==CDF_NONE){
      CDBError("Invalid sourceImage->dataObject[varNr]->dataType");
      return 1;
    }
    //Get Unit
    CDF::Attribute *varUnits=var[varNr]->getAttribute("units");
    if(varUnits!=NULL){
      sourceImage->dataObject[varNr]->units.copy((char*)varUnits->data);
    }else sourceImage->dataObject[varNr]->units.copy("");
  
  //  bool scaleOffsetIsApplied;
   // double dfscale_factor;
    //double dfadd_offset;
    // Check for packed data / scaleOffsetIsApplied
    CDF::Attribute *scale_factor = var[varNr]->getAttribute("scale_factor");
    
    if(scale_factor!=NULL){
      sourceImage->dataObject[varNr]->scaleOffsetIsApplied=true;
        // Currently two unpacked data types are supported (CF-1.4): float and double
      if(scale_factor->type==CDF_FLOAT)sourceImage->dataObject[varNr]->dataType=CDF_FLOAT;
      if(scale_factor->type==CDF_DOUBLE)sourceImage->dataObject[varNr]->dataType=CDF_DOUBLE;
      //sourceImage->dataObject[varNr]->dataType=CDF_DOUBLE;
    
      char dataTypeName[256];
      CDF::getCDFDataTypeName(dataTypeName,255, sourceImage->dataObject[varNr]->dataType);
      //CDBDebug("Dataset datatype for reading = %s sizeof(short)=%d",dataTypeName,sizeof(short));
    
        //Internally we use always double for scale and offset parameters:
      scale_factor->getData(&sourceImage->dataObject[varNr]->dfscale_factor,1);
      CDF::Attribute *add_offset = var[varNr]->getAttribute("add_offset");
      if(add_offset!=NULL){
        add_offset->getData(&sourceImage->dataObject[varNr]->dfadd_offset,1);
      }else sourceImage->dataObject[varNr]->dfadd_offset=0;
    }else sourceImage->dataObject[varNr]->scaleOffsetIsApplied=false;
  }

  if(mode==CNETCDFREADER_MODE_GET_METADATA){
    sourceImage->nrMetadataItems=0;
    char szTemp[MAX_STR_LEN+1];
    // Find out how many attributes we have
    sourceImage->nrMetadataItems=cdfObject->attributes.size();
    //CDBDebug("NC_GLOBAL: %d",sourceImage->nrMetadataItems);
    for(size_t i=0;i<cdfObject->variables.size();i++){
      sourceImage->nrMetadataItems+=cdfObject->variables[i]->attributes.size();
      //CDBDebug("%s: %d",cdfObject->variables[i]->name.c_str(),cdfObject->variables[i]->attributes.size());
    }
    //Allocate data for the attributes
    if(sourceImage->metaData != NULL)delete[] sourceImage->metaData;
    sourceImage->metaData = new CT::string[sourceImage->nrMetadataItems+1];
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
        //CDBDebug("variableName %d==%d %s",nrAttributes,sourceImage->nrMetadataItems,szTemp);
        sourceImage->metaData[nrAttributes++].copy(szTemp);
      }
    }
    return 0;
  }


  if(mode==CNETCDFREADER_MODE_OPEN_ALL){
    // Read the variable
    //char szTemp[1024];
    //CDF::getCDFDataTypeName(szTemp,1023, sourceImage->dataObject[0]->dataType);
    //CDBDebug("Datatype: %s",szTemp);
#ifdef MEASURETIME
  StopWatch_Stop("start reading image data");
#endif
  //for(size_t varNr=0;varNr<sourceImage->dataObject.size();varNr++)
    
  double dfNoData = 0;
  bool hasNodataValue =false;;
  
  for(size_t varNr=0;varNr<sourceImage->dataObject.size();varNr++)
  {

    /*//Get Unit
    CDF::Attribute *varUnits=var[varNr]->getAttribute("units");
    if(varUnits!=NULL){
      sourceImage->dataObject[varNr]->units.copy((char*)varUnits->data);
    }else sourceImage->dataObject[varNr]->units.copy("");
    */
  
    CDF::Attribute *fillValue = var[varNr]->getAttribute("_FillValue");
    if(fillValue!=NULL){
      hasNodataValue=true;
      fillValue->getData(&dfNoData,1);
      
    }else hasNodataValue=false;
  //size_t varNr=0;
    CDBDebug("--- varNR [%d], name=\"%s\"",varNr,var[varNr]->name.c_str());
    for(size_t d=0;d<var[varNr]->dimensionlinks.size();d++){
      CDBDebug("%s  \tstart: %d\tcount %d",var[varNr]->dimensionlinks[d]->name.c_str(),start[d],count[d]);
    }
    
    cdfReader->readVariableData(var[varNr], sourceImage->dataObject[varNr]->dataType,start,count,stride);
//    CDBDebug("Reading variable %s",var[varNr]->name.c_str());
    if(sourceImage->dataObject[varNr]->scaleOffsetIsApplied == true ){
      double dfscale_factor = sourceImage->dataObject[varNr]->dfscale_factor;
      double dfadd_offset = sourceImage->dataObject[varNr]->dfadd_offset;
      //CDBDebug("dfNoData=%f",dfNoData*dfscale_factor);

      if(sourceImage->dataObject[varNr]->dataType==CDF_FLOAT){
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
      }
      if(sourceImage->dataObject[varNr]->dataType==CDF_DOUBLE){
        // packed data to be unpacked to FLOAT:
        double *_data=(double*)var[varNr]->data;
        for(size_t j=0;j<var[varNr]->getSize();j++){
          _data[j]*=dfscale_factor;
          _data[j]+=dfadd_offset;
        }
        //Convert the nodata type
        dfNoData*=(double)dfscale_factor;
        dfNoData+=(double)dfadd_offset;
      }
    }
    sourceImage->dataObject[varNr]->data=var[varNr]->data;
    sourceImage->dataObject[varNr]->dfNodataValue=dfNoData;
    //CDBDebug("%f %f",sourceImage->dataObject[varNr]->dfscale_factor,dfNoData);
    sourceImage->dataObject[varNr]->hasNodataValue=hasNodataValue;
  }
  
  if(sourceImage->stretchMinMax){
    if(sourceImage->statistics==NULL){
      sourceImage->statistics = new CDataSource::Statistics();
      sourceImage->statistics->calculate(sourceImage);
    }
    
    float min=(float)sourceImage->statistics->getMinimum();
    float max=(float)sourceImage->statistics->getMaximum();
    //Make sure that there is always a range in between the min and max.
    if(max==min)max=min+0.1;
    //Calculate legendOffset legendScale
    float ls=240/(max-min);
    float lo=-(min*ls);
    lo=int(lo-ls/2);
    //float minValue=min;
    //float maxValue=max;
    sourceImage->legendScale=ls;
    sourceImage->legendOffset=lo;
  }
#ifdef MEASURETIME
  StopWatch_Stop("all read");
#endif

    //CDBDebug("Read succesfully:\nsf=%f\naf=%f,nd=%f\n",dfscale_factor,dfadd_offset,sourceImage->dfNodataValue);
  //Check wether this local file does already exist...
    
/*    for(int j=0;j<sourceImage->dNetCDFNumDims;j++){
      CDBDebug("start %d count %d\n",start[j],count[j]);
  }*/

    if(enableDataCache==true&&saveFieldFile==true){
      //CDFObject newCDFObject
      
  //CDBDebug("Writing data cache file");
      /*if(saveMetadataFile){
        CDBDebug("Writing to %s",uniqueIDFor2DField.c_str());
        CDFNetCDFWriter netCDFWriter(cdfObject);
        netCDFWriter.disableVariableWrite();
        netCDFWriter.write(uniqueIDForMetadata.c_str());
    }*/
      int keepVariable[sourceImage->dataObject.size()];
      for(size_t v=0;v<cdfObject->variables.size();v++){
        keepVariable[v]=0;        
      }
      //if(saveFieldFile)
      {
        //size_t dataSize=sourceImage->dWidth*sourceImage->dHeight*CDF::getTypeSize(sourceImage->dataObject[0]->dataType);
        //CDBDebug("Writing to %s",uniqueIDFor2DField.c_str());
        
        //Make a list of required variables and dimensions...
        for(size_t v=0;v<sourceImage->dataObject.size();v++){
          for(size_t w=0;w<cdfObject->variables.size();w++){
            if(keepVariable[w]==0){
              if(cdfObject->variables[w]->isDimension==true||cdfObject->variables[w]->dimensionlinks.size()==0){
                keepVariable[w]=1;
              }
              if(keepVariable[w]==0){
                if(cdfObject->variables[w]->name.equals(&sourceImage->dataObject[v]->variableName)){
                  keepVariable[w]=1;
                }
              }
            }
          }
        }
        std::vector<CT::string *> deleteVarNames;
        for(size_t v=0;v<cdfObject->variables.size();v++){
          if(keepVariable[v]==0){
            deleteVarNames.push_back(new CT::string(&cdfObject->variables[v]->name));
          }
        }
        for(size_t i=0;i<deleteVarNames.size();i++){
          cdfObject->removeVariable(deleteVarNames[i]->c_str());
          delete deleteVarNames[i];
          deleteVarNames[i]=NULL;
        }
        //cdfObject->removeVariable("image1.image_data");
        //for(size_t v=0;v<cdfObject->variables.size();v++){
          //CDBDebug("%d%s",cdfObject->variables[v]->isDimension,cdfObject->variables[v]->name.c_str());
          
          //if(cdfObject->variables[v]->name.equals("QC")){
            //CDF::allocateData(CDF_FLOAT,&cdfObject->variables[v]->data,1036800);
          //}
        //}
        //Adjust dimensions for the single object:
        
        varX->type=CDF_DOUBLE;
        varY->type=CDF_DOUBLE;
        int timeStep = sourceImage->getCurrentTimeStep();

        /*for(size_t j=0;j<cdfObject->variables.size();j++){
          cdfObject->variables[j]->type=CDF_DOUBLE;
      }*/
          
        for(size_t j=0;j<sourceImage->timeSteps[timeStep]->dims.dimensions.size();j++){
          CDF::Dimension *dim = cdfObject->getDimension(sourceImage->timeSteps[timeStep]->dims.dimensions[0]->name.c_str());
          CDF::Variable *var = cdfObject->getVariable(sourceImage->timeSteps[timeStep]->dims.dimensions[0]->name.c_str());
          
            cdfReader->readVariableData(var,CDF_DOUBLE);
            
            double dimValue[var->getSize()];
            //sourceImage->timeSteps[0]->dims.dimensions[0]->index*CDF::getTypeSize(var->type)
            CDF::dataCopier.copy(dimValue,var->data,var->type,var->getSize());
            size_t index=sourceImage->timeSteps[timeStep]->dims.dimensions[0]->index;
            //CDBDebug("dimvalue %s at index %d = %f",var->name.c_str(),index,dimValue[index]);
            
            var->setSize(1);
            dim->length=1;
            ((double*)var->data)[0]=dimValue[index];
            //sourceImage->timeSteps[0]->dims.dimensions[0]->index
            //var->setData(var->type,const void *dataToSet,1){
             //sourceImage->timeSteps[0]->dims.dimensions[0]->index);
          }
         // CT::string dumpString;
          //CDF::dump(cdfObject,&dumpString);
          //printf("\nSTART\n%s\nEND\n",dumpString.c_str());
        
        CDFNetCDFWriter netCDFWriter(cdfObject);
        netCDFWriter.disableReadData();
        
        //netCDFWriter.disableVariableWrite();
        //uniqueIDFor2DField.concat("nep");
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
          CDBDebug("*** [1/4] Claiming cache file ");
          const char buffer[] = { "ab" };
          pFile = fopen ( uniqueIDFor2DFieldTmp.c_str() , "wb" );
          size_t bytesWritten = fwrite (buffer , sizeof(char),2 , pFile );
          fflush (pFile);   
          fclose (pFile);
          if(bytesWritten!=2){
            CDBDebug("*** Failed to Claim the cache  file %d",bytesWritten);
            workingOnCache=true;
          }else {
            CDBDebug("*** [2/4] Succesfully claimed the cache file");
          }
        }
    
        if(workingOnCache==true){
          CDBDebug("*** Another process is working on the cache file");
          saveFieldFile=false;
          cacheAvailable=false;
        }else{
          
          CDBDebug("*** [3/4] Writing cache file");
          
          
         
        
          netCDFWriter.write(uniqueIDFor2DFieldTmp.c_str());
        
          //Move the temp file to the final name
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

  if(cdfReader!=NULL){cdfReader->close();delete cdfReader;cdfReader=NULL;}
  if(cdfObject!=NULL){delete cdfObject;cdfObject=NULL;}
  return 0;
}
int CDataReader::getTimeUnit(char * pszTime){
  if(cdfObject==NULL){
    CDBError("CNetCDFReader is not opened");
    return 1;
  }
  pszTime[0]='\0';
  CDF::Variable *time = cdfObject->getVariable(sourceImage->cfgLayer->Dimension[0]->attr.name.c_str());
  if(time==NULL){return 1;}
  CDF::Attribute *timeUnits = time->getAttribute("units");
  if(timeUnits ==NULL){return 1;}
  snprintf(pszTime,MAX_STR_LEN,"%s",(char*)timeUnits->data);
  return 0;
}

int CDataReader::getTimeString(char * pszTime){
  if(cdfObject==NULL){
    CDBError("CNetCDFReader is not opened");
    return 1;
  }
  pszTime[0]='\0';
  CDF::Variable *time = cdfObject->getVariable(sourceImage->cfgLayer->Dimension[0]->attr.name.c_str());
  if(time==NULL){return 1;}
  CDF::Attribute *timeUnits = time->getAttribute("units");
  if(timeUnits ==NULL){return 1;}
  cdfReader->readVariableData(time,CDF_DOUBLE);
  if(sourceImage->dNetCDFNumDims>2){
    size_t index=sourceImage->getDimensionIndex("time");
    if(index>=0&&index<time->getSize()){
      CADAGUC_time *ADTime = new CADAGUC_time((char*)timeUnits->data);
      stADAGUC_time timest;
      status = ADTime->OffsetToAdaguc(timest,((double*)time->data)[index]);
      if(status == 0){
        char ISOTime[MAX_STR_LEN+1];
        ADTime->PrintISOTime(ISOTime,MAX_STR_LEN,timest);
        ISOTime[19]='Z';ISOTime[20]='\0';
        snprintf(pszTime,MAX_STR_LEN,"%s",ISOTime);
      }
      delete ADTime;
    }
  }else{
    snprintf(pszTime,MAX_STR_LEN,"(time missing: dims = %d)",sourceImage->dNetCDFNumDims);
  }
  return 0;
}

int CDataReader::createDBUpdateTables(CPGSQLDB *DB,CDataSource *sourceImage,int &removeNonExistingFiles){
  //First check and create all tables...
  for(size_t d=0;d<sourceImage->cfgLayer->Dimension.size();d++){
    bool isTimeDim = false;
    CT::string dimName(sourceImage->cfgLayer->Dimension[d]->attr.name.c_str());
    dimName.toLowerCase();
    if(dimName.equals("time"))isTimeDim=true;
    //Create database tablenames
    CT::string tablename(sourceImage->cfgLayer->DataBaseTable[0]->value.c_str());
    makeCorrectTableName(&tablename,&dimName);
    //Create column names
    CT::string tableColumns("path varchar (255)");
    if(isTimeDim==true){
      tableColumns.printconcat(", time timestamp, dimtime int");
    }else{
      tableColumns.printconcat(", %s real, dim%s int",dimName.c_str(),dimName.c_str());
    }
    // Test if the non-temporary table exists , if not create the table
    printf("Check table %s ...\t",tablename.c_str());
    status = DB->checkTable(tablename.c_str(),tableColumns.c_str());
    if(status == 0)printf("OK: Table is available\n");
    if(status == 1){CDBError("\nFAIL: Table %s could not be created: %s",tablename.c_str(),tableColumns.c_str()); DB->close();return 1;  }
    if(status == 2){
      //removeExisting files can be set back to zero, because there are no files available to remove
      //note the int &removeNonExistingFiles as parameter of this function!
      //(Setting the value will have effect outside this function)
      removeNonExistingFiles=0;
      printf("OK: Table %s created, (check for unavailable files is off);\n",tablename.c_str());
    }
    
    
    if(removeNonExistingFiles==1){
      //The temporary table should always be dropped before filling.  
      //We will do a complete new update, so store everything in an new table
      //Later we will rename this table
      CT::string tablename_temp(&tablename);
      if(removeNonExistingFiles==1){
        tablename_temp.concat("_temp");
      }
      printf("Making empty temporary table %s ... ",tablename_temp.c_str());
      CT::string query;
      status=DB->checkTable(tablename_temp.c_str(),tableColumns.c_str());
      if(status == 1){CDBError("\nFAIL: Table %s could not be created: %s",tablename_temp.c_str(),tableColumns.c_str()); DB->close();return 1;  }
      if(status == 2)printf("OK\n");
      if(status==0){
        query.print("drop table %s",tablename_temp.c_str());
        printf("\n*** %s\n",query.c_str());
        if(DB->query(query.c_str())!=0){
          CDBError("Query %s failed",query.c_str());
          DB->close();
          return 1;
        }
        printf("Check table %s ... ",tablename_temp.c_str());
        status = DB->checkTable(tablename_temp.c_str(),tableColumns.c_str());
        if(status == 0)printf("OK: Table is available\n");
        if(status == 1){CDBError("\nFAIL: Table %s could not be created: %s",tablename_temp.c_str(),tableColumns.c_str()); DB->close();return 1;  }
        if(status == 2)printf("OK: Table %s created\n",tablename_temp.c_str());
      }
    }

  }
  return 0;
}



int CDataReader::DBLoopFiles(CPGSQLDB *DB,CDataSource *sourceImage,int removeNonExistingFiles,CDirReader *dirReader){
  int status = 0;
  try{
    //Loop dimensions and files
    printf("Adding files that are now available...\n");
    char ISOTime[MAX_STR_LEN+1];
    for(size_t j=0;j<dirReader->fileList.size();j++){
      for(size_t d=0;d<sourceImage->cfgLayer->Dimension.size();d++){
        int fileExistsInDB=0;
        bool isTimeDim = false;
        CT::string dimName(sourceImage->cfgLayer->Dimension[d]->attr.name.c_str());
        dimName.toLowerCase();
        if(dimName.equals("time"))isTimeDim=true;
        //Create database tablenames
        CT::string tablename(sourceImage->cfgLayer->DataBaseTable[0]->value.c_str());
        makeCorrectTableName(&tablename,&dimName);
        //Create column names
        CT::string tableColumns("path varchar (255)");
        if(isTimeDim==true){
          tableColumns.printconcat(", time timestamp, dimtime int");
        }else{
          tableColumns.printconcat(", %s real, dim%s int",dimName.c_str(),dimName.c_str());
        }
        //Create temporary tablename
        CT::string tablename_temp(&tablename);
        if(removeNonExistingFiles==1){
          tablename_temp.concat("_temp");
        }
        //If we are messing in the non-temporary table (e.g.removeNonExistingFiles==0)
        //we need to make a transaction to make sure a file is not added twice
        //If removeNonExistingFiles==1, we are using the temporary table
        //Which is already locked by a transaction 
        if(removeNonExistingFiles==0){
          //CDBDebug("BEGIN");
          status = DB->query("BEGIN"); if(status!=0)throw(__LINE__);
        }

        // Check if file already resides in the database
        CT::string query;
        query.print("select path from %s where path = '%s' limit 1",
                tablename.c_str(),
                dirReader->fileList[j]->fullName.c_str());
        CT::string *pathValues = DB->query_select(query.c_str(),0);
        if(pathValues == NULL){CDBError("Query failed");DB->close();throw(__LINE__);}
        // Does the file already reside in the DB?
        if(pathValues->count==1)
          fileExistsInDB=1; //YES file is already in the database
        else
          fileExistsInDB=0; // NO
        delete[] pathValues;
        
        //Insert the record corresponding to this file from the nontemporary table into the temporary table
        if(fileExistsInDB == 1&&removeNonExistingFiles==1){
          //The file resides already in the database, copy it into the new table with _temp
          CT::string mvRecordQuery;
          mvRecordQuery.print("INSERT INTO %s select path,%s,dim%s from %s where path = '%s'",
                              tablename_temp.c_str(),
                              dimName.c_str(),
                              dimName.c_str(),
                              tablename.c_str(),
                              dirReader->fileList[j]->fullName.c_str()
                             );
          //printf("%s\n",mvRecordQuery.c_str());
          if(DB->query(mvRecordQuery.c_str())!=0){CDBError("Query %s failed",mvRecordQuery.c_str());throw(__LINE__);}
        }

        //The file metadata does not already reside in the db.
        //Therefore we need to read information from it
        if(fileExistsInDB == 0){
          try{
            printf("Adding fileNo %d/%d\t %s",
                  (int)j,
                  (int)dirReader->fileList.size(),
                  dirReader->fileList[j]->baseName.c_str());
            //Create CDF object
            cdfObject=new CDFObject();
            cdfReader = getCDFReader(sourceImage);
            if(cdfReader == NULL)throw(__LINE__);
            
            //Open the file
            status = cdfReader->open(dirReader->fileList[j]->fullName.c_str());
            if(status!=0){
              CDBError("Unable to open file '%s'",dirReader->fileList[j]->fullName.c_str());
              throw(__LINE__);
            }
            if(status==0){
              CDF::Dimension * dimDim = cdfObject->getDimension(sourceImage->cfgLayer->Dimension[d]->attr.name.c_str());
              CDF::Variable *  dimVar = cdfObject->getVariable(sourceImage->cfgLayer->Dimension[d]->attr.name.c_str());
              if(dimDim==NULL||dimVar==NULL){
                CDBError("In file %s dimension %s not found",dirReader->fileList[j]->fullName.c_str(),sourceImage->cfgLayer->Dimension[0]->attr.name.c_str());
                throw(__LINE__);
              }else{
                CDF::Attribute *dimUnits = dimVar->getAttribute("units");
                if(dimUnits==NULL){
                  CDBError("No units found for dimension %s",dimVar->name.c_str());
                  throw(__LINE__);
                }
                status = cdfReader->readVariableData(dimVar,CDF_DOUBLE);if(status!=0){
                  CDBError("Unable to read variable data for %s",dimVar->name.c_str());
                  throw(__LINE__);
                }
                //Add other dim than time
                if(isTimeDim==false){
                  const double *dimValues=(double*)dimVar->data;
                  for(size_t i=0;i<dimDim->length;i++){
                    CT::string queryString;
                    queryString.print("INSERT into %s VALUES ('%s','%f','%d')",
                            tablename_temp.c_str(),
                            dirReader->fileList[j]->fullName.c_str(),
                            double(dimValues[i]),
                            int(i));
                    status = DB->query(queryString.c_str()); if(status!=0)throw(__LINE__);
                    if(removeNonExistingFiles==1){
                      //We are adding the query above to the temporay table if removeNonExistingFiles==1;
                      //Lets add it also to the real table for convenience
                      //Later this table will be dropped, but it is more up to date this way.
                      CT::string queryString;
                      queryString.print("INSERT into %s VALUES ('%s','%f','%d')",
                                        tablename_temp.c_str(),
                                        dirReader->fileList[j]->fullName.c_str(),
                                        double(dimValues[i]),
                                        int(i));
                      status = DB->query(queryString.c_str()); if(status!=0)throw(__LINE__);
                    }
                  }
                }
                //Add time dim:
                if(isTimeDim==true){
                  const double *dtimes=(double*)dimVar->data;
                  CADAGUC_time *ADTime = new CADAGUC_time((char*)dimUnits->data);
                  for(size_t i=0;i<dimDim->length;i++){
                    if(dtimes[i]!=NC_FILL_DOUBLE){
                      ADTime->PrintISOTime(ISOTime,MAX_STR_LEN,dtimes[i]);
                      status = 0;//TODO make PrintISOTime return a 0 if succeeded
                      if(status == 0){
                        ISOTime[19]='Z';ISOTime[20]='\0';
                        printf("%s\n", ISOTime);
                        CT::string queryString;
                        queryString.print("INSERT into %s VALUES ('%s','%s','%d')",
                                tablename_temp.c_str(),
                                dirReader->fileList[j]->fullName.c_str(),
                                ISOTime,
                                int(i));
                        status = DB->query(queryString.c_str()); if(status!=0)throw(__LINE__);
                        if(removeNonExistingFiles==1){
                          //We are adding the query above to the temporay table if removeNonExistingFiles==1;
                          //Lets add it also to the real table for convenience
                          //Later this table will be dropped, but it is more up to date this way.
                          CT::string queryString;
                          queryString.print("INSERT into %s VALUES ('%s','%s','%d')",
                                            tablename.c_str(),
                                            dirReader->fileList[j]->fullName.c_str(),
                                            ISOTime,
                                            int(i));
                          status = DB->query(queryString.c_str()); if(status!=0)throw(__LINE__);
                        }
                      }else {
                        CDBError("Time attribute invalid in [%s]",dirReader->fileList[j]->fullName.c_str());
                      }
                    }
                  }
                  delete ADTime;
                }
              }
            }
            if(cdfReader!=NULL)cdfReader->close();
            delete cdfReader;cdfReader=NULL;
            delete cdfObject;cdfObject=NULL;
          }catch(int linenr){
            fprintf(stderr,"*** SKIPPING FILE *** File open exception in DBLoopFiles at line %d\n",linenr);
            //Close cdfReader. this is only needed if an exception occurs, otherwise it does nothing...
            if(cdfReader!=NULL)cdfReader->close();
            delete cdfReader;cdfReader=NULL;
            delete cdfObject;cdfObject=NULL;
          }
        }
        //If we are messing in the non-temporary table (e.g.removeNonExistingFiles==0)
        //we need to make a transaction to make sure a file is not added twice
        //If removeNonExistingFiles==1, we are using the temporary table
        //Which is already locked by a transaction 
        if(removeNonExistingFiles==0){
          //CDBDebug("COMMIT");
          status = DB->query("COMMIT"); if(status!=0)throw(__LINE__);
        }
      }
    }
    if(status != 0){CDBError(DB->getError());throw(__LINE__);}
    }
    catch(int linenr){
      //Exception handling!
      //Always do a COMMIT after an exception to be sure a transaction will not hang
      //CDBDebug("COMMIT");
      DB->query("COMMIT"); 
      CDBError("Exception in DBLoopFiles at line %d",linenr);
      //Close cdfReader. this is only needed if an exception occurs, otherwise it does nothing...
      if(cdfReader!=NULL)cdfReader->close();
      delete cdfReader;cdfReader=NULL;
      delete cdfObject;cdfObject=NULL;
      return 1;
    }
    //Not really necessary...
    if(cdfReader!=NULL)cdfReader->close();
    delete cdfReader;cdfReader=NULL;
    delete cdfObject;cdfObject=NULL;
    return 0;
}



int CDataReader::updatedb(const char *pszDBParams, CDataSource *sourceImage,CT::string *_tailPath,CT::string *_layerPathToScan){
  if(sourceImage->dLayerType!=CConfigReaderLayerTypeDataBase)return 0;
  
  //We only need to update the provided path in layerPathToScan. We will simply ignore the other directories
  if(_layerPathToScan!=NULL){
    if(_layerPathToScan->length()!=0){
      CT::string layerPath,layerPathToScan;
      layerPath.copy(sourceImage->cfgLayer->FilePath[0]->value.c_str());
      layerPathToScan.copy(_layerPathToScan);
      CDirReader *temp = new CDirReader();
      temp->makeCleanPath(&layerPath);
      temp->makeCleanPath(&layerPathToScan);
      delete temp;
      //If this is another directory we will simply ignore it.
      if(layerPath.equals(&layerPathToScan)==false){
        //printf("Skipping %s\n",layerPath.c_str());
        return 0;
      }
    }
  }
  // This variable enables the query to remove files that no longer exist in the filesystem
  int removeNonExistingFiles=1;

  int status;
  CPGSQLDB DB;

  //Copy tailpath (can be provided to scan only certain subdirs)
  CT::string tailPath(_tailPath);
  CDirReader *temp = new CDirReader();
  temp->makeCleanPath(&tailPath);
  delete temp;
  //if tailpath is defined than removeNonExistingFiles must be zero
  if(tailPath.length()>0)removeNonExistingFiles=0;

  CT::string FilePath(sourceImage->cfgLayer->FilePath[0]->value.c_str());
  FilePath.concat(&tailPath);
  //temp = new CDirReader();
  //temp->makeCleanPath(&FilePath);
  //delete temp;
  
  CDirReader dirReader;
  
  printf("\n*** Starting update layer \"%s\" with title \"%s\"\n",sourceImage->cfgLayer->Name[0]->value.c_str(),sourceImage->cfgLayer->Title[0]->value.c_str());
    
  if(FilePath.lastIndexOf(".nc")>0){
    CFileObject * fileObject = new CFileObject();
    fileObject->fullName.copy(&FilePath);
    fileObject->baseName.copy(&FilePath);
    fileObject->isDir=false;
    dirReader.fileList.push_back(fileObject);
  }else{
    dirReader.makeCleanPath(&FilePath);
    try{
      CT::string fileFilterExpr("\\.nc");
      if(sourceImage->cfgLayer->FilePath[0]->attr.filter.c_str()!=NULL){
        fileFilterExpr.copy(sourceImage->cfgLayer->FilePath[0]->attr.filter.c_str());
      }
      printf("Reading directory %s with filter %s\n",FilePath.c_str(),fileFilterExpr.c_str()); 
      dirReader.listDirRecursive(FilePath.c_str(),fileFilterExpr.c_str());
    }catch(const char *msg){
      printf("Directory %s does not exist, silently skipping...\n",FilePath.c_str());
      return 0;
    }
  }
  printf("Found %d file(s) in directory\n",int(dirReader.fileList.size()));
  if(dirReader.fileList.size()==0)return 0;
  
  /*----------- Connect to DB --------------*/
  printf("Connecting to DB %s ...\t",pszDBParams);
  status = DB.connect(pszDBParams);if(status!=0){
    CDBError("FAILED...");
    CDBError("Error Could not connect to the database with parameters: [%s]",pszDBParams);
    return 1;
  }
  printf("OK\n");
  try{ 
    //First check and create all tables...
    status = createDBUpdateTables(&DB,sourceImage,removeNonExistingFiles);
    if(status != 0 )throw(__LINE__);
    
    //printf("removeNonExistingFiles = %d\n",removeNonExistingFiles);
    //We need to do a transaction if we want to remove files from the existing table
    if(removeNonExistingFiles==1){
      //CDBDebug("BEGIN");
      status = DB.query("BEGIN"); if(status!=0)throw(__LINE__);
    }
    

    //Loop Through all files
    status = DBLoopFiles(&DB,sourceImage,removeNonExistingFiles,&dirReader);
    if(status != 0 )throw(__LINE__);
  
    //In case of a complete update, the data is written in a temporary table
    //Rename the table to the correct one (remove _temp)
    if(removeNonExistingFiles==1){
      for(size_t d=0;d<sourceImage->cfgLayer->Dimension.size();d++){
        CT::string dimName(sourceImage->cfgLayer->Dimension[d]->attr.name.c_str());
        CT::string tablename(sourceImage->cfgLayer->DataBaseTable[0]->value.c_str());
        makeCorrectTableName(&tablename,&dimName);
        printf("Renaming temporary table... %s\n",tablename.c_str());
        CT::string query;
        //Drop old table
        query.print("drop table %s",tablename.c_str());
        if(DB.query(query.c_str())!=0){CDBError("Query %s failed",query.c_str());DB.close();throw(__LINE__);}
        //Rename new table to old table name
        query.print("alter table %s_temp rename to %s",tablename.c_str(),tablename.c_str());
        if(DB.query(query.c_str())!=0){CDBError("Query %s failed",query.c_str());DB.close();throw(__LINE__);}
        if(status!=0){throw(__LINE__);}
      }
    }

  }
  catch(int linenr){
    CDBError("Exception in updatedb at line %d",linenr);
  }
  // Close DB
  //CDBDebug("COMMIT");
  if(removeNonExistingFiles==1)status = DB.query("COMMIT");
  status = DB.close();if(status!=0)return 1;

  printf("*** Finished\n");
  //printStatus("OK","HOi %s","Maarten");
  return 0;
}

//Table names need to be different between dims like time and height.
// Therefore create unique tablenames like tablename_time and tablename_height
void makeCorrectTableName(CT::string *tableName,CT::string *dimName){
  tableName->concat("_");tableName->concat(dimName);
  tableName->toLowerCase();
}

