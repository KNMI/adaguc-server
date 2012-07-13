#include "CConvertASCAT.h"
#include "CFillTriangle.h"
const char *CConvertASCAT::className="CConvertASCAT";

int CConvertASCAT::convertASCATHeader( CDFObject *cdfObject ){

  try{
    cdfObject->getDimension("NUMROWS");
    cdfObject->getDimension("NUMCELLS");
  }catch(int e){
    return 1;
  }
  bool hasTimeData = false;
  
  //if(backupT==NULL){
    CDF::Variable *origT = cdfObject->getVariableNE("time");
    if(origT!=NULL){
      hasTimeData=true;
      //origT->name.concat("_backup");
      CDF::Dimension *dimT=new CDF::Dimension();
      dimT->name="time2D";
      dimT->setSize(1);
      cdfObject->addDimension(dimT);
      CDF::Variable *varT = new CDF::Variable();
      varT->type=CDF_DOUBLE;
      varT->name.copy(dimT->name.c_str());
      varT->setAttributeText("standard_name","time");
      varT->setAttributeText("long_name","time");
      varT->dimensionlinks.push_back(dimT);
      CDF::allocateData(CDF_DOUBLE,&varT->data,dimT->length);
      cdfObject->addVariable(varT);
      
      //Detect time from the netcdf data
      //Copy the same units from the original time variable
      if(origT!=NULL){
        try{
          
          varT->setAttributeText("units",origT->getAttribute("units")->toString().c_str());
          //CDBDebug("origTtimeunits = %s",origT->getAttribute("units")->toString().c_str());
          //CDBDebug("varTtimeunits = %s",varT->getAttribute("units")->toString().c_str());
          if(origT->readData(CDF_DOUBLE)!=0){
            CDBError("Unable to read time variable");
          }else{
            double tfill;
            bool hastfill =false;
            try{
              origT->getAttribute("_FillValue")->getData(&tfill,1);
              hastfill=true;
            }catch(int e){}
            double *tdata=((double *)origT->data);
            double firstTimeValue = tdata[0];
            size_t tsize = origT->getSize();
            if(hastfill==true){
              for(size_t j=0;j<tsize;j++){
                if(tdata[j]!=tfill){
                  firstTimeValue = tdata[j];
                }
              }
            }
            #ifdef CCONVERTASCAT_DEBUG
            CDBDebug("firstTimeValue  = %f",firstTimeValue );
            #endif
            varT->setData(CDF_DOUBLE,&firstTimeValue,1);
          }
        }catch(int e){}
      }
      
    }
  double dfBBOX[]={-180,-90,180,90};
  int width=2;
  int height=2;
  
  double cellSizeX=(dfBBOX[2]-dfBBOX[0])/double(width);
  double cellSizeY=(dfBBOX[3]-dfBBOX[1])/double(height);
  double offsetX=dfBBOX[0];
  double offsetY=dfBBOX[1];
  
  
  CDF::Dimension *dimX = cdfObject->getDimensionNE("x");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("y");
  CDF::Variable *varX = cdfObject->getVariableNE("x");
  CDF::Variable *varY = cdfObject->getVariableNE("y");
  if(dimX==NULL||dimY==NULL||varX==NULL||varY==NULL) {
    //Create new dimensions and variables (X,Y,T)
    dimX=new CDF::Dimension();
    dimX->name="x";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    
    
    dimY=new CDF::Dimension();
    dimY->name="y";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    
    
    
    varX = new CDF::Variable();
    varX->type=CDF_DOUBLE;
    varX->name.copy("x");
    varX->isDimension=true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    
    varY = new CDF::Variable();
    varY->type=CDF_DOUBLE;
    varY->name.copy("y");
    varY->isDimension=true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    
    
    
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
  }

  
  
  CT::StackList<CT::string> varsToConvert;
  for(size_t v=0;v<cdfObject->variables.size();v++){
    CDF::Variable *var = cdfObject->variables[v];
    if(var->isDimension==false){
      if(!var->name.equals("time2D")&&!var->name.equals("time")&&!var->name.equals("lon")&&!var->name.equals("lat")){
        varsToConvert.add(CT::string(var->name.c_str()));
      }
    }
  }
  
  CDF::Variable *swathVar;
  for(size_t v=0;v<varsToConvert.size();v++){
    swathVar=cdfObject->getVariable(varsToConvert[v].c_str());
    #ifdef CCONVERTASCAT_DEBUG
    CDBDebug("Converting %s",swathVar->name.c_str());
    #endif
    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);
    
    if(hasTimeData){
      CDF::Variable *newTimeVar=cdfObject->getVariableNE("time2D");             
      if(newTimeVar!=NULL){
        new2DVar->dimensionlinks.push_back(newTimeVar->dimensionlinks[0]);
      }
    }
    new2DVar->dimensionlinks.push_back(dimY);
    new2DVar->dimensionlinks.push_back(dimX);
    
    new2DVar->type=swathVar->type;
    new2DVar->name=swathVar->name.c_str();
    swathVar->name.concat("_backup");
  
    
    //copy attributes
    for(size_t j=0;j<swathVar->attributes.size();j++){
      CDF::Attribute *a =swathVar->attributes[j];
      new2DVar->setAttribute(a->name.c_str(),a->type,a->data,a->length);
    }
    swathVar->setAttributeText("ADAGUC_SKIP","true");
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");
    new2DVar->type=CDF_FLOAT;
  }
  return 0;
}





int CConvertASCAT::convertASCATData(CDataSource *dataSource,int mode){
  
  #ifdef CCONVERTASCAT_DEBUG
  CDBDebug("convertASCATData");
  #endif
  CDFObject *cdfObject = dataSource->dataObject[0]->cdfObject;
  try{
    cdfObject->getDimension("NUMROWS");
    cdfObject->getDimension("NUMCELLS");
  }catch(int e){
    return 1;
  }
  
  CDF::Variable *new2DVar;
  new2DVar = dataSource->dataObject[0]->cdfVariable;
  
  CDF::Variable *swathVar;
  CT::string origSwathName=new2DVar->name.c_str();
  origSwathName.concat("_backup");
  swathVar=cdfObject->getVariableNE(origSwathName.c_str());
  if(swathVar==NULL){
    CDBError("Unable to find orignal swath variable with name %s",origSwathName.c_str());
    return 1;
  }
  CDF::Variable *swathLon;
  CDF::Variable *swathLat;


 
  try{
    swathLon = cdfObject->getVariable("lon");
    swathLat = cdfObject->getVariable("lat");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }

  //Read original data first 
  swathVar->readData(CDF_FLOAT,true);
  swathLon->readData(CDF_FLOAT,true);
  swathLat->readData(CDF_FLOAT,true);
 
  CDF::Attribute *fillValue = swathVar->getAttributeNE("_FillValue");
  if(fillValue!=NULL){
    dataSource->dataObject[0]->hasNodataValue=true;
    fillValue->getData(&dataSource->dataObject[0]->dfNodataValue,1);
    #ifdef CCONVERTASCAT_DEBUG
    CDBDebug("_FillValue = %f",dataSource->dataObject[0]->dfNodataValue);
    #endif
    float f=dataSource->dataObject[0]->dfNodataValue;
    new2DVar->getAttribute("_FillValue")->setData(CDF_FLOAT,&f,1);
  }else dataSource->dataObject[0]->hasNodataValue=false;
  
  //Detect minimum and maximum values
  float fill = (float)dataSource->dataObject[0]->dfNodataValue;
  float min = fill;float max=fill;
  for(size_t j=0;j<swathVar->getSize();j++){
    float v=((float*)swathVar->data)[j];
    if(v!=fill){
      if(min==fill)min=v;
      if(max==fill)max=v;
      if(v<min)min=v;
      if(v>max)max=v;
    }
  }
  
  #ifdef CCONVERTASCAT_DEBUG
  CDBDebug("Calculated min/max : %f %f",min,max);
  #endif
  
  //Set statistics
  if(dataSource->stretchMinMax){
    #ifdef CCONVERTASCAT_DEBUG
    CDBDebug("dataSource->stretchMinMax");
    #endif
    if(dataSource->statistics==NULL){
      #ifdef CCONVERTASCAT_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f",min,max);
      #endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(max);
      dataSource->statistics->setMinimum(min);
      
    }
  }
  
  //Make the width and height of the new 2D ascat field the same as the viewing window
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
 
  #ifdef CCONVERTASCAT_DEBUG
  CDBDebug("Datasource bbox:%f %f %f %f",dataSource->srvParams->Geo->dfBBOX[0],dataSource->srvParams->Geo->dfBBOX[1],dataSource->srvParams->Geo->dfBBOX[2],dataSource->srvParams->Geo->dfBBOX[3]);
  CDBDebug("Datasource width height %d %d",dataSource->dWidth,dataSource->dHeight);
  CDBDebug("L2 %d %d",dataSource->dWidth,dataSource->dHeight);
  #endif
    
  //if(dataSource->srvParams->requestType==REQUEST_WMS_GETMAP||dataSource->srvParams->requestType==REQUEST_WCS_GETCOVERAGE||dataSource->srvParams->requestType==REQUEST_WMS_GETFEATUREINFO||dataSource->srvParams->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
    if(mode==CNETCDFREADER_MODE_OPEN_ALL){//&&!dataSource->srvParams->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
    #ifdef CCONVERTASCAT_DEBUG
    CDBDebug("Drawing %s",new2DVar->name.c_str());
    #endif
    
    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX ;
    CDF::Variable *varY;
  
    //Create new dimensions and variables (X,Y,T)
    dimX=cdfObject->getDimension("x");
    dimX->setSize(dataSource->dWidth);
    
    dimY=cdfObject->getDimension("y");
    dimY->setSize(dataSource->dHeight);
    
    varX = cdfObject->getVariable("x");
    varY = cdfObject->getVariable("y");
    
    
    
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
    
    size_t fieldSize = dataSource->dWidth*dataSource->dHeight;
    new2DVar->setSize(fieldSize);
    CDF::allocateData(new2DVar->type,&(new2DVar->data),fieldSize);
    
    //Draw data!
    for(size_t j=0;j<fieldSize;j++){
      ((float*)dataSource->dataObject[0]->cdfVariable->data)[j]=(float)dataSource->dataObject[0]->dfNodataValue;
    }
    
    float *sdata = ((float*)dataSource->dataObject[0]->cdfVariable->data);
    
    float *lonData=(float*)swathLon->data;
    float *latData=(float*)swathLat->data;
    
    int numRows=swathVar->dimensionlinks[0]->getSize();
    int numCells=swathVar->dimensionlinks[1]->getSize();
    #ifdef CCONVERTASCAT_DEBUG
    CDBDebug("numRows %d numCells %d",numRows,numCells);
    #endif
    
    float *swathData = (float*)swathVar->data;
    
    for(int rowNr=0;rowNr<numRows;rowNr++){ 
      for(int cellNr=0;cellNr<numCells;cellNr++){
        
        int pSwath = cellNr+rowNr * numCells;
        
        int bo = (rowNr == 0 ?numCells:-numCells);
        
        float lons[4],lats[4],vals[4];
        lons[0] = lonData[pSwath];
        lons[1]= lonData[pSwath+(cellNr==0?1:-1)];
        lons[2] = lonData[pSwath+bo];
        lons[3] = lonData[pSwath+bo+(cellNr==0?1:-1)];
        
        lats[0] = latData[pSwath];
        lats[1]= latData[pSwath+(cellNr==0?1:-1)];
        lats[2] = latData[pSwath+bo];
        lats[3] = latData[pSwath+bo+(cellNr==0?1:-1)];
        
        vals[0] = swathData[pSwath];
        vals[1]= swathData[pSwath+(cellNr==0?1:-1)];
        vals[2] = swathData[pSwath+bo];
        vals[3] = swathData[pSwath+bo+(cellNr==0?1:-1)];
        
        float cx=0,cy=0,cv=vals[0];
        
        bool tileIsTooLarge=false;
        bool moveTile=false;
        
        for(int j=0;j<4;j++){
          if(lons[j]==fill){tileIsTooLarge=true;break;}
          if(lons[j]>185)moveTile=true;
        }
        float lon0 ;
        float lat0;
        if(tileIsTooLarge==false){
          for(int j=0;j<4;j++){
            if(moveTile==true)lons[j]-=360;
            if(lons[j]<-280)lons[j]+=360;
            if(j==0){
              lon0 =lons[0];
              lat0 =lats[0];
            }
            if(fabs(lon0-lons[j])>5)tileIsTooLarge=true;
            if(fabs(lat0-lats[j])>5)tileIsTooLarge=true;
            lons[j]=(lons[j]-offsetX)/cellSizeX;
            lats[j]=(lats[j]-offsetY)/cellSizeY;
            cx+=lons[j];
            cy+=lats[j];
            if(j>0){
              if(cv!=fill){
                if(vals[j]==fill){cv=fill;}else cv+=vals[j];
              }
            }
          }
        }
        //vals[0]=10;
        vals[1]=vals[0];
        vals[2]=vals[0];
        vals[3]=vals[0];
        cv=vals[0]*4;
        
        if(tileIsTooLarge==false){
          cx/=4;cy/=4;
          if(cv!=fill)cv/=4;
          if(cv!=fill){
            float cornerV[3];
            int cornerX[3];
            int cornerY[3];
            
            cornerX[0]=(int)lons[0];cornerY[0]=(int)lats[0];cornerV[0]=vals[0];
            cornerX[1]=(int)lons[1];cornerY[1]=(int)lats[1];cornerV[1]=vals[1];
            cornerX[2]=(int)  cx   ;cornerY[2]=(int) cy    ;cornerV[2]=cv;
            fillTriangle(sdata, cornerV, dataSource->dWidth,dataSource->dHeight, cornerX,cornerY);
            
            cornerX[0]=(int)lons[1];cornerY[0]=(int)lats[1];cornerV[0]=vals[1];
            cornerX[1]=(int)lons[3];cornerY[1]=(int)lats[3];cornerV[1]=vals[3];
            cornerX[2]=(int)  cx   ;cornerY[2]=(int) cy    ;cornerV[2]=cv;
            fillTriangle(sdata, cornerV, dataSource->dWidth,dataSource->dHeight, cornerX,cornerY);
            
            cornerX[0]=(int)lons[3];cornerY[0]=(int)lats[3];cornerV[0]=vals[3];
            cornerX[1]=(int)lons[2];cornerY[1]=(int)lats[2];cornerV[1]=vals[2];
            cornerX[2]=(int)  cx   ;cornerY[2]=(int) cy    ;cornerV[2]=cv;
            fillTriangle(sdata, cornerV, dataSource->dWidth,dataSource->dHeight, cornerX,cornerY);
            
            cornerX[0]=(int)lons[2];cornerY[0]=(int)lats[2];cornerV[0]=vals[2];
            cornerX[1]=(int)lons[0];cornerY[1]=(int)lats[0];cornerV[1]=vals[0];
            cornerX[2]=(int)  cx   ;cornerY[2]=(int) cy    ;cornerV[2]=cv;
            fillTriangle(sdata, cornerV, dataSource->dWidth,dataSource->dHeight, cornerX,cornerY);
          }
        }
      }
    }
  }
  #ifdef CCONVERTASCAT_DEBUG
  CDBDebug("/convertASCATData");
  #endif
  return 0;
}
