#include "CNetCDFDataWriter.h"
#include "CGenericDataWarper.h"
const char * CNetCDFDataWriter::className = "CNetCDFDataWriter";
#include "CRequest.h"
// #define CNetCDFDataWriter_DEBUG

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
  // reader.enableReporting(false); // DO not set to false, functional test will fail if set to false
  
  int status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
  if(status!=0){
    CDBError("Could not open file: %s",dataSource->getFileName());
    return 1;
  }
  

  // Copy global attributes
  CDFObject * srcObj=dataSource->getDataObject(0)->cdfObject;
  for(size_t j=0;j<srcObj->attributes.size();j++){
    destCDFObject->setAttribute(srcObj->attributes[j]->name.c_str(),srcObj->attributes[j]->type,srcObj->attributes[j]->data,srcObj->attributes[j]->length);
  }
  

  
//   //Copy provenance, if available
//   CDF::Variable *KNMIProv = srcObj->getVariableNE("knmi_provenance");
//   if(KNMIProv!=NULL){
//     CDF::Variable *newProv = new CDF::Variable();
//     newProv->name="knmi_provenance";
//     newProv->setType(CDF_CHAR);
//     newProv->setSize(0);
//     destCDFObject->addVariable(newProv);
//     for(size_t j=0;j<KNMIProv->attributes.size();j++){
//       newProv->setAttribute(KNMIProv->attributes[j]->name.c_str(),KNMIProv->attributes[j]->type,KNMIProv->attributes[j]->data,KNMIProv->attributes[j]->length);
//     }
//   }
//   
  
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
  
   //Adjust history
  CT::string historyText = "";
  CDF::Attribute *historyAttr = destCDFObject->getAttributeNE("history");
  if(historyAttr!=NULL){
    historyText = historyAttr->toString();
  }
  
  CT::string adagucwcsdestgrid;
  adagucwcsdestgrid.print("width=%d&height=%d&bbox=%f,%f,%f,%f&crs=%s", 
                       srvParam->Geo->dWidth,
                       srvParam->Geo->dHeight,
                       dfDstBBOX[0],
                       dfDstBBOX[1],
                       dfDstBBOX[2],
                       dfDstBBOX[3],
                       srvParam->Geo->CRS.c_str());
  
  CT::string newHistoryText;
  newHistoryText.print("Created by ADAGUC WCS Server version %s, destination grid settings: %s. %s",
                       ADAGUCSERVER_VERSION,
                       adagucwcsdestgrid.c_str(),
                       historyText.c_str());
  destCDFObject->setAttributeText("history",newHistoryText.c_str());
  
  //Write dest grid attribute
  destCDFObject->setAttributeText("adaguc_wcs_destgridspec",adagucwcsdestgrid.c_str());

  //Write CF Convertion variable
  destCDFObject->setAttributeText("Conventions","CF-1.6");
  
  //Create projection variables
  createProjectionVariables(destCDFObject,srvParam->Geo->dWidth,srvParam->Geo->dHeight,dfDstBBOX);
  
  //Move uuid
  CDF::Attribute *uuid = destCDFObject->getAttributeNE("uuid");
  if(uuid != NULL){
    uuid->name = "input_uuid";
  }
  
  CDF::Attribute *tracking_id = destCDFObject->getAttributeNE("tracking_id");
  if(tracking_id != NULL){
    tracking_id->name = "invar_tracking_id";
  }
  
  // Now it will work also on gridded polygondata
  destCDFObject->removeAttribute("featureType");
  
  // Remove attributes which have no meaning anymore
  destCDFObject->removeAttribute("geospatial_increment");
  destCDFObject->removeAttribute("geospatial_lat_max");
  destCDFObject->removeAttribute("geospatial_lat_min");
  destCDFObject->removeAttribute("geospatial_lon_max ");
  
  destCDFObject->removeAttribute("geospatial_lon_min");
  destCDFObject->removeAttribute("domain");
  CT::string software;software.print("ADAGUC WCS Server version %s",ADAGUCSERVER_VERSION);
  destCDFObject->setAttributeText("software",software.c_str());
  destCDFObject->removeAttribute("software_platform");
  destCDFObject->removeAttribute("time_coverage_end");
  destCDFObject->removeAttribute("time_coverage_start");
  destCDFObject->removeAttribute("time_number_gaps");
  destCDFObject->removeAttribute("time_number_steps");
  
  
  //Create other NonGeo dimensions
  //CCDFDims *dims = baseDataSource->getCDFDims();
#ifdef CNetCDFDataWriter_DEBUG    
  CDBDebug("Number of requireddims=%d",baseDataSource->requiredDims.size());
#endif
  for(size_t d=0;d<baseDataSource->requiredDims.size();d++){
    
    CT::string dimName = "null";

    dimName = baseDataSource->requiredDims[d]->netCDFDimName;
    if(dimName.equals("none")==true){
      break;
    }
    CDataReader::DimensionType dtype = CDataReader::getDimensionType(srcObj,dimName.c_str());
    if(dtype==CDataReader::dtype_none){
      CDBWarning("dtype_none for %s",dtype,dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
    }

    
    bool isTimeDim = false;
    if(dtype == CDataReader::dtype_time || dtype == CDataReader::dtype_reference_time){
      isTimeDim = true;
    }
    
    
    CDF::Variable *sourceVar = srcObj->getVariableNE(dimName.c_str());
    if(sourceVar == NULL){
      CDBError("Unable to find dimension [%s]",dimName.c_str());
      throw(__LINE__);
    }
    
    
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

    for(size_t i=0;i<sourceVar->attributes.size();i++){
      var->setAttribute(sourceVar->attributes[i]->name.c_str(),sourceVar->attributes[i]->getType(),sourceVar->attributes[i]->data,sourceVar->attributes[i]->length);
    } 
    
    dim->setSize(baseDataSource->requiredDims[d]->uniqueValues.size());
#ifdef CNetCDFDataWriter_DEBUG        
    CDBDebug("Adding dimension [%s] with type [%d] and length [%d]",dimName.c_str(),var->getType(),dim->length);
#endif    
    if(dim->length == 0){
      CDBError("Cannot create dimension [%s] with length zero",dimName.c_str());
      return 1;
    }
    
    var->setSize(dim->length);
    CDF::allocateData(var->getType(),&var->data,dim->length);
    
    if(CDF::fill(var->data,var->getType(),0,var->getSize())!=0){
      CDBError("Unable to initialize data field to nodata value");
      return 1;
    }
    
   
    //Fill dimension with correct data
    for(size_t j=0;j<baseDataSource->requiredDims[d]->uniqueValues.size();j++){
#ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("START");
#endif 
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
      //CDBDebug("dimValue.c_str() = %s",dimValue.c_str());
      if(var->getType()!=CDF_STRING){
        if(isTimeDim){
          CTime ctime;
          ctime.init(CDataReader::getTimeDimension(dataSource));
          //CDBDebug("dimValue.c_str() = %s",dimValue.c_str());
          double offset = ctime.dateToOffset(ctime.freeDateStringToDate(dimValue.c_str()));
#ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("Dimension [%s]: writing value %s with offset %f to index %d",dimName.c_str(),dimValue.c_str(),offset,j);
#endif       
          double *dimData = ((double*)var->data);
          dimData[j]=offset;
        } else{
    #ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("Dimension [%s]: writing scalar value %s to index %d for variable %s",dimName.c_str(),dimValue.c_str(),j,var->name.c_str());
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
      
#ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("DONE");
#endif 
    }
  
    
//     //Copy dim attributes
//     for(size_t i=0;i<sourceVar->attributes.size();i++){
//       var->setAttribute(sourceVar->attributes[i]->name.c_str(),sourceVar->attributes[i]->getType(),sourceVar->attributes[i]->data,sourceVar->attributes[i]->length);
//     } 
  
  }
  
  #ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("CREATE VARIABLES");
#endif 
  //Create variables
  
  for(size_t j=0;j<baseDataSource->getNumDataObjects();j++){
#ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("INIT START %d/%d",j,baseDataSource->getNumDataObjects());
#endif 
    CDF::Variable *destVar = new CDF::Variable();
    destCDFObject->addVariable(destVar);
    CDF::Variable *sourceVar = baseDataSource->getDataObject(j)->cdfVariable;
    destVar->name.copy(sourceVar->name.c_str());
    
#ifdef CNetCDFDataWriter_DEBUG            
    CDBDebug("Name = %s, type = %d",sourceVar->name.c_str(),sourceVar->getType());
#endif 
    
    destVar->setType(sourceVar->getType());
    size_t varSize = 1;
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
      
      #ifdef CNetCDFDataWriter_DEBUG            
        CDBDebug("Using dimension %s with size %d",d->name.c_str(),d->getSize());
      #endif 
    
    
      varSize*=d->getSize();
      
    }
    
#ifdef CNetCDFDataWriter_DEBUG            
    //CDBDebug("Name = %s, type = %d",sourceVar->name.c_str(),sourceVar->getType());
    CDBDebug("Allocating %d elements for variable %s",varSize/(projectionDimX->getSize()*projectionDimY->getSize()),destVar->name.c_str());
#endif 
        
    
    if(CDF::allocateData(destVar->getType(),&destVar->data,varSize)!=0){
      CDBError("Unable to allocate data for variable %s with %d elements",destVar->name.c_str(),varSize);
      return 1;
    }
    double dfNoData = NAN;
    if(dataSource->getDataObject(j)->hasNodataValue==1){
      dfNoData=dataSource->getDataObject(j)->dfNodataValue;
    }
    
#ifdef CNetCDFDataWriter_DEBUG            
    CDBDebug("Filling variable data of size %d",varSize);
#endif 
    
    if(CDF::fill(destVar->data,destVar->getType(),dfNoData,varSize)!=0){
      CDBError("Unable to initialize data field to nodata value");
      return 1;
    }
    
#ifdef CNetCDFDataWriter_DEBUG            
    CDBDebug("Setting attributes");
#endif 
    for(size_t i=0;i<sourceVar->attributes.size();i++){
      //CDBDebug("For %s: Copying attribute %s with length %d",destVar->name.c_str(),sourceVar->attributes[i]->name.c_str(),sourceVar->attributes[i]->length);
      if(!sourceVar->attributes[i]->name.equals("scale_factor")&&
        !sourceVar->attributes[i]->name.equals("add_offset")&&
        !sourceVar->attributes[i]->name.equals("_FillValue")){
        destVar->setAttribute(sourceVar->attributes[i]->name.c_str(),sourceVar->attributes[i]->getType(),sourceVar->attributes[i]->data,sourceVar->attributes[i]->length);
      }
    }
  
    
    //destVar->removeAttribute("calendar");
    //destCDFObject->setAttribute("_FillValue",destVar->getType(),&dfNoData,1);
    
    //destCDFObject->removeAttribute("grid_mapping");
    destVar->setAttributeText("grid_mapping","crs");
   
  }
  
  #ifdef CNetCDFDataWriter_DEBUG            
          CDBDebug("DONE");
#endif 
  
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
  #ifdef CNetCDFDataWriter_DEBUG     
  CDBDebug("Add data");
  #endif  
  int status;
  bool verbose = false;
  for(size_t i=0;i<dataSources.size();i++){
    CDataSource *dataSource = dataSources[i];
    CDataReader reader;
    reader.enableReporting(verbose);
//     render
  
     if(verbose){
       CDBDebug("Reading file %s",dataSource->getFileName());
     }
     status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
   
     //CDBDebug("Initializing warper for file %s",dataSource->getFileName());
       
    CImageWarper warper;
    dataSource->srvParams=this->srvParam;
    status = warper.initreproj(dataSource,this->srvParam->Geo,&srvParam->cfg->Projection);
    if(status != 0){
      CDBError("Unable to initialize projection");
      return 1;
    }
            CGeoParams sourceGeo;
      
      
      bool usePixelExtent = false;
      bool optimizeExtentForTiles = false;
      if(dataSource->cfgLayer->TileSettings.size() == 1 && !dataSource->cfgLayer->TileSettings[0]->attr.optimizeextent.empty()){
        if(dataSource->cfgLayer->TileSettings[0]->attr.optimizeextent.equals("true")){
          optimizeExtentForTiles=true;
        }
      }
      
      if(optimizeExtentForTiles){
        usePixelExtent = true;
      }
      
      if(usePixelExtent ){
        sourceGeo.dWidth = dataSource->dWidth;
        sourceGeo.dHeight = dataSource->dHeight;
        sourceGeo.dfBBOX[0] = dataSource->dfBBOX[0];
        sourceGeo.dfBBOX[1] = dataSource->dfBBOX[1];
        sourceGeo.dfBBOX[2] = dataSource->dfBBOX[2];
        sourceGeo.dfBBOX[3] = dataSource->dfBBOX[3];
        sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
        sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
        sourceGeo.CRS = dataSource->nativeProj4;
      
        int PXExtentBasedOnSource[4];
        PXExtentBasedOnSource[0]=0;
        PXExtentBasedOnSource[1]=0;
        PXExtentBasedOnSource[2]=dataSource->dWidth;;
        PXExtentBasedOnSource[3]=dataSource->dHeight;;
        GenericDataWarper::findPixelExtent(PXExtentBasedOnSource,&sourceGeo,this->srvParam->Geo,&warper);
        
        
        
        if(PXExtentBasedOnSource[0] == PXExtentBasedOnSource[2] || PXExtentBasedOnSource[1] == PXExtentBasedOnSource[3])   {
          //CDBDebug("PXExtentBasedOnSource = [%d,%d,%d,%d]",PXExtentBasedOnSource[0],PXExtentBasedOnSource[1],PXExtentBasedOnSource[2],PXExtentBasedOnSource[3]);  
          return 1;
        }
        
        status = reader.openExtent(dataSource,CNETCDFREADER_MODE_OPEN_EXTENT,PXExtentBasedOnSource);
        
        /* Check if there is any data in the found grid */
        if(dataSource->statistics == NULL){
          dataSource->statistics = new CDataSource::Statistics();
          dataSource->statistics->calculate(dataSource);
        }
        if(dataSource->statistics!=NULL){
          if(verbose){
            CDBDebug("min %f, max %f samples %d",dataSource->statistics->getMinimum(),dataSource->statistics->getMaximum(), dataSource->statistics->getNumSamples());
          }
          if(dataSource->statistics->getNumSamples() ==0)return 1;
        }
      }else{
        status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL);
      }
      sourceGeo.dWidth = dataSource->dWidth;
      sourceGeo.dHeight = dataSource->dHeight;
      sourceGeo.dfBBOX[0] = dataSource->dfBBOX[0];
      sourceGeo.dfBBOX[1] = dataSource->dfBBOX[1];
      sourceGeo.dfBBOX[2] = dataSource->dfBBOX[2];
      sourceGeo.dfBBOX[3] = dataSource->dfBBOX[3];
      sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
      sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
      sourceGeo.CRS = dataSource->nativeProj4;
    if(status!=0){
      CDBError("Could not open file: %s",dataSource->getFileName());
      return 1;
    }

    
    
    
    for(size_t j=0;j<baseDataSource->getNumDataObjects();j++){
  #ifdef CNetCDFDataWriter_DEBUG            
            CDBDebug("START %d/%d",j,baseDataSource->getNumDataObjects());
  #endif 

      
      
      CDF::Variable *variable = destCDFObject->getVariable(dataSource->getDataObject(j)->cdfVariable->name.c_str());;
      
      //Set dimension
      CCDFDims *dims = dataSource->getCDFDims();
      #ifdef CNetCDFDataWriter_DEBUG    
      CDBDebug("Setting nrof dimensions %d",dims->getNumDimensions());
  #endif
      //CDBDebug("getCurrentTimeStep %d",dataSource->getCurrentTimeStep());
      if(dims->getNumDimensions() == 0){
        CDBDebug("Note: This datasource [%d] has no dimensions",i);
      }
      
      /*
      * This step figures out the required dimindex for each dimension based on timestep.
      */
      int dimIndices[dims->getNumDimensions()+1];
      
      //CDBDebug("baseDataSource->requiredDims.size(); = %d",baseDataSource->requiredDims.size());

      
      for(size_t d=0;d<baseDataSource->requiredDims.size();d++){
        dimIndices[d]=0;
        CT::string dimName = baseDataSource->requiredDims[d]->netCDFDimName;
        if(dimName.equals("none")==true){
          break;
        }
        CDataReader::DimensionType dtype = CDataReader::getDimensionType(dataSource->getDataObject(j)->cdfObject,dimName.c_str());
        if(dtype==CDataReader::dtype_none){
          CDBWarning("dtype_none for %s",dtype,dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
        }

        
        bool isTimeDim = false;
        if(dtype == CDataReader::dtype_time || dtype == CDataReader::dtype_reference_time){
          isTimeDim = true;
        }
        CT::string dimValue = dataSource->getDimensionValueForNameAndStep(dimName.c_str(),dataSource->getCurrentTimeStep());
        int indexTofind = -1;
        
        //CDBDebug("dimName.c_str() %s dimValue  = %s  step %d",dimName.c_str(),dimValue.c_str(),dataSource->getCurrentTimeStep());
        
        
        CDF::Variable *var=destCDFObject->getVariable(dimName.c_str());
        //CDBDebug("trying to search in var with size %d",var->getSize());
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
            //CDBDebug("isTimeDim");
            CTime ctime;
            ctime.init(CDataReader::getTimeDimension(dataSource));
            //CDBDebug("Trying to convert string %s",dimValue.c_str());
            double offset = ctime.dateToOffset(ctime.freeDateStringToDate(dimValue.c_str()));
            //CDBDebug("offset = %f",offset);
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
  
      
      destCDFObject->getVariable("crs")->setAttributeText("proj4_params",warper.getDestProjString().c_str());

      
      void *sourceData = dataSource->getDataObject(j)->cdfVariable->data;
      
      Settings settings;
      settings.width = srvParam->Geo->dWidth;
      settings.height = srvParam->Geo->dHeight;
      
      settings.rField = NULL;
      settings.gField=NULL;
      settings.bField = NULL;
      settings.numField = NULL;
      settings.trueColorRGBA=(drawFunctionMode == CNetCDFDataWriter_AVG_RGB); 
      
      if(settings.trueColorRGBA){
        size_t size = settings.width*settings.height;
        
        settings.rField = new float[size];
        settings.gField = new float[size];
        settings.bField = new float[size];
        settings.numField = new int[size];
        for(size_t j=0;j<size;j++){
          settings.rField[j] = 0;
          settings.gField[j] = 0;
          settings.bField[j] = 0;
          settings.numField[j] = 0;
        }
      }
      
//      CDBDebug("Setting pointers");
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
      
  
     
      
      if(verbose){
        CDBDebug("Warping from %dx%d to %dx%d", sourceGeo.dWidth, sourceGeo.dHeight, settings.width, settings.height);
      }

      if(drawFunctionMode == CNetCDFDataWriter_NEAREST){
        switch(variable->getType()){
          case CDF_CHAR  :  GenericDataWarper::render<char>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
          case CDF_BYTE  :  GenericDataWarper::render<char>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
          case CDF_UBYTE :  GenericDataWarper::render<unsigned char> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
          case CDF_SHORT :  GenericDataWarper::render<short> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
          case CDF_USHORT:  GenericDataWarper::render<ushort>(&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
          case CDF_INT   :  GenericDataWarper::render<int>   (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
          case CDF_UINT  :  GenericDataWarper::render<uint>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
          case CDF_FLOAT :  GenericDataWarper::render<float> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
          case CDF_DOUBLE:  GenericDataWarper::render<double>(&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_nearest);break;
        }
      }
      
      if(drawFunctionMode == CNetCDFDataWriter_AVG_RGB){
        switch(variable->getType()){
          case CDF_CHAR  :  GenericDataWarper::render<char>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
          case CDF_BYTE  :  GenericDataWarper::render<char>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
          case CDF_UBYTE :  GenericDataWarper::render<unsigned char> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
          case CDF_SHORT :  GenericDataWarper::render<short> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
          case CDF_USHORT:  GenericDataWarper::render<ushort>(&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
          case CDF_INT   :  GenericDataWarper::render<int>   (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
          case CDF_UINT  :  GenericDataWarper::render<uint>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
          case CDF_FLOAT :  GenericDataWarper::render<float> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
          case CDF_DOUBLE:  GenericDataWarper::render<double>(&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction_avg_rbg);break;
        }
      }
        
      
      reader.close();
      
            
      
      if(settings.trueColorRGBA){
        delete[] settings.rField;
        delete[] settings.gField;
        delete[] settings.bField;
        delete[] settings.numField;
        settings.rField = NULL;
        settings.gField=NULL;
        settings.bField = NULL;
        settings.numField = NULL;
      }
      //Set _FillValue
      if(dataSource->getDataObject(j)->hasNodataValue){
        if(variable->getAttributeNE("_FillValue")==NULL){
          variable->setAttribute("_FillValue",variable->getType(),dataSource->getDataObject(j)->dfNodataValue);
        }
      }
      
      //Copy feature paramlist
      if (dataSource->getDataObject(j)->features.empty() == false){
        CT::string paramListAttr = "";
        CT::string featureVarName = variable->name.c_str();
        CT::string featureDimIndexName = featureVarName+"_index";
        CDBDebug("featureDimIndexName = %s",featureDimIndexName.c_str());
         CDF::Dimension *featureIndexDim = destCDFObject->getDimensionNE(featureDimIndexName.c_str());
         CDF::Variable *featureIndexVar = NULL;
          if(featureIndexDim == NULL){
            featureIndexDim = new CDF::Dimension();
            featureIndexDim->setSize(dataSource->getDataObject(j)->features.size());
            featureIndexDim->name = featureDimIndexName.c_str();;
            destCDFObject->addDimension(featureIndexDim);
            featureIndexVar = new CDF::Variable();
            featureIndexVar->name = featureDimIndexName.c_str();
            featureIndexVar->setType(CDF_UINT);
            featureIndexVar->setAttributeText("long_name","feature index number");
            featureIndexVar->setAttributeText("auxiliary",variable->name.c_str());
            featureIndexVar->dimensionlinks.push_back(featureIndexDim);
            CDF::allocateData(CDF_DOUBLE,&featureIndexVar->data,featureIndexDim->getSize());

            destCDFObject->addVariable(featureIndexVar);
          }
          featureIndexVar = destCDFObject->getVariable(featureDimIndexName.c_str());
          int featureIndex=0;
          std::map<int,CFeature>::iterator featureIt;
          for (featureIt=dataSource->getDataObject(j)->features.begin(); featureIt!=dataSource->getDataObject(j)->features.end(); ++featureIt){
            CFeature *feature = &featureIt->second;
            if(featureIndex!=featureIt->first){
              CDBError("featureIndex!=featureIt->first: [%d,%d]",featureIndex,featureIt->first);
              return 1;
            }
            if(feature->paramMap.empty() == false){
              std::map<std::string,std::string>::iterator paramItemIt  ;
              for (paramItemIt=feature->paramMap.begin(); paramItemIt!=feature->paramMap.end(); ++paramItemIt){
                CT::string newfeatureVarName= variable->name.c_str();
                newfeatureVarName.printconcat("_%s",paramItemIt->first.c_str());
                CDF::Variable *featureVar = destCDFObject->getVariableNE(newfeatureVarName.c_str());
                if(featureVar == NULL){
                  if(paramListAttr.length()>0){
                    paramListAttr.concat(",");
                  }
                  paramListAttr.concat(newfeatureVarName);
                  featureVar = new CDF::Variable();
                  featureVar->name = newfeatureVarName.c_str();
                  featureVar->setType(CDF_STRING);
                  featureVar->dimensionlinks.push_back(featureIndexDim);
                  CDF::allocateData(CDF_STRING,&featureVar->data,featureIndexDim->getSize());
                  destCDFObject->addVariable(featureVar);
                  featureVar->setAttributeText("long_name",paramItemIt->first.c_str());
                  featureVar->setAttributeText("auxiliary",variable->name.c_str());
                }
                char *str = strdup(paramItemIt->second.c_str());
                ((char**)featureVar->data)[featureIndex]=str;
              // CDBDebug("Clicked %s %s",paramItemIt->first.c_str(),paramItemIt->second.c_str());
              }
            }
            ((int*)featureIndexVar->data)[featureIndex]=featureIndex;
            featureIndex++;
          }
          variable->setAttributeText("paramlist",paramListAttr.c_str());
      }
      
    }
  }
  
  #ifdef CNetCDFDataWriter_DEBUG     
  CDBDebug("Add data done");
  #endif  
  
  return 0;
  
}


int CNetCDFDataWriter::writeFile(const char *fileName,int adaguctilelevel, bool enableCompression){
  if(adaguctilelevel!=-1){
    destCDFObject->setAttribute("adaguctilelevel",CDF_INT,&adaguctilelevel,1);
  }
  CDFNetCDFWriter *netCDFWriter = new CDFNetCDFWriter(destCDFObject);
  netCDFWriter->setNetCDFMode(4);
  if(enableCompression){
    netCDFWriter->setDeflateShuffle(1,2, 0);
  }
  int  status = netCDFWriter->write(fileName);
  delete netCDFWriter;
  
  if(status!=0){
    CDBError("Unable to write file to temporary directory");
    return 1;
  }
  return 0;
}

int CNetCDFDataWriter::end(){
  
    #ifdef CNetCDFDataWriter_DEBUG
      CDBDebug("CNetCDFDataWriter::end()");
    #endif
      
  const char * pszADAGUCWriteToFile=getenv("ADAGUC_WRITETOFILE");      
  if(pszADAGUCWriteToFile != NULL){
    CDFNetCDFWriter *netCDFWriter = new CDFNetCDFWriter(destCDFObject);
    netCDFWriter->setNetCDFMode(4);
    // netCDFWriter->setDeflateShuffle(1,2,0);
    CDBDebug("Write to ADAGUC_WRITETOFILE %s",pszADAGUCWriteToFile);
    int  status = netCDFWriter->write(pszADAGUCWriteToFile);
    if(status!=0){
    CDBError("Unable to write file to file specified in ADAGUC_WRITETOFILE");
      return 1;
    }
    delete netCDFWriter;
    return 0;
  }
  CDFNetCDFWriter *netCDFWriter = new CDFNetCDFWriter(destCDFObject);
 
  netCDFWriter->setNetCDFMode(4);
  netCDFWriter->setDeflateShuffle(1,2,0);
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
  humanReadableString.concat(".nc");
  
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
  //Remove temporary file
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
  drawFunctionMode = CNetCDFDataWriter_NEAREST;
}
CNetCDFDataWriter::~CNetCDFDataWriter(){
#ifdef CNetCDFDataWriter_DEBUG
        CDBDebug("CNetCDFDataWriter::~CNetCDFDataWriter()");
#endif   
  delete destCDFObject;destCDFObject = NULL;
}

void CNetCDFDataWriter::setInterpolationMode(int mode){
  drawFunctionMode = mode;
}
