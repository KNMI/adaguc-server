#include "CConvertADAGUCPoint.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"
//#define CCONVERTADAGUCPOINT_DEBUG
const char *CConvertADAGUCPoint::className="CConvertADAGUCPoint";


/**
 * Class which holds min and max values.
 * isSet indicates whether the values have been set or not.
 */
class MinMax{
public:
  MinMax(){
    isSet = false;
  }
  bool isSet;
  double min,max;
};


/**
 * Returns minmax values for a data array
 * throws integer if no min max are found
 * @param data The data array in double format
 * @param hasFillValue Is there a nodata value
 * @param fillValue the Nodata value
 * @param numElements The length of the data array
 * @return minmax object
 */
MinMax getMinMax(float *data, bool hasFillValue, double fillValue,size_t numElements){
  MinMax minMax;
  bool firstSet = false;
  for(size_t j=0;j<numElements;j++){
    float v=data[j];
    if(v==v){
      if(v!=fillValue||(!hasFillValue)){
        if(firstSet == false){
          firstSet = true;
          minMax.min=v;
          minMax.max=v;
          minMax.isSet=true;
        }
        if(v<minMax.min)minMax.min=v;
        if(v>minMax.max)minMax.max=v;
      }
    }
  }
  if(minMax.isSet==false){
    throw __LINE__+100;
  }
  return minMax;
}      


/**
 * Returns minmax values for a variable
 * @param var The variable to retrieve the min max for.
 * @return minmax object
 */
MinMax getMinMax(CDF::Variable *var){
  MinMax minMax;
  if(var!=NULL){
 
      float *data = (float*)var->data;
      
      float scaleFactor=1,addOffset=0,fillValue = 0;
      bool hasFillValue = false;
      
      try{
        var->getAttribute("scale_factor")->getData(&scaleFactor,1);
      }catch(int e){}
      try{
        var->getAttribute("add_offset")->getData(&addOffset,1);
      }catch(int e){}
      try{
        var->getAttribute("_FillValue")->getData(&fillValue,1);
        hasFillValue = true;
      }catch(int e){}
      
      size_t lsize= var->getSize();
      
      //Apply scale and offset
      if(scaleFactor!=1||addOffset!=0){
        for(size_t j=0;j<lsize;j++){
          data[j]=data[j]*scaleFactor+addOffset;
        }
        fillValue=fillValue*scaleFactor+addOffset;
      }
      

      minMax = getMinMax(data,hasFillValue,fillValue,lsize);
    
  }else{
    //CDBError("getMinMax: Variable has not been set");
    throw __LINE__+100;
  }
  return minMax;
}




/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertADAGUCPoint::convertADAGUCPointHeader( CDFObject *cdfObject ){
  //Check whether this is really an adaguc file
  try{
    if(cdfObject->getAttribute("featureType")->toString().equals("timeSeries")==false)return 1;
  }catch(int e){
    return 1;
  }
  //CDBDebug("THIS IS ADAGUC POINT DATA");

  
 
  //Standard bounding box of adaguc data is worldwide
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;
  
  try{
    pointLon = cdfObject->getVariable("lon");
    pointLat = cdfObject->getVariable("lat");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }
  pointLon->readData(CDF_FLOAT,true);
  pointLat->readData(CDF_FLOAT,true);
  MinMax lonMinMax = getMinMax(pointLon);
  MinMax latMinMax = getMinMax(pointLat);
  
  double dfBBOX[]={lonMinMax.min,latMinMax.min,lonMinMax.max,latMinMax.max};
  //double dfBBOX[]={-180,-90,180,90};
  //CDBDebug("Datasource dfBBOX:%f %f %f %f",dfBBOX[0],dfBBOX[1],dfBBOX[2],dfBBOX[3]);
  
  //Default size of adaguc 2dField is 2x2
  int width=2;
  int height=2;
  
  double cellSizeX=(dfBBOX[2]-dfBBOX[0])/double(width);
  double cellSizeY=(dfBBOX[3]-dfBBOX[1])/double(height);
  double offsetX=dfBBOX[0];
  double offsetY=dfBBOX[1];
  
  //Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("x");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("y");
  CDF::Dimension *dimT = cdfObject->getDimensionNE("time");
  
  if(dimT==NULL){
    CDBError("No time dimension found");
    return 1;
  }
  
  CDF::Variable *varX = cdfObject->getVariableNE("x");
  CDF::Variable *varY = cdfObject->getVariableNE("y");
  if(dimX==NULL||dimY==NULL||varX==NULL||varY==NULL) {
    //If not available, create new dimensions and variables (X,Y,T)
    //For x 
    dimX=new CDF::Dimension();
    dimX->name="x";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->type=CDF_DOUBLE;
    varX->name.copy("x");
    varX->isDimension=true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE,&varX->data,dimX->length);

    //For y 
    dimY=new CDF::Dimension();
    dimY->name="y";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->type=CDF_DOUBLE;
    varY->name.copy("y");
    varY->isDimension=true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE,&varY->data,dimY->length);
    
    //Fill in the X and Y dimensions with the array of coordinates
    for(size_t j=0;j<dimX->length;j++){
      double x=offsetX+double(j)*cellSizeX+cellSizeX/2;
      ((double*)varX->data)[j]=x;
    }
    for(size_t j=0;j<dimY->length;j++){
      double y=offsetY+double(j)*cellSizeY+cellSizeY/2;
      ((double*)varY->data)[j]=y;
    }
  }
  
  //Make a list of variables which will be available as 2D fields  
  CT::StackList<CT::string> varsToConvert;
  for(size_t v=0;v<cdfObject->variables.size();v++){
    CDF::Variable *var = cdfObject->variables[v];
    if(var->isDimension==false){
      if(!var->name.equals("time2D")&&
        !var->name.equals("time")&&
        !var->name.equals("lon")&&
        !var->name.equals("lat")&&
        !var->name.equals("lat_bnds")&&
        !var->name.equals("lon_bnds")&&
        !var->name.equals("custom")&&
        !var->name.equals("projection")&&
        !var->name.equals("product")&&
        !var->name.equals("iso_dataset")&&
        !var->name.equals("tile_properties")
      ){
        varsToConvert.add(CT::string(var->name.c_str()));
      }
      if(var->name.equals("projection")){
        var->setAttributeText("ADAGUC_SKIP","true");
      }
    }
  }
  
  //Create the new 2D field variables based on the swath variables
  for(size_t v=0;v<varsToConvert.size();v++){
    CDF::Variable *pointVar=cdfObject->getVariable(varsToConvert[v].c_str());
    
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("Converting %s",pointVar->name.c_str());
    #endif
    
    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);
    
    //Assign X,Y,T dims 
    new2DVar->dimensionlinks.push_back(dimT);
    new2DVar->dimensionlinks.push_back(dimY);
    new2DVar->dimensionlinks.push_back(dimX);
    
    new2DVar->type=pointVar->type;
    new2DVar->name=pointVar->name.c_str();
    pointVar->name.concat("_backup");
    
    //Copy variable attributes
    for(size_t j=0;j<pointVar->attributes.size();j++){
      CDF::Attribute *a =pointVar->attributes[j];
      new2DVar->setAttribute(a->name.c_str(),a->type,a->data,a->length);
      new2DVar->setAttributeText("ADAGUC_POINT","true");
    }
    
    //The swath variable is not directly plotable, so skip it
    pointVar->setAttributeText("ADAGUC_SKIP","true");
    
    //Scale and offset are already applied
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");
    
    new2DVar->type=CDF_FLOAT;
  }
 
  return 0;
}




/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertADAGUCPoint::convertADAGUCPointData(CDataSource *dataSource,int mode){
  #ifdef CCONVERTADAGUCPOINT_DEBUG
  CDBDebug("convertADAGUCPointData");
  #endif
  CDFObject *cdfObject0 = dataSource->dataObject[0]->cdfObject;
  try{
    if(cdfObject0->getAttribute("featureType")->toString().equals("timeSeries")==false)return 1;
  }catch(int e){
    return 1;
  }
  //CDBDebug("THIS IS ADAGUC POINT DATA");
  
  
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;
  
  try{
    pointLon = cdfObject0->getVariable("lon");
    pointLat = cdfObject0->getVariable("lat");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }
  

  size_t nrDataObjects = dataSource->dataObject.size();
  CDF::Variable *new2DVar[nrDataObjects];
  CDF::Variable *pointVar[nrDataObjects];
  
  
  for(size_t d=0;d<nrDataObjects;d++){
    new2DVar[d] = dataSource->dataObject[d]->cdfVariable;
    CT::string origSwathName=new2DVar[d]->name.c_str();
    origSwathName.concat("_backup");
    pointVar[d]=dataSource->dataObject[d]->cdfObject->getVariableNE(origSwathName.c_str());
    if(pointVar[d]==NULL){
      CDBError("Unable to find orignal swath variable with name %s",origSwathName.c_str());
      return 1;
    }
    
  }
  
  //Read original data first 
  
  pointLon->readData(CDF_FLOAT,true);
  pointLat->readData(CDF_FLOAT,true);
  
  for(size_t d=0;d<nrDataObjects;d++){
    pointVar[d]->readData(CDF_FLOAT,true);
  }
  
  
  for(size_t d=0;d<nrDataObjects;d++){
    CDF::Attribute *fillValue = pointVar[d]->getAttributeNE("_FillValue");
    if(fillValue!=NULL){
      dataSource->dataObject[d]->hasNodataValue=true;
      fillValue->getData(&dataSource->dataObject[d]->dfNodataValue,1);
      #ifdef CCONVERTADAGUCPOINT_DEBUG
      CDBDebug("_FillValue = %f",dataSource->dataObject[d]->dfNodataValue);
      #endif
      float f=dataSource->dataObject[d]->dfNodataValue;
      new2DVar[d]->getAttribute("_FillValue")->setData(CDF_FLOAT,&f,1);
    }else {
      dataSource->dataObject[d]->hasNodataValue=true;
      dataSource->dataObject[d]->dfNodataValue=-9999999;
      float f=dataSource->dataObject[d]->dfNodataValue;
      new2DVar[d]->setAttribute("_FillValue",CDF_FLOAT,&f,1);
    }
  }
  
  //Detect minimum and maximum values
  float fill = (float)dataSource->dataObject[0]->dfNodataValue;
  float min = fill;float max=fill;
  for(size_t j=0;j<pointVar[0]->getSize();j++){
    float v=((float*)pointVar[0]->data)[j];
    if(v!=fill){
      if(min==fill)min=v;
      if(max==fill)max=v;
      if(v<min)min=v;
      if(v>max)max=v;
    }
  }
  
  #ifdef CCONVERTADAGUCPOINT_DEBUG
  CDBDebug("Calculated min/max : %f %f",min,max);
  #endif
  
  //Set statistics
  if(dataSource->stretchMinMax){
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("dataSource->stretchMinMax");
    #endif
    if(dataSource->statistics==NULL){
      #ifdef CCONVERTADAGUCPOINT_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f",min,max);
      #endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(max);
      dataSource->statistics->setMinimum(min);
    }
  }
  
  //Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth=dataSource->srvParams->Geo->dWidth;
  dataSource->dHeight=dataSource->srvParams->Geo->dHeight;      
  
  if(dataSource->dWidth == 1 && dataSource->dHeight == 1){
    dataSource->srvParams->Geo->dfBBOX[0]=dataSource->srvParams->Geo->dfBBOX[0];
    dataSource->srvParams->Geo->dfBBOX[1]=dataSource->srvParams->Geo->dfBBOX[1];
    dataSource->srvParams->Geo->dfBBOX[2]=dataSource->srvParams->Geo->dfBBOX[2];
    dataSource->srvParams->Geo->dfBBOX[3]=dataSource->srvParams->Geo->dfBBOX[3];
  }
  
  //Width needs to be at least 2 in this case.
  if(dataSource->dWidth == 1)dataSource->dWidth=2;
  if(dataSource->dHeight == 1)dataSource->dHeight=2;
  double cellSizeX=(dataSource->srvParams->Geo->dfBBOX[2]-dataSource->srvParams->Geo->dfBBOX[0])/double(dataSource->dWidth);
  double cellSizeY=(dataSource->srvParams->Geo->dfBBOX[3]-dataSource->srvParams->Geo->dfBBOX[1])/double(dataSource->dHeight);
  double offsetX=dataSource->srvParams->Geo->dfBBOX[0];
  double offsetY=dataSource->srvParams->Geo->dfBBOX[1];
 
 
  

  
  
  

  if(mode==CNETCDFREADER_MODE_OPEN_ALL){
   
    
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    for(size_t d=0;d<nrDataObjects;d++){
      CDBDebug("Drawing %s",new2DVar[d]->name.c_str());
    }
    #endif
    
    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX ;
    CDF::Variable *varY;
  
    //Create new dimensions and variables (X,Y,T)
    dimX=cdfObject0->getDimension("x");
    dimX->setSize(dataSource->dWidth);
    
    dimY=cdfObject0->getDimension("y");
    dimY->setSize(dataSource->dHeight);
    
    varX = cdfObject0->getVariable("x");
    varY = cdfObject0->getVariable("y");
    
    CDF::allocateData(CDF_DOUBLE,&varX->data,dimX->length);
    CDF::allocateData(CDF_DOUBLE,&varY->data,dimY->length);
    
    //Fill in the X and Y dimensions with the array of coordinates
    for(size_t j=0;j<dimX->length;j++){
      double x=offsetX+double(j)*cellSizeX+cellSizeX/2;
      ((double*)varX->data)[j]=x;
    }
    for(size_t j=0;j<dimY->length;j++){
      double y=offsetY+double(j)*cellSizeY+cellSizeY/2;
      ((double*)varY->data)[j]=y;
    }
    
    
    
    //Allocate 2D field
    for(size_t d=0;d<nrDataObjects;d++){
      size_t fieldSize = dataSource->dWidth*dataSource->dHeight;
      new2DVar[d]->setSize(fieldSize);
      CDF::allocateData(new2DVar[d]->type,&(new2DVar[d]->data),fieldSize);
      
      //Fill in nodata
      if(dataSource->dataObject[d]->hasNodataValue){
        for(size_t j=0;j<fieldSize;j++){
          ((float*)dataSource->dataObject[d]->cdfVariable->data)[j]=(float)dataSource->dataObject[d]->dfNodataValue;
        }
      }else{
        for(size_t j=0;j<fieldSize;j++){
          ((float*)dataSource->dataObject[d]->cdfVariable->data)[j]=NAN;
        }
      }
    }
    
    
    
    float *lonData=(float*)pointLon->data;
    float *latData=(float*)pointLat->data;
    
    int numStations=pointVar[0]->dimensionlinks[0]->getSize();
    int numDates=pointVar[0]->dimensionlinks[1]->getSize();
    
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("numStations %d ",numStations);
    CDBDebug("numDates %d ",numDates);
    #endif
    
    

  
    CImageWarper imageWarper;
    bool projectionRequired=false;
    if(dataSource->srvParams->Geo->CRS.length()>0){
      projectionRequired=true;
      for(size_t d=0;d<nrDataObjects;d++){
        new2DVar[d]->setAttributeText("grid_mapping","customgridprojection");
      }
      if(cdfObject0->getVariableNE("customgridprojection")==NULL){
        CDF::Variable *projectionVar = new CDF::Variable();
        projectionVar->name.copy("customgridprojection");
        cdfObject0->addVariable(projectionVar);
        dataSource->nativeEPSG = dataSource->srvParams->Geo->CRS.c_str();
        imageWarper.decodeCRS(&dataSource->nativeProj4,&dataSource->nativeEPSG,&dataSource->srvParams->cfg->Projection);
        if(dataSource->nativeProj4.length()==0){
          dataSource->nativeProj4=LATLONPROJECTION;
          dataSource->nativeEPSG="EPSG:4326";
          projectionRequired=false;
        }
        projectionVar->setAttributeText("proj4_params",dataSource->nativeProj4.c_str());
      }
    }
    
    
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("Datasource CRS = %s nativeproj4 = %s",dataSource->nativeEPSG.c_str(),dataSource->nativeProj4.c_str());
    CDBDebug("Datasource bbox:%f %f %f %f",dataSource->srvParams->Geo->dfBBOX[0],dataSource->srvParams->Geo->dfBBOX[1],dataSource->srvParams->Geo->dfBBOX[2],dataSource->srvParams->Geo->dfBBOX[3]);
    CDBDebug("Datasource width height %d %d",dataSource->dWidth,dataSource->dHeight);
    #endif
    
    
    if(projectionRequired){
      int status = imageWarper.initreproj(dataSource,dataSource->srvParams->Geo,&dataSource->srvParams->cfg->Projection);
      if(status !=0 ){
        CDBError("Unable to init projection");
        return 1;
      }
    }
  
    
    //Read ID's
    CDF::Variable *pointID = cdfObject0->getVariableNE("station");
    if(pointID!=NULL){
      pointID->readData(CDF_STRING);
    }
    
    
    int dateDimIndex=dataSource->getDimensionIndex("time");
    //CDBDebug("Date DIM %d numStations = %d",dateDimIndex,numStations);
   
    for(int stationNr=0;stationNr<numStations;stationNr++){ 
      int pPoint = stationNr*numDates+dateDimIndex;//*numStations;
      int pGeo = stationNr;
      //CDBDebug("stationNr %d dateDimIndex %d,pPoint DIM %d",stationNr,dateDimIndex,pPoint);
      double lat,lon;
      lon = (double)lonData[pGeo];
      lat = (double)latData[pGeo];
      if(projectionRequired)imageWarper.reprojfromLatLon(lon,lat);
      int dlon=int((lon-offsetX)/cellSizeX);
      int dlat=int((lat-offsetY)/cellSizeY);
      
      for(size_t d=0;d<nrDataObjects;d++){
        float val = ((float*)pointVar[d]->data)[pPoint];
        float *sdata = ((float*)dataSource->dataObject[d]->cdfVariable->data);
        if(val!=fill){
          const char *id="";
          if(pointID!=NULL){
            id=((const char**)pointID->data)[pGeo];
          }
         
          
          //CDBDebug("P %d %d %f",dlon,dlat,val);
          if(dlon>=0&&dlat>=0&&dlon<dataSource->dWidth&&dlat<dataSource->dHeight){
          
            dataSource->dataObject[d]->points.push_back(PointDV(dlon,dlat,val,id));
          }        
          
          drawCircle(sdata,val,dataSource->dWidth,dataSource->dHeight,dlon-1,dlat,5);
        }
      }
    }
   
    imageWarper.closereproj();
   
  }
  #ifdef CCONVERTADAGUCPOINT_DEBUG
  CDBDebug("/convertADAGUCPointData");
  #endif
  return 0;
}
