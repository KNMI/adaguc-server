#include "CNetCDFDataWriter.h"
#include "CGenericDataWarper.h"
const char * CNetCDFDataWriter::className = "CNetCDFDataWriter";

//#define CNetCDFDataWriter_DEBUG

void CNetCDFDataWriter::createProjectionVariables(CDFObject *cdfObject,int width,int height,double *bbox){
  bool isProjected=true;
  if(projectionDimX!=NULL){
    CDBWarning("createProjectionVariables already done");
    return;
  }
  projectionDimX = new CDF::Dimension();
  cdfObject->addDimension(projectionDimX);
  if(isProjected){
    projectionDimX->name="x";
  }else{
    projectionDimX->name="lon";
  }
  projectionDimX->setSize(width);
  
  projectionVarX = new CDF::Variable();
  cdfObject->addVariable(projectionVarX);
  projectionVarX->setType(CDF_DOUBLE);
  projectionVarX->name.copy(projectionDimX->name.c_str());
  projectionVarX->isDimension=true;
  projectionVarX->dimensionlinks.push_back(projectionDimX);
  
  CDF::allocateData(CDF_DOUBLE,&projectionVarX->data,width);
  
  projectionDimY = new CDF::Dimension();
  cdfObject->addDimension(projectionDimY);
  if(isProjected){
    projectionDimY->name="y";
  }else{
    projectionDimY->name="lat";
  }
  projectionDimY->setSize(height);

  projectionVarY = new CDF::Variable();
  cdfObject->addVariable(projectionVarY);
  projectionVarY->setType(CDF_DOUBLE);
  projectionVarY->name.copy(projectionDimY->name.c_str());
  projectionVarY->isDimension=true;
  projectionVarY->dimensionlinks.push_back(projectionDimY);
  
  CDF::allocateData(CDF_DOUBLE,&projectionVarY->data,height);
  
  if(isProjected){
    projectionVarX->setAttributeText("standard_name","projection_x_coordinate");
    projectionVarY->setAttributeText("standard_name","projection_y_coordinate");
  }else{
    projectionVarX->setAttributeText("standard_name","longitude");
    projectionVarX->setAttributeText("units","degrees");
    projectionVarY->setAttributeText("standard_name","latitude");
    projectionVarY->setAttributeText("units","degrees");
  }
  
  double cellSizeX = (bbox[2]-bbox[0])/(double(width));
  double cellSizeY = (bbox[1]-bbox[3])/(double(height));
  
  for(int x=0;x<width;x++){
    ((double*)projectionVarX->data)[x]=bbox[0]+double(x)*cellSizeX+cellSizeX/2;
  }
  for(int y=0;y<height;y++){
    ((double*)projectionVarY->data)[y]=bbox[3]+double(y)*cellSizeY+cellSizeY/2;
  }
  
}


int CNetCDFDataWriter::init(CServerParams *srvParam,CDataSource *dataSource, int nrOfBands){
  
#ifdef CNetCDFDataWriter_DEBUG
  CDBDebug(">CNetCDFDataWriter::init");
#endif
  baseDataSource = dataSource;
  this->srvParam = srvParam;
  destCDFObject = new CDFObject();
  CT::string randomString = CServerParams::randomString(32);
  tempFileName.print("%s/%s.nc",srvParam->cfg->TempDir[0]->attr.value.c_str(),randomString.c_str());
  CDataReader reader;
  int status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
  if(status!=0){
    CDBError("Could not open file: %s",dataSource->getFileName());
    return 1;
  }
  
  CT::string timeUnits;
  try{
    timeUnits = reader.getTimeUnit(dataSource);
  }catch(int e){
    timeUnits = "";
  }

  // Copy global attributes
  CDFObject * srcObj=dataSource->getDataObject(0)->cdfObject;
  for(size_t j=0;j<srcObj->attributes.size();j++){
    destCDFObject->setAttribute(srcObj->attributes[j]->name.c_str(),srcObj->attributes[j]->type,srcObj->attributes[j]->data,srcObj->attributes[j]->length);
  }
  
  double dfDstBBOX[4];
  double dfSrcBBOX[4];
  //Setup projection
  // Set up geo parameters
  if(srvParam->WCS_GoNative==1){
    //Native!
    for(int j=0;j<4;j++)dfSrcBBOX[j]=dataSource->dfBBOX[j];
    dfDstBBOX[0]=dfSrcBBOX[0];
    dfDstBBOX[1]=dfSrcBBOX[3];
    dfDstBBOX[2]=dfSrcBBOX[2];
    dfDstBBOX[3]=dfSrcBBOX[1];
    srvParam->Geo->dWidth=dataSource->dWidth;
    srvParam->Geo->dHeight=dataSource->dHeight;
    srvParam->Geo->CRS.copy(&dataSource->nativeProj4);
    if(srvParam->Format.length()==0)srvParam->Format.copy("adagucnetcdf");
  }else{
    // Non native projection units
    for(int j=0;j<4;j++)dfSrcBBOX[j]=dataSource->dfBBOX[j];
    for(int j=0;j<4;j++)dfDstBBOX[j]=srvParam->Geo->dfBBOX[j];
    // dfResX and dfResY are in the CRS ore ResponseCRS
    if(srvParam->dWCS_RES_OR_WH==1){
      srvParam->Geo->dWidth=int(((dfDstBBOX[2]-dfDstBBOX[0])/srvParam->dfResX));
      srvParam->Geo->dHeight=int(((dfDstBBOX[1]-dfDstBBOX[3])/srvParam->dfResY));
      srvParam->Geo->dHeight=abs(srvParam->Geo->dHeight);
    }
    if(srvParam->Geo->dWidth>20000||srvParam->Geo->dHeight>20000){
      CDBError("Requested Width or Height is larger than 20000 pixels. Aborting request.");
      return 1;
    }
  }
  //Create projection variables
  createProjectionVariables(destCDFObject,srvParam->Geo->dWidth,srvParam->Geo->dHeight,dfDstBBOX);
 
  
  
  //Create other NonGeo dimensions
  //CCDFDims *dims = baseDataSource->getCDFDims();
#ifdef CNetCDFDataWriter_DEBUG    
  CDBDebug("Number of requireddims=%d",baseDataSource->requiredDims.size());
#endif
  for(size_t d=0;d<baseDataSource->requiredDims.size();d++){
    
    CT::string dimName = "null";

    dimName = baseDataSource->requiredDims[d]->netCDFDimName;

    CCDFDims *cdfDims = baseDataSource->getCDFDims();
    
    int dimIndex=cdfDims->getArrayIndexForName(dimName.c_str());
    if(dimIndex == -1){
      CDBError("Unable to find dimension %s",dimName.c_str());
      return 1;
    }
    bool isTimeDim=cdfDims->isTimeDimension(dimIndex);
    
    CDF::Variable *sourceVar = srcObj->getVariable(dimName.c_str());
    
    CDF::Dimension *dim = new CDF::Dimension();
    destCDFObject->addDimension(dim);
    dim->name = dimName;
   
    CDF::Variable *var = new CDF::Variable();
    destCDFObject->addVariable(var);
    if(isTimeDim){
      var->setType(CDF_DOUBLE);
    }else{
      var->setType(sourceVar->getType());
    }
    var->name.copy(dim->name.c_str());
    var->isDimension=true;
    var->dimensionlinks.push_back(dim);

    dim->setSize(baseDataSource->requiredDims[d]->uniqueValues.size());
#ifdef CNetCDFDataWriter_DEBUG        
    CDBDebug("Adding dimension [%s] with type [%d] and length [%d]",dimName.c_str(),var->getType(),dim->length);
#endif    
    
    var->setSize(dim->length);
    CDF::allocateData(var->getType(),&var->data,dim->length);
    
    if(CDF::fill(var->data,var->getType(),0,var->getSize())!=0){
      CDBError("Unable to initialize data field to nodata value");
      return 1;
    }
    
   
    //Fill dimension with correct data
    for(size_t j=0;j<baseDataSource->requiredDims[d]->uniqueValues.size();j++){
      CT::string dimValue=baseDataSource->requiredDims[d]->uniqueValues[j].c_str();
#ifdef CNetCDFDataWriter_DEBUG
      CDBDebug("Setting dimension %s value = %s",dimName.c_str(),dimValue.c_str());
#endif        
      if(var->getType()==CDF_STRING){
#ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("Dimension [%s]: writing string value %s to index %d",dimName.c_str(),dimValue.c_str(),j);
#endif 
        ((char**)var->data)[j]=strdup(dimValue.c_str());
      }
      
      if(var->getType()!=CDF_STRING){
        if(isTimeDim){
          CTime ctime;
          ctime.init(timeUnits.c_str());
          double offset = ctime.dateToOffset(ctime.freeDateStringToDate(dimValue.c_str()));
#ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("Dimension [%s]: writing string value %s with units %s, offset %f to index %d",dimName.c_str(),dimValue.c_str(),timeUnits.c_str(),offset,j);
#endif       
          double *dimData = ((double*)var->data);
          dimData[j]=offset;
        } else{
    #ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("Dimension [%s]: writing scalar value %s to index %d",dimName.c_str(),dimValue.c_str(),j);
#endif 
          switch(var->getType()){
            case CDF_CHAR  : ((char*)          var->data)[j]=dimValue.toInt();break;
            case CDF_BYTE  : ((char*)          var->data)[j]=dimValue.toInt();break;
            case CDF_UBYTE : ((unsigned char*) var->data)[j]=dimValue.toInt();break;
            case CDF_SHORT : ((short*)         var->data)[j]=dimValue.toInt();break;
            case CDF_USHORT: ((unsigned short*)var->data)[j]=dimValue.toInt();break;
            case CDF_INT   : ((int*)           var->data)[j]=dimValue.toInt();break;
            case CDF_UINT  : ((unsigned int*)  var->data)[j]=dimValue.toInt();break;
            case CDF_FLOAT : ((float*)         var->data)[j]=dimValue.toFloat();break;
            case CDF_DOUBLE: ((double*)        var->data)[j]=dimValue.toDouble();break;
            default:return 1;
          }
        }
      }
    }
  
    
    //Copy dim attributes
    for(size_t i=0;i<sourceVar->attributes.size();i++){
      var->setAttribute(sourceVar->attributes[i]->name.c_str(),sourceVar->attributes[i]->getType(),sourceVar->attributes[i]->data,sourceVar->attributes[i]->length);
    } 
  
  }
  
  //Create variables
  size_t varSize = 1;
  for(size_t j=0;j<baseDataSource->getNumDataObjects();j++){
    CDF::Variable *destVar = new CDF::Variable();
    destCDFObject->addVariable(destVar);
    CDF::Variable *sourceVar = baseDataSource->getDataObject(j)->cdfVariable;
    destVar->name.copy(sourceVar->name.c_str());
    CDBDebug("Name = %s, type = %d",sourceVar->name.c_str(),sourceVar->getType());
    destVar->setType(sourceVar->getType());
    for(size_t i=0;i<sourceVar->dimensionlinks.size();i++){
      CDF::Dimension *d = destCDFObject->getDimensionNE(sourceVar->dimensionlinks[i]->name.c_str());
      if(d==NULL){
        if(i==(sourceVar->dimensionlinks.size()-1)-0){
          d=projectionDimX;
        }
        if(i==(sourceVar->dimensionlinks.size()-1)-1){
          d=projectionDimY;
        }
      }
      if(d==NULL){
        CDBError("Unable to add dimension nr %d, name[%s]",i,sourceVar->dimensionlinks[i]->name.c_str());
        throw(__LINE__);
      }
      destVar->dimensionlinks.push_back(d);
      varSize*=d->getSize();
      
    }
    CDBDebug("Allocating %d elements for variable %s",varSize/(projectionDimX->getSize()*projectionDimY->getSize()),destVar->name.c_str());
    CDF::allocateData(destVar->getType(),&destVar->data,varSize);
    double dfNoData = NAN;
    if(dataSource->getDataObject(0)->hasNodataValue==1){
      dfNoData=dataSource->getDataObject(0)->dfNodataValue;
    }
    
    
    
    if(CDF::fill(destVar->data,destVar->getType(),dfNoData,varSize)!=0){
      CDBError("Unable to initialize data field to nodata value");
      return 1;
    }
    for(size_t i=0;i<sourceVar->attributes.size();i++){
      destVar->setAttribute(sourceVar->attributes[i]->name.c_str(),sourceVar->attributes[i]->getType(),sourceVar->attributes[i]->data,sourceVar->attributes[i]->length);
    }
    //destCDFObject->setAttribute("_FillValue",destVar->getType(),&dfNoData,1);
    
    //destCDFObject->removeAttribute("grid_mapping");
    destVar->setAttributeText("grid_mapping","crs");
   
  }
  
  CDF::Variable *crs = new CDF::Variable();
  crs->name="crs";
  crs->setType(CDF_CHAR);
  crs->setSize(0);
  
  
  destCDFObject->addVariable(crs);
  #ifdef CNetCDFDataWriter_DEBUG
  CDBDebug("<CNetCDFDataWriter::init");
  #endif
 
  return 0;
}

int CNetCDFDataWriter::addData(std::vector <CDataSource*> &dataSources){

  int status;
  
  for(size_t i=0;i<dataSources.size();i++){
    CDataSource *dataSource = dataSources[i];
    CDataReader reader;
    status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL);
    if(status!=0){
      CDBError("Could not open file: %s",dataSource->getFileName());
      return 1;
    }
      
    CT::string timeUnits;
    try{
      timeUnits = reader.getTimeUnit(dataSource);
    }catch(int e){
      timeUnits = "";
    }

    CDF::Variable *variable = destCDFObject->getVariable(dataSource->getDataObject(0)->cdfVariable->name.c_str());;
    
    //Set dimension
    CCDFDims *dims = dataSource->getCDFDims();
    #ifdef CNetCDFDataWriter_DEBUG    
    CDBDebug("Setting dimension,nr %d",dims->getNumDimensions());
#endif
    /*
     * This step figures out the required dimindex for each dimension based on timestep.
     */
    int dimIndices[dims->getNumDimensions()+1];
    

    
    for(size_t d=0;d<baseDataSource->requiredDims.size();d++){
      CT::string dimName = baseDataSource->requiredDims[d]->netCDFDimName;
      CCDFDims *cdfDims = baseDataSource->getCDFDims();
      int dimIndex=cdfDims->getArrayIndexForName(dimName.c_str());
      if(dimIndex == -1){
        CDBError("Unable to find dimension %s",dimName.c_str());
        return 1;
      }
      bool isTimeDim=cdfDims->isTimeDimension(dimIndex);
      CT::string dimValue = baseDataSource->getDimensionValueForNameAndStep(dimName.c_str(),dataSource->getCurrentTimeStep());
      int indexTofind = -1;
      CDF::Variable *var=destCDFObject->getVariable(dimName.c_str());
      if(var->getType()==CDF_STRING){
        for(size_t j=0;j<var->getSize();j++){
          if(dimValue.equals(((char**)var->data)[j])){
            indexTofind = j;
            break;
          }
        }
      }
      
      if(var->getType()!=CDF_STRING){
        if(isTimeDim){
          CTime ctime;
          ctime.init(timeUnits.c_str());
          double offset = ctime.dateToOffset(ctime.freeDateStringToDate(dimValue.c_str()));
          for(size_t j=0;j<var->getSize();j++){
            if(((double*)var->data)[j]==offset){
              indexTofind = j;
              break;
            }
          }
        } else{
          double valueToFind=dimValue.toDouble();
          for(size_t j=0;j<var->getSize();j++){
            double value;
            switch(var->getType()){
              case CDF_CHAR  : value = ((char*)          var->data)[j];break;
              case CDF_BYTE  : value = ((char*)          var->data)[j];break;
              case CDF_UBYTE : value = ((unsigned char*) var->data)[j];break;
              case CDF_SHORT : value = ((short*)         var->data)[j];break;
              case CDF_USHORT: value = ((unsigned short*)var->data)[j];break;
              case CDF_INT   : value = ((int*)           var->data)[j];break;
              case CDF_UINT  : value = ((unsigned int*)  var->data)[j];break;
              case CDF_FLOAT : value = ((float*)         var->data)[j];break;
              case CDF_DOUBLE: value = ((double*)        var->data)[j];break;
              default:return 1;
            }
            if(value == valueToFind){
              indexTofind = j;
              break;
            }
          }
        }
      }
      if(indexTofind == -1){
        CDBError("Unable to find dim value %s in destination object",dimValue.c_str());
        return 1;
      }
#ifdef CNetCDFDataWriter_DEBUG      
      CDBDebug("Found dimindex %d for dimvalue %s",indexTofind,dimValue.c_str());
#endif      
      dimIndices[d]=indexTofind;
    }

    int _dimMultiplier=1;
    int dimMultipliers[dims->getNumDimensions()+1];
    for(size_t d=0;d<baseDataSource->requiredDims.size();d++){
      dimMultipliers[(baseDataSource->requiredDims.size()-1)-d]=_dimMultiplier;
      _dimMultiplier*=baseDataSource->requiredDims[(baseDataSource->requiredDims.size()-1)-d]->uniqueValues.size();
    }
    
    
    #ifdef CNetCDFDataWriter_DEBUG
    for(size_t d=0;d<baseDataSource->requiredDims.size();d++){
     CDBDebug("For [%s]: index = %d, multiplier = %d",baseDataSource->requiredDims[d]->name.c_str(),dimIndices[d],dimMultipliers[d]);
    }
    #endif
    
    int dataStepIndex = 0;
    for(size_t d=0;d<baseDataSource->requiredDims.size();d++){
      dataStepIndex+=dimMultipliers[d]*dimIndices[d];
    }
    
    #ifdef CNetCDFDataWriter_DEBUG
      CDBDebug("DataStep index = %d, timestep = %d",dataStepIndex,dataSource->getCurrentTimeStep());
    #endif
    //Warp
    CImageWarper warper;
    
    status = warper.initreproj(dataSource,srvParam->Geo,&srvParam->cfg->Projection);
    if(status != 0){
      CDBError("Unable to initialize projection");
      return 1;
    }
    
    destCDFObject->getVariable("crs")->setAttributeText("proj4_params",warper.getDestProjString().c_str());
    CGeoParams sourceGeo;
    
    sourceGeo.dWidth = dataSource->dWidth;
    sourceGeo.dHeight = dataSource->dHeight;
    sourceGeo.dfBBOX[0] = dataSource->dfBBOX[0];
    sourceGeo.dfBBOX[1] = dataSource->dfBBOX[1];
    sourceGeo.dfBBOX[2] = dataSource->dfBBOX[2];
    sourceGeo.dfBBOX[3] = dataSource->dfBBOX[3];
    sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
    sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
    sourceGeo.CRS = dataSource->nativeProj4;
    
    void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;
    
    Settings settings;
    settings.width = srvParam->Geo->dWidth;
    settings.height = srvParam->Geo->dHeight;
    
    size_t elementOffset = dataStepIndex*settings.width*settings.height;
    void *warpedData = NULL;

    switch(variable->getType()){
      case CDF_CHAR  : warpedData = ((char*)variable->data)+elementOffset;break;
      case CDF_BYTE  : warpedData = ((char*)variable->data)+elementOffset;break;
      case CDF_UBYTE : warpedData = ((unsigned char*)variable->data)+elementOffset;break;
      case CDF_SHORT : warpedData = ((short*)variable->data)+elementOffset;break;
      case CDF_USHORT: warpedData = ((unsigned short*)variable->data)+elementOffset;break;
      case CDF_INT   : warpedData = ((int*)variable->data)+elementOffset;break;
      case CDF_UINT  : warpedData = ((unsigned int*)variable->data)+elementOffset;break;
      case CDF_FLOAT : warpedData = ((float*)variable->data)+elementOffset;break;
      case CDF_DOUBLE: warpedData = ((double*)variable->data)+elementOffset;break;
      default:return 1;
    }
    settings.data=warpedData;
    GenericDataWarper::render<float> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);/*break;*/
    reader.close();
  }
  
  return 0;
  
}


int CNetCDFDataWriter::writeFile(const char *fileName,int level){
  if(level!=-1){
    destCDFObject->setAttribute("adaguctilelevel",CDF_INT,&level,1);
  }
  CDFNetCDFWriter *netCDFWriter = new CDFNetCDFWriter(destCDFObject);
  netCDFWriter->setNetCDFMode(4);
  int  status = netCDFWriter->write(fileName);
  delete netCDFWriter;
  
  if(status!=0){
    CDBError("Unable to write file to temporary directory");
    return 1;
  }
  return 0;
}

int CNetCDFDataWriter::end(){
  CDFNetCDFWriter *netCDFWriter = new CDFNetCDFWriter(destCDFObject);
  netCDFWriter->setNetCDFMode(4);
  int  status = netCDFWriter->write(tempFileName.c_str());
  
  delete netCDFWriter;
  
  
  if(status!=0){
    CDBError("Unable to write file to temporary directory");
    return 1;
  }
  
  CT::string humanReadableString;
  humanReadableString.copy(srvParam->Format.c_str());
  humanReadableString.concat("_");
  humanReadableString.concat(baseDataSource->getDataObject(0)->variableName.c_str());
  for(size_t i=0;i<baseDataSource->requiredDims.size();i++){
    humanReadableString.printconcat("_%s",baseDataSource->requiredDims[i]->value.c_str());
  }
  humanReadableString.replaceSelf(":","_");
  humanReadableString.replaceSelf(".","_");
  
  int returnCode=0;
  FILE *fp=fopen(tempFileName.c_str(), "r");
  if (fp == NULL) {
    CDBError("Invalid File: %s<br>\n", tempFileName.c_str());
    returnCode = 1;
  }else{
    //Send the binary data
    fseek( fp, 0L, SEEK_END );
    size_t endPos = ftell( fp);
    fseek( fp, 0L, SEEK_SET );
    //CDBDebug("File opened: size = %d",endPos);
    
    CDBDebug("Now start streaming %d bytes to the client",endPos);
    printf("Content-Disposition: attachment; filename=%s\r\n",humanReadableString.c_str());
    printf("Content-Description: File Transfer\r\n");
    printf("Content-Transfer-Encoding: binary\r\n");
    printf("Content-Length: %zu\r\n",endPos); 
    printf("%s\r\n\r\n","Content-Type:application/netcdf");
    for(size_t j=0;j<endPos;j++)putchar(getc(fp));
    fclose(fp);
    fclose(stdout);
  }
  //Remove temporary files
  remove(tempFileName.c_str());
  CDBDebug("Done");
  if(returnCode!=0)return 1;
  return status;
  
}

CNetCDFDataWriter::CNetCDFDataWriter(){
  #ifdef CNetCDFDataWriter_DEBUG
        CDBDebug("CNetCDFDataWriter::CNetCDFDataWriter()");
#endif   

  destCDFObject = NULL;
  baseDataSource = NULL;
  projectionDimX=NULL;
  projectionDimY=NULL;
  projectionVarX=NULL;
  projectionVarY=NULL;
}
CNetCDFDataWriter::~CNetCDFDataWriter(){
#ifdef CNetCDFDataWriter_DEBUG
        CDBDebug("CNetCDFDataWriter::~CNetCDFDataWriter()");
#endif   
  delete destCDFObject;destCDFObject = NULL;
}