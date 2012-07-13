#ifndef CDataSource_H
#define CDataSource_H
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
      //void *data;

      std::vector<StatusFlag*> statusFlagList;
      CDF::Variable *cdfVariable;
      CDFObject *cdfObject;
      //CDFType dataType;
      CT::string variableName,units;

      
  };
  
  class Statistics{
    private:
      template <class T>
      
      void calcMinMax(T *data,size_t size,DataClass *dataObject){
#ifdef MEASURETIME
  StopWatch_Stop("Start min/max calculation");
#endif
  
      //CDBDebug("nodataval %f",(T)dataObject->dfNodataValue);
        T _min=(T)0.0f,_max=(T)0.0f;
        int firstDone=0;
        for(size_t p=0;p<size;p++){
          
          T v=data[p];

          if((((T)v)!=(T)dataObject->dfNodataValue||(!dataObject->hasNodataValue))&&v==v){
            if(firstDone==0){
              _min=v;_max=v;
              firstDone=1;
            }else{
              if(v<_min)_min=v;
              if(v>_max)_max=v;
            }
          }
        }
        min=(double)_min;
        max=(double)_max;
#ifdef MEASURETIME
  StopWatch_Stop("Finished min/max calculation");
#endif
      }
      double min,max;
    public:
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

  //Number of metadata items
  size_t nrMetadataItems;
  CT::string *metaData;

  //Configured?
  bool isConfigured;

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
