#ifndef CDATASOURCE_H
#define CDATASOURCE_H
#include <math.h>
#include "CXMLSerializerInterface.h"
#include "CServerParams.h"
#include "CServerConfig_CPPXSD.h"
#include "CDebugger.h"
#include "Definitions.h"
#include "CTypes.h"
#include "CCDFDataModel.h"
#include "COGCDims.h"
#include "CStopWatch.h"
#include "CPGSQLDB.h"


/**
 * This class represents data to be used further in the server. Specific  metadata and data is filled in by CDataReader
 * This class is used for both image drawing (WMS) and data output (WCS)
 */
class CDataSource{
  private:
   DEF_ERRORFUNCTION();
 
  public:
  class StatusFlag{
    public:
      CT::string meaning;
      double value;
  };

  class DataClass{
    public:
      DataClass();
      ~DataClass();
      bool hasStatusFlag,hasNodataValue,appliedScaleOffset,hasScaleOffset;
      double dfNodataValue,dfscale_factor,dfadd_offset;
      std::vector<StatusFlag*> statusFlagList;
      CDF::Variable *cdfVariable;
      CDFObject *cdfObject;
      
      CT::string variableName,units;
      std::vector<PointDVWithLatLon> points;
  };
  
  class Statistics{
    private:
      template <class T>
      void calcMinMax(size_t size,std::vector <DataClass *> &dataObject){
#ifdef MEASURETIME
  StopWatch_Stop("Start min/max calculation");
#endif
      if(dataObject.size()==1){
        T* data = (T*)dataObject[0]->cdfVariable->data;
        
        CDFType type=dataObject[0]->cdfVariable->type;
        
        //CDBDebug("nodataval %f",(T)dataObject[0]->dfNodataValue);
        
        T _min=(T)0.0f,_max=(T)1.0f;
        
        T maxInf=(T)INFINITY;
        T minInf=(T)-INFINITY;
        
        bool checkInfinity = false;
        if(type==CDF_FLOAT||type==CDF_DOUBLE)checkInfinity=true;
        
        //CDBDebug("MINMAX %f %f %s",(double)maxInf,(double)minInf,CDF::getCDFDataTypeName(type).c_str());
        int firstDone=0;
        for(size_t p=0;p<size;p++){
          
          T v=data[p];

          //CDBDebug("Value %d =  %f",p,(double)v);
          if((((T)v)!=(T)dataObject[0]->dfNodataValue||(!dataObject[0]->hasNodataValue))&&v==v){
            if((checkInfinity&&v!=maxInf&&v!=minInf)||(!checkInfinity))
            {
            
              if(firstDone==0){
                _min=v;_max=v;
                firstDone=1;
              }else{
                if(v<_min)_min=v;
                if(v>_max)_max=v;
              }
            }
          }
        }
        min=(double)_min;
        max=(double)_max;
      }
      //Wind vector min max calculation
      if(dataObject.size()==2){
         T* dataU = (T*)dataObject[0]->cdfVariable->data;
         T* dataV = (T*)dataObject[1]->cdfVariable->data;
      //CDBDebug("nodataval %f",(T)dataObject->dfNodataValue);
        T _min=(T)0.0f,_max=(T)0.0f;
        int firstDone=0;
        T s =0;
        for(size_t p=0;p<size;p++){
          
          T u=dataU[p];
          T v=dataV[p];
          
          if(((((T)v)!=(T)dataObject[0]->dfNodataValue||(!dataObject[0]->hasNodataValue))&&v==v)&&
            ((((T)u)!=(T)dataObject[0]->dfNodataValue||(!dataObject[0]->hasNodataValue))&&u==u)){
            s=(T)hypot(u,v);
            if(firstDone==0){
              _min=s;_max=s;
              firstDone=1;
            }else{
              
              if(s<_min)_min=s;
              if(s>_max)_max=s;
            }
          }
        }
        min=(double)_min;
        max=(double)_max;
      }
#ifdef MEASURETIME
  StopWatch_Stop("Finished min/max calculation");
#endif
      }
      double min,max;
    public:
      Statistics(){
        min=0;
        max=0;
      }
      double getMinimum();
      double getMaximum();
      void setMinimum(double min);
      void setMaximum(double max);
      int calculate(CDataSource *dataSource);      // TODO this currently works only for float data
  };
  
  class TimeStep{
    public:
      CT::string fileName;   //Filename of the file to load
      CT::string timeString; //String of the current time
      CCDFDims   dims;//Dimension index in the corresponding name and file
  };
  int datasourceIndex;
  int currentAnimationStep;
  std::vector <TimeStep*> timeSteps;
  bool stretchMinMax;
  std::vector <COGCDims*> requiredDims;
  Statistics *statistics;
  //The actual dataset data (can have multiple variables)
  std::vector <DataClass *> dataObject;
  //source image parameters
  double dfBBOX[4],dfCellSizeX,dfCellSizeY;
  int dWidth,dHeight;
  CT::string nativeEPSG;
  CT::string nativeProj4;

  //Used for scaling the legend to the palette range of 0-240
  float legendScale,legendOffset,legendLog,legendLowerRange,legendUpperRange;
  bool legendValueRange;
  CT::string styleName;

  //Number of metadata items
  size_t nrMetadataItems;
  CT::string *metaData;

  //Configured?
  bool isConfigured;
  
  
  //Used for vectors and points
  bool level2CompatMode;
  
  //The index of the X and Y dimension in the variable dimensionlist (not the id's from the netcdf file)
  int dimXIndex;
  int dimYIndex;
  
  //The striding of the read 2D map
  int stride2DMap;
  
  
  // Lon transformation is used to swap datasets from 0-360 degrees to -180 till 180 degrees
  //Swap data from >180 degrees to domain of -180 till 180 in case of lat lon source data
  int useLonTransformation;
  
  //Sometimes X and Y need to be swapped, this boolean indicates whether it should or not.
  bool swapXYDimensions;
  
  //X and Y variables of the 2D field
  CDF::Variable *varX;
  CDF::Variable *varY;
   
  
  
  //Numver of dims
  int dNetCDFNumDims;
  int dLayerType;
  CT::string layerName;

  //Current value index of the dim
  //int dOGCDimValues[MAX_DIMS];
  
  CServerParams *srvParams;

  //Link to the XML configuration
  CServerConfig::XMLE_Layer * cfgLayer;
  CServerConfig::XMLE_Configuration *cfg;
  
  //Link to the root CDFObject, is owned by the datareader.
  
  
  CDataSource();
  ~CDataSource();
  static void readStatusFlags(CDF::Variable * var, std::vector<CDataSource::StatusFlag*> *statusFlagList);
  static const char *getFlagMeaning(std::vector<CDataSource::StatusFlag*> *statusFlagList,double value);
  static void getFlagMeaningHumanReadable(CT::string *flagMeaning ,std::vector<CDataSource::StatusFlag*> *statusFlagList,double value);
  int autoCompleteDimensions(CPGSQLDB *dataBaseConnection);
  int checkDimTables(CPGSQLDB *dataBaseConnection);
  
  
  int setCFGLayer(CServerParams *_srvParams,CServerConfig::XMLE_Configuration *_cfg,CServerConfig::XMLE_Layer * _cfgLayer,const char *_layerName, int layerIndex);
  void addTimeStep(const char * pszName,const char *pszTimeString);
  const char *getFileName();
  void setTimeStep(int timeStep);
  int getCurrentTimeStep();
  size_t getDimensionIndex(const char *name);
  size_t getDimensionIndex(int i);
  int getNumTimeSteps();
  const char *getLayerName();
  
  
  int attachCDFObject(CDFObject *cdfObject){
    if(cdfObject==NULL){
      CDBError("cdfObject==NULL");
      return 1;
    }
    if(isConfigured==false){
      CDBError("Datasource %s is not configured",cfgLayer->Name[0]->value.c_str());
      return 1;
    }
    if(dataObject.size()<=0){
      CDBError("No variables found for datasource %s",cfgLayer->Name[0]->value.c_str());
      return 1;
    }
  
    for(size_t varNr=0;varNr<dataObject.size();varNr++){
      dataObject[varNr]->cdfObject = cdfObject;
      dataObject[varNr]->cdfVariable = cdfObject->getVariableNE(dataObject[varNr]->variableName.c_str());
      if(dataObject[varNr]->cdfVariable==NULL){
        CDBError("attachCDFObject: variable \"%s\" does not exist",dataObject[varNr]->variableName.c_str());
        return 1;
      }
    }
    return 0;
  }
  void detachCDFObject(){
    for(size_t j=0;j<dataObject.size();j++){
      dataObject[j]->cdfVariable = NULL;
      dataObject[j]->cdfObject = NULL;
    }
  }
  
};

#endif
