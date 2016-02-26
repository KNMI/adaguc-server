#include "CNetCDFDataWriter.h"
#include "CGenericDataWarper.h"
const char * CNetCDFDataWriter::className = "CNetCDFDataWriter";

//#define CNetCDFDataWriter_DEBUG

void createProjectionVariables(CDFObject *cdfObject,int width,int height,double *bbox){
  bool isProjected=true;
  CDF::Dimension *projectionDimX = new CDF::Dimension();
  if(isProjected){
    projectionDimX->name="x";
  }else{
    projectionDimX->name="lon";
  }
  projectionDimX->setSize(width);
  cdfObject->addDimension(projectionDimX);
  CDF::Variable *projectionVarX = new CDF::Variable();
  projectionVarX->setType(CDF_DOUBLE);
  projectionVarX->name.copy(projectionDimX->name.c_str());
  projectionVarX->isDimension=true;
  projectionVarX->dimensionlinks.push_back(projectionDimX);
  cdfObject->addVariable(projectionVarX);
  CDF::allocateData(CDF_DOUBLE,&projectionVarX->data,width);
  
  CDF::Dimension *projectionDimY = new CDF::Dimension();
  if(isProjected){
    projectionDimY->name="y";
  }else{
    projectionDimY->name="lat";
  }
  projectionDimY->setSize(height);
  cdfObject->addDimension(projectionDimY);
  CDF::Variable *projectionVarY = new CDF::Variable();
  projectionVarY->setType(CDF_DOUBLE);
  projectionVarY->name.copy(projectionDimY->name.c_str());
  projectionVarY->isDimension=true;
  projectionVarY->dimensionlinks.push_back(projectionDimY);
  cdfObject->addVariable(projectionVarY);
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
  baseDataSource = dataSource;
  this->srvParam = srvParam;
  destCDFObject = new CDFObject();
  CT::string randomString = CServerParams::randomString(32);
  tempFileName.print("%s/%s.nc",srvParam->cfg->TempDir[0]->attr.value.c_str(),randomString.c_str());
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
#ifdef CNetCDFDataWriter_DEBUG        
    CDBDebug("Adding dimension [%s]",dimName.c_str());
#endif    
    CDF::Dimension *dim = new CDF::Dimension();
    dim->name = dimName;
    dim->setSize(1);//TODO
    destCDFObject->addDimension(dim);
    CDF::Variable *var = new CDF::Variable();
    var->setType(CDF_DOUBLE);
    var->name.copy(dim->name.c_str());
    var->isDimension=true;
    var->dimensionlinks.push_back(dim);
    destCDFObject->addVariable(var);
    CDF::allocateData(CDF_DOUBLE,&var->data,dim->length);
    var->setSize(dim->length);
    if(CDF::fill(var->data,var->getType(),0,var->getSize())!=0){
      CDBError("Unable to initialize data field to nodata value");
      return 1;
    }
    CDF::Variable *sourceVar = srcObj->getVariable(dimName.c_str());
    for(size_t i=0;i<sourceVar->attributes.size();i++){
      var->setAttribute(sourceVar->attributes[i]->name.c_str(),sourceVar->attributes[i]->getType(),sourceVar->attributes[i]->data,sourceVar->attributes[i]->length);
    } 
  }
  
  //Create variables
  size_t varSize = 1;
  for(size_t j=0;j<baseDataSource->getNumDataObjects();j++){
    CDF::Variable *destVar = new CDF::Variable();
    CDF::Variable *sourceVar = baseDataSource->getDataObject(j)->cdfVariable;
    destVar->name.copy(sourceVar->name.c_str());
    destVar->setType(sourceVar->getType());
    for(size_t i=0;i<sourceVar->dimensionlinks.size();i++){
      CDF::Dimension *d = destCDFObject->getDimensionNE(sourceVar->dimensionlinks[i]->name.c_str());
      if(d==NULL){
        CDBError("Unable to add dimension %s",sourceVar->dimensionlinks[i]->name.c_str());
        throw(__LINE__);
      }
      destVar->dimensionlinks.push_back(d);
      varSize*=d->getSize();
    }
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
    destCDFObject->addVariable(destVar);
  }
  
  CDF::Variable *crs = new CDF::Variable();
  crs->name="crs";
  crs->setType(CDF_CHAR);
  crs->setSize(0);
  
  
  destCDFObject->addVariable(crs);
  
  return 0;
}

// CDF::Variable *createOrGetVariable(CDFObject *cdfObject,CT::string name){
//   CDF::Variable *variable = cdfObject->getVariableNE(name.c_str());
//   if(variable == NULL){
//     variable = new CDF::Variable();
//     if(name.equals("x")){
//       
//     }
//   }
// }


int CNetCDFDataWriter::addData(std::vector <CDataSource*> &dataSources){
//  CDBDebug("addData");

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
    
    //
    CDF::Variable *variable = destCDFObject->getVariable(dataSource->getDataObject(0)->cdfVariable->name.c_str());;
    
    //Set dimension
    CCDFDims *dims = dataSource->getCDFDims();
    #ifdef CNetCDFDataWriter_DEBUG    
    CDBDebug("Setting dimension,nr %d",dims->getNumDimensions());
#endif
    
    for(size_t d=0;d<dims->getNumDimensions();d++){
      CT::string dimName = dims->getDimensionName(d);
      if(dimName.equals("forecast_reference_time")==false){
#ifdef CNetCDFDataWriter_DEBUG
        CDBDebug("Setting dimension %s for timestep %d",dimName.c_str(),dataSource->getCurrentTimeStep());
#endif        
        //int destIndex = -1;
        CDF::Variable *dim=destCDFObject->getVariable(dimName.c_str());
        double *dimData = ((double*)dim->data);
        CT::string dimValue = dataSource->getDimensionValue(d);
#ifdef CNetCDFDataWriter_DEBUG
        CDBDebug("Setting dimension %s value = %s",dimName.c_str(),dataSource->getDimensionValue(dataSource->getDimensionIndex(dimName.c_str())).c_str());
#endif        
        if(dims->isTimeDimension(d)){
          CTime ctime;
          ctime.init(timeUnits.c_str());
          double offset = ctime.dateToOffset(ctime.ISOStringToDate(dims->getDimensionValue(d).c_str()));
          dimData[0]=offset;
        } 
        
        
      }
    }
    
    
    //Warp
    void *warpedData = variable->data;
    
    
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
    settings.data=warpedData;

    
    GenericDataWarper::render<float> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);/*break;*/
    reader.close();
    
    
    
//    CDBDebug("Addata finished, data warped");
    
//     if(dataSource->statistics==NULL){
//       dataSource->statistics = new CDataSource::Statistics();
//       dataSource->statistics->calculate(dataSource);
//     }
    
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
  destCDFObject = NULL;
  baseDataSource = NULL;
}
CNetCDFDataWriter::~CNetCDFDataWriter(){
  delete destCDFObject;destCDFObject = NULL;
}