#include <vector>
#include <algorithm>
#include "CMakeJSONTimeSeries.h"
#include "CImageDataWriter.h"

const char * CMakeJSONTimeSeries::className = "CMakeJSONTimeSeries";

// #define CMakeJSONTimeSeries_DEBUG

#define CMakeJSONTimeSeries_MAX_DIMS 255

class UniqueRequests{
private:
     DEF_ERRORFUNCTION();

public:
   bool readDataAsCDFDouble;
  class AggregatedDimension{
  public:
    CT::string name;
    int start;
    std::vector<CT::string> values;
    
  };

  typedef std::map<int ,CT::string*>::iterator it_type_dimvalindex;
  class DimInfo{
    public:
     std::map <int ,CT::string*> dimValuesMap; // All values, many starts with 1 count, result of set()
     std::vector<AggregatedDimension*> aggregatedValues; //Aggregated values (start/count series etc), result of  addDimSet()
     ~DimInfo(){
        for(it_type_dimvalindex dimvalindexmapiterator = dimValuesMap.begin(); dimvalindexmapiterator != dimValuesMap.end(); dimvalindexmapiterator++) {
          delete dimvalindexmapiterator->second;
        }
        for(size_t j=0;j<aggregatedValues.size();j++){
          delete aggregatedValues[j];
        }
     }
  };
  
  typedef std::map<std::string ,DimInfo*>::iterator it_type_diminfo;
      
  class Request{
  public:
    int numDims;
    AggregatedDimension *dimensions[CMakeJSONTimeSeries_MAX_DIMS];
 
  };
  
  
  
  
  
  
  class FileInfo{
  public:
    std::vector<Request*>requests;
    std::map <std::string ,DimInfo*> dimInfoMap;//AggregatedDimension name is key
    ~FileInfo(){
      for(it_type_diminfo diminfomapiterator = dimInfoMap .begin(); diminfomapiterator != dimInfoMap .end(); diminfomapiterator++) {
        delete diminfomapiterator->second;
      }
      for(size_t j=0;j<requests.size();j++){
        delete requests[j];
      }
    }
   };
  
  std::map <std::string ,FileInfo*> fileInfoMap;//File name is key
  


  typedef std::map<std::string ,FileInfo*>::iterator it_type_file;

  
  CT::string *dimensionKeys[CMakeJSONTimeSeries_MAX_DIMS];
  
  int dimOrdering[CMakeJSONTimeSeries_MAX_DIMS];
  
  int *getDimOrder(){
    return dimOrdering;
  }
  
  class Result{
    private:
    UniqueRequests *parent;
  public:
    Result( UniqueRequests *parent){
      this->parent=parent;
    }
     CT::string *dimensionKeys[CMakeJSONTimeSeries_MAX_DIMS];
     CT::string value;
     int numDims;
     int *getDimOrder(){return parent->getDimOrder();}
      
  };
  std::vector<Result*> results;
  
  UniqueRequests(){
    readDataAsCDFDouble = false;
  }
  ~UniqueRequests(){
    typedef std::map<std::string ,FileInfo*>::iterator it_type_file;
    for(it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
       
        delete filemapiterator->second;
    }
    for(size_t j=0;j<results.size();j++){
      delete results[j];
    }
    results.clear();
  }
  
  
  void set(const char*filename,const char*dimName,size_t dimIndex,CT::string dimValue){
   
    /* Find the right file based on filename */
    FileInfo *fileInfo = NULL;
    std::map<std::string,FileInfo*>::iterator itf=fileInfoMap.find(filename);
    if(itf!=fileInfoMap.end()){
      fileInfo = (*itf).second;
    }else{
      fileInfo = new FileInfo();
      fileInfoMap.insert(std::pair<std::string,FileInfo*>(filename,fileInfo));
    }
    
    /* Find the right diminfo based on dimension name */
    DimInfo *dimInfo = NULL;
    std::map<std::string, DimInfo*>::iterator itd=fileInfo->dimInfoMap.find(dimName);
    if(itd!=fileInfo->dimInfoMap.end()){
      dimInfo=(*itd).second;
    }else{
      dimInfo = new DimInfo();
      fileInfo->dimInfoMap.insert(std::pair<std::string, DimInfo*>(dimName,dimInfo));
    }
    
    /* Find the right dimension indexes and values based on dimension index */
    CT::string *dimIndexesAndValues = NULL;
    std::map<int, CT::string*>::iterator itdi=dimInfo->dimValuesMap.find(dimIndex);
    if(itdi!=dimInfo->dimValuesMap.end()){
      dimIndexesAndValues = (*itdi).second;
    }else{
      dimIndexesAndValues = new CT::string();
      dimInfo->dimValuesMap.insert(std::pair<int, CT::string*>(dimIndex,dimIndexesAndValues));
    }
    

    dimIndexesAndValues->copy(dimValue.c_str());
#ifdef CMakeJSONTimeSeries_DEBUG
    CDBDebug("Adding %s %d %s",dimName,dimIndex,dimValue.c_str());
#endif    
  }
  
  void addDimSet(DimInfo* dimInfo,int start,std::vector <CT::string> *valueList){
#ifdef CMakeJSONTimeSeries_DEBUG    
    CDBDebug("Adding %d with %d values",start,valueList->size());
#endif    
    AggregatedDimension * aggregatedValue = new AggregatedDimension();
    aggregatedValue->start=start;
    aggregatedValue->values=*valueList;
    dimInfo->aggregatedValues.push_back(aggregatedValue);
  }

  AggregatedDimension *dimensions[CMakeJSONTimeSeries_MAX_DIMS];

  void nestRequest(it_type_diminfo diminfomapiterator,FileInfo*fileInfo,int depth){
    if(diminfomapiterator != fileInfo->dimInfoMap .end()){
      it_type_diminfo currentIt =  diminfomapiterator;
      int currentDepth = depth;
      diminfomapiterator++;
      depth++;
      for(size_t j=0;j<(currentIt->second)->aggregatedValues.size();j++){
        AggregatedDimension * aggregatedValue = (currentIt->second)->aggregatedValues[j];
        aggregatedValue->name=(currentIt->first).c_str();
        dimensions[currentDepth]=aggregatedValue;
        nestRequest(diminfomapiterator,fileInfo,depth);
      }
      return;
    }else{
#ifdef CMakeJSONTimeSeries_DEBUG
      CDBDebug("Add request with following:");
#endif      
      Request *request = new Request();
      for(int j=0;j<depth;j++){
        //CDBDebug("  %d %s %d %d",j,dimensions[j]->name.c_str(),dimensions[j]->start,dimensions[j]->values.size());
        request->dimensions[j]=dimensions[j];
      }
      request->numDims = depth;
      fileInfo->requests.push_back(request);
      return;
    }
  }
  
  void sortAndAggregate(){
    for(it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
      
      for(it_type_diminfo diminfomapiterator = (filemapiterator->second)->dimInfoMap .begin(); diminfomapiterator != (filemapiterator->second)->dimInfoMap .end(); diminfomapiterator++) {
#ifdef CMakeJSONTimeSeries_DEBUG
        CDBDebug("%s/%s",(filemapiterator->first).c_str(),(diminfomapiterator->first).c_str());
#endif        
        std::map <int ,CT::string*> *dimValuesMap=&diminfomapiterator->second->dimValuesMap;
        int currentDimIndex=-1;
        int dimindex;
        
        int startDimIndex;
        std::vector<CT::string>dimValues;
        for(it_type_dimvalindex dimvalindexmapiterator = dimValuesMap ->begin(); dimvalindexmapiterator != dimValuesMap->end(); dimvalindexmapiterator++) {
          //const char *filename=(filemapiterator->first).c_str();
          //const char *dimname=(diminfomapiterator->first).c_str();
          dimindex=dimvalindexmapiterator->first;
          const char *dimvalue=dimvalindexmapiterator->second->c_str();
         
          
          if(currentDimIndex != -1){
            if(currentDimIndex == dimindex-1){
              currentDimIndex=dimindex;
            }else{
              
              //*** GO ***
#ifdef CMakeJSONTimeSeries_DEBUG              
              CDBDebug("Print stop at %d",currentDimIndex);
#endif              
              currentDimIndex=-1;
              addDimSet(diminfomapiterator->second,startDimIndex,&dimValues);
            }
          }
          
          if(currentDimIndex == -1){
#ifdef CMakeJSONTimeSeries_DEBUG            
            CDBDebug("Print start at %d",dimindex);
#endif            
            currentDimIndex=dimindex;
            startDimIndex = dimindex;
            dimValues.clear();
          }
          
          if(currentDimIndex != -1){
#ifdef CMakeJSONTimeSeries_DEBUG            
             CDBDebug("Add %d/%s",dimindex,dimvalue);
#endif             
             dimValues.push_back(dimvalue);
          }
          
        }
        if(currentDimIndex!=-1){
          //*** GO ***
#ifdef CMakeJSONTimeSeries_DEBUG          
          CDBDebug("Print stop at %d",dimindex);
#endif          
          currentDimIndex=-1;
          addDimSet(diminfomapiterator->second,startDimIndex,&dimValues);
        }
        
      }
    }
    
    //Generate UniqueRequests
    for(it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
      nestRequest((filemapiterator->second)->dimInfoMap .begin(),filemapiterator->second,0);
    }
    
    
  }

  
  void expandData(CDataSource::DataObject *dataObject,CDF::Variable *variable,size_t *start,size_t *count,int d,Request *request,int index){
    if(d<int(variable->dimensionlinks.size())-2){
      CDF::Dimension *dim = variable->dimensionlinks[d];
      
      int requestDimIndex=-1;
      for(int i=0;i<request->numDims;i++){
        if(request->dimensions[i]->name.equals(dim->name.c_str())){
          requestDimIndex = i;
        }
      }
      if(requestDimIndex==-1){
        CDBError("Unable to find dimension %s in request",dim->name.c_str());throw(__LINE__);
      }
      if(count[d]!=request->dimensions[requestDimIndex]->values.size()){
        CDBError("count[d]!=request->dimensions[requestDimIndex]->values.size()");throw(__LINE__);
      }
    
      
      int multiplier = 1;
      for(size_t j=d+1;j<variable->dimensionlinks.size()-2;j++){
        multiplier*=count[j];
      }
      for(size_t j=0;j<request->dimensions[requestDimIndex]->values.size();j++){
        dimensionKeys[d] = &request->dimensions[requestDimIndex]->values[j];
        expandData(dataObject,variable,start,count,d+1,request,j*multiplier+index);
      }
    }else{
      double pixel=CImageDataWriter::convertValue(variable->getType(),variable->data,index);
      CT::string dataAsString="nodata";
      if((pixel!=dataObject->dfNodataValue&&dataObject->hasNodataValue==true&&pixel==pixel)||dataObject->hasNodataValue==false){
      
        //dataAsString.print("%f",data[index]);
        if(dataObject->hasStatusFlag){
          CT::string flagMeaning;
          CDataSource::getFlagMeaningHumanReadable(&flagMeaning,&dataObject->statusFlagList,pixel);
          dataAsString.print("%s (%d)",flagMeaning.c_str(),(int)pixel);
        }else{
          dataAsString.print("%f",pixel);//=szTemp;
        }
      }
      // CDBDebug("VAL [%s][%d][%f]",p.c_str(),index,data[index]);
      Result *result = new Result(this);

      for(size_t j=0;j<variable->dimensionlinks.size()-2;j++){
        result->dimensionKeys[j] = dimensionKeys[j];
      }
      result->value = dataAsString.c_str();
      result->numDims= variable->dimensionlinks.size()-2;
      results.push_back(result);
    }
  }
  
  void recurDataStructure(CXMLParser::XMLElement *dataStructure,Result *result,int depth,int *dimOrdering){
    int dimIndex = dimOrdering[depth];
    
    CT::string dimindexvalue = result->dimensionKeys[dimIndex]->c_str();
    
    CXMLParser::XMLElement *el = NULL;
    try{
      el = dataStructure->get(dimindexvalue.c_str());
    }catch(int e){
      dataStructure->add(CXMLParser::XMLElement(dimindexvalue.c_str()));
      el = dataStructure->getLast();
    }
    if(depth+1<result->numDims){
      recurDataStructure(el,result,depth+1,dimOrdering);
    }else{
      el->setValue(result->value.c_str());
    }
    
  }
  

   
  struct less_than_key{
      inline bool operator() (Result* result1, Result* result2)
      {
        int *dimOrder= result1->getDimOrder();
        std::string s1;
        std::string s2;
        for(int d=0;d<result1->numDims;d++){
          s1+=result1->dimensionKeys[dimOrder[d]]->c_str();
          s2+=result2->dimensionKeys[dimOrder[d]]->c_str();
        
        }
        if(s1.compare(s2)<0)return true;
        return false;
        //return (struct1.key < struct2.key);
      }
  };

  void createStructure(CDataSource::DataObject *dataObject,CDrawImage *drawImage,CImageWarper *imageWarper,CDataSource *dataSource,int dX,int dY,CXMLParser::XMLElement *gfiStructure){

    /* Determine ordering of dimensions */
    int numberOfDims = dataSource->requiredDims.size();
    int timeDimIndex = -1;
   
    for(int dimnr = 0;dimnr<numberOfDims;dimnr++){
      COGCDims *ogcDim=dataSource->requiredDims[dimnr];
      if(ogcDim->isATimeDimension){
        timeDimIndex=dimnr;
        break;
      }
    }
   
    for(int dimnr = 0;dimnr<numberOfDims;dimnr++){
      dimOrdering[dimnr]=dimnr;
    }
    if(timeDimIndex!=-1){
      int a= dimOrdering[timeDimIndex];
      int b= dimOrdering[numberOfDims-1];
      dimOrdering[timeDimIndex]=b;
      dimOrdering[numberOfDims-1]=a;
    }else{
      timeDimIndex = 0;
    }
    
    #ifdef CMakeJSONTimeSeries_DEBUG
    for(int dimnr = 0;dimnr<numberOfDims;dimnr++){
      CDBDebug("New order = %d/%d",dimnr,dimOrdering[dimnr]);
    }
    #endif        
    
    CXMLParser::XMLElement *layerStructure = gfiStructure->add("root");
    layerStructure->add(CXMLParser::XMLElement("name", dataSource->getLayerName()));
    
    /* Add metadata */
    CT::string standardName = dataObject->variableName.c_str();
    CDF::Attribute * attr_standard_name=dataObject->cdfVariable->getAttributeNE("standard_name");
    if(attr_standard_name!=NULL){
      standardName = attr_standard_name->toString();
    }

    layerStructure->add(CXMLParser::XMLElement("standard_name",standardName.c_str()));
    layerStructure->add(CXMLParser::XMLElement("units",dataObject->getUnits().c_str()));

    CT::string ckey;
    ckey.print("%d%d%s",dX,dY,dataSource->nativeProj4.c_str());
    CImageDataWriter::ProjCacheInfo projCacheInfo = CImageDataWriter::GetProjInfo(ckey,drawImage,dataSource,imageWarper, dataSource->srvParams,dX,dY);
    CXMLParser::XMLElement point("point");
    point.add(CXMLParser::XMLElement("SRS", "EPSG:4326"));
    CT::string coord;
    coord.print("%f,%f", projCacheInfo.lonX, projCacheInfo.lonY);
    point.add(CXMLParser::XMLElement("coords", coord.c_str()));
    layerStructure->add(point);
  
    for(size_t i=0;i<dataSource->requiredDims.size();i++){
      COGCDims *ogcDim=dataSource->requiredDims[dimOrdering[i]];
      layerStructure->add(CXMLParser::XMLElement("dims",ogcDim->name.c_str()));
    }
  
    CXMLParser::XMLElement *dataStructure= NULL;
    try{
      dataStructure = layerStructure->get("data");
    }catch(int e){
      layerStructure->add(CXMLParser::XMLElement("data"));
      dataStructure = layerStructure->getLast();
    }
    CDBDebug("Sorting");
    std::sort(results.begin(), results.end(), less_than_key());
    CDBDebug("Found %d elements",results.size());
    for(size_t j=0;j<results.size();j++){
      recurDataStructure(dataStructure,results[j],0,dimOrdering);
    }
  }
  
  void makeRequests(CDrawImage *drawImage,CImageWarper *imageWarper,CDataSource *dataSource,int dX,int dY,CXMLParser::XMLElement *gfiStructure){
    #ifdef CMakeJSONTimeSeries_DEBUG
    CDBDebug("makeRequests");
    #endif    
    CDataReader reader;
    int numberOfDims = dataSource->requiredDims.size();
    size_t start[numberOfDims+2],count[numberOfDims+2];ptrdiff_t stride[numberOfDims+2];
    
    reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
    


    #ifdef CMakeJSONTimeSeries_DEBUG
    CDBDebug("===================== iterating data objects ==================");
    #endif
    for(size_t dataObjectNr=0;dataObjectNr<dataSource->dataObjects.size();dataObjectNr++){
    //CDBDebug("Found %d elements",results.size());
      for(size_t j=0;j<results.size();j++){
        delete results[j];
      }
      results.clear();
      CDataSource::DataObject *dataObject = dataSource->getDataObject(dataObjectNr);
      CT::string variableName = dataObject->cdfVariable->name;
      //Show all requests
      
      for(it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
        CT::string ckey;ckey.print("%d%d%s",dX,dY,dataSource->nativeProj4.c_str());
        CImageDataWriter::ProjCacheInfo projCacheInfo = CImageDataWriter::GetProjInfo(ckey,drawImage,dataSource,imageWarper, dataSource->srvParams,dX,dY);
        
        if(projCacheInfo.isOutsideBBOX == false){
          CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource->srvParams,(filemapiterator->first).c_str());
          

          if(cdfObject->getVariableNE("forecast_reference_time")!=NULL){
            CDBDebug("IS REFERENCE TIME");
            CDF::Dimension *forecastRefDim = new CDF::Dimension();
            forecastRefDim->name = "forecast_reference_time";
            forecastRefDim->setSize(1);
            cdfObject->addDimension(forecastRefDim);
          }
          
          bool foundReferenceTime = false;
          for(size_t j=0;j<dataObject->cdfVariable->dimensionlinks.size();j++){
            if(dataObject->cdfVariable->dimensionlinks[j]->name.equals("forecast_reference_time")){foundReferenceTime=true;break;}
          }
          if(foundReferenceTime==false){
            CDF::Dimension *forecastRefDim = cdfObject->getDimensionNE("forecast_reference_time");
            if(forecastRefDim!=NULL){
              dataObject->cdfVariable->dimensionlinks.insert(dataObject->cdfVariable->dimensionlinks.begin()+dataObject->cdfVariable->dimensionlinks.size()-2,forecastRefDim);
            }
          }

         

          
      
                  
                  
          #ifdef CMakeJSONTimeSeries_DEBUG
          CDBDebug("Getting data variable [%s]",variableName.c_str());
          #endif    
          
          CDF::Variable *variable =cdfObject->getVariableNE(variableName.c_str());
          dataObject->cdfVariable= variable;
          if(variable == NULL){
            CDBError("Variable %s not found",variableName.c_str());
            throw (__LINE__);
          }
            
          for(size_t j=0;j<(filemapiterator->second)->requests.size();j++){
              
            Request *request=(filemapiterator->second)->requests[j];
#ifdef CMakeJSONTimeSeries_DEBUG                          
            CDBDebug("Reading file %s  for variable %s",(filemapiterator->first).c_str(), variable->name.c_str());
#endif            
            

            variable->freeData();
          
            for(int j=0;j<numberOfDims+2;j++){
              start[j]=0;
              count[j]=1;
              stride[j]=1;
            }
#ifdef CMakeJSONTimeSeries_DEBUG              
            CDBDebug("Querying raster location %d %d",projCacheInfo.imx,projCacheInfo.imy);
            CDBDebug("Querying raster dimIndex %d %d",dataSource->dimXIndex,dataSource->dimYIndex);
#endif
            start[dataSource->dimXIndex] = projCacheInfo.imx;
            start[dataSource->dimYIndex] = projCacheInfo.imy;

            for(int i=0;i<request->numDims;i++){
              
              int netcdfDimIndex = -1;
              CDataReader::DimensionType dtype = CDataReader::getDimensionType(cdfObject,request->dimensions[i]->name.c_str());
              if(dtype != CDataReader::dtype_reference_time){
                if(dtype==CDataReader::dtype_none){
                  CDBWarning("dtype_none for %s",dtype,request->dimensions[i]->name.c_str());
                }
                try{
                  netcdfDimIndex = variable->getDimensionIndex(request->dimensions[i]->name.c_str());
                }catch(int e){
                  //CDBError("Unable to find dimension [%s]",request->dimensions[i]->name.c_str());
                  if(dtype == CDataReader::dtype_reference_time){
                    CDBDebug("IS REFERENCE TIME %s",request->dimensions[i]->name.c_str());
        
                  }else{
                    CDBError("IS NOT REFERENCE TIME %s",request->dimensions[i]->name.c_str());
                    throw(__LINE__);
                  }
                }
                start[netcdfDimIndex]=request->dimensions[i]->start;
                count[netcdfDimIndex]=request->dimensions[i]->values.size();
  #ifdef CMakeJSONTimeSeries_DEBUG                
                CDBDebug("  request index: %d  netcdfdimindex %d  %s %d %d",i,netcdfDimIndex, request->dimensions[i]->name.c_str(),request->dimensions[i]->start,request->dimensions[i]->values.size());
  #endif              
               }
            }
#ifdef CMakeJSONTimeSeries_DEBUG              
            for(int i=0;i<numberOfDims+2;i++){
              CDBDebug("  %d %s [%d:%d]",i,"",start[i],count[i]);
            }
#endif       
  
  /*
            if(variable->getAttributeNE("scale_factor")!=NULL){
              readDataAsCDFDouble = true;
            }*/

            if(readDataAsCDFDouble){
              variable->setType(CDF_DOUBLE);
            }
            int status = variable->readData(variable->currentType,start,count,stride,true);
            
            if(status != 0){
                CDBError("Unable to read variable %s",variable->name.c_str());
                throw(__LINE__);
            }
          
            if(status == 0){
              /**
              * DataPostProc: Here our datapostprocessor comes into action!
              */
              for(size_t dpi=0;dpi<dataSource->cfgLayer->DataPostProc.size();dpi++){
                CServerConfig::XMLE_DataPostProc * proc = dataSource->cfgLayer->DataPostProc[dpi];
                //Algorithm ax+b:
                if(proc->attr.algorithm.equals("ax+b")){
                  double dfadd_offset = 0;
                  double dfscale_factor = 1;
                  
                  CT::string offsetStr = proc->attr.b.c_str();
                  dfadd_offset = offsetStr.toDouble();
                  CT::string scaleStr = proc->attr.a.c_str();
                  dfscale_factor = scaleStr.toDouble();
                  double *_data=(double*)variable->data;
                  for(size_t j=0;j<variable->getSize();j++){
                    //if(j%10000==0){CDBError("%d = %f",j,_data[j]);}
                    _data[j]=_data[j]*dfscale_factor+dfadd_offset;
                  }
                  //Convert the nodata type
                  dataSource->getDataObject(dataObjectNr)->dfNodataValue=dataSource->getDataObject(dataObjectNr)->dfNodataValue*dfscale_factor+dfadd_offset;
                }
                //Apply units:
                if(proc->attr.units.empty()==false){
                  dataSource->getDataObject(dataObjectNr)->setUnits(proc->attr.units.c_str());
                }
            
              }
              /* End of data postproc */
#ifdef CMakeJSONTimeSeries_DEBUG          
              CDBDebug("Read %d elements",variable->getSize());
              
              for(size_t j=0;j<variable->getSize();j++){
                CDBDebug("Data value %d is \t %f",j,((float*)variable->data)[j]);
              }
#endif              
              try{
                expandData(dataObject,variable,start,count,0,request,0);
              }catch(int e){
                CDBError("Error in expandData at line %d",e);
                throw(__LINE__);
              }
            }
            
          }
        }
      }
      
  
      try{
        CDBDebug("dataObjectNr: %d", dataObjectNr);
        createStructure(dataObject ,drawImage,imageWarper,dataSource,dX,dY,gfiStructure);
      }catch(int e){
        CDBError("Error in createStructure at line %d",e);
        throw(__LINE__);
      }
    }
    reader.close();
    
    #ifdef CMakeJSONTimeSeries_DEBUG
    CDBDebug("/makeRequests");
    #endif    

  }
  
  size_t size(){
    return fileInfoMap.size();
  }

  FileInfo *get(size_t index){
    typedef std::map<std::string ,FileInfo*>::iterator it_type_file;
    size_t s=0;
    for(it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
        if(s == index)return filemapiterator->second;
        s++;
    }
    return NULL;
  }
  

};
const char * UniqueRequests::className = "UniqueRequests";

int CMakeJSONTimeSeries::MakeJSONTimeSeries(CDrawImage *drawImage,CImageWarper *imageWarper,std::vector<CDataSource *>dataSources,int dataSourceIndex,int dX,int dY,CXMLParser::XMLElement *gfiStructure){
  CDataSource *dataSource=dataSources[dataSourceIndex];

  UniqueRequests uniqueRequest;
  /**
  * DataPostProc: Here our datapostprocessor comes into action!
  */
  for(size_t dpi=0;dpi<dataSource->cfgLayer->DataPostProc.size();dpi++){
    CServerConfig::XMLE_DataPostProc * proc = dataSource->cfgLayer->DataPostProc[dpi];
    //Algorithm ax+b:
    if(proc->attr.algorithm.equals("ax+b")){
      uniqueRequest.readDataAsCDFDouble = true;
      break;
    }
  }
          
  int numberOfDims = dataSource->requiredDims.size();
  int numberOfSteps = dataSource->getNumTimeSteps();
      



#ifdef CMakeJSONTimeSeries_DEBUG
  CDBDebug("1) /*Find all individual files*/");
#endif            

  for(int step=0;step<numberOfSteps;step++){
    dataSource->setTimeStep(step);
    for(int dimnr = 0;dimnr<numberOfDims;dimnr++){
      COGCDims *ogcDim=dataSource->requiredDims[dimnr];
      uniqueRequest.set(dataSource->getFileName(),ogcDim->netCDFDimName.c_str(),dataSource->getDimensionIndex(dimnr),dataSource->getDimensionValue(dimnr));
    }
  }
  
  //Sort
  try{
    uniqueRequest.sortAndAggregate();
  }catch(int e){
    CDBError("Error in sortAndAggregate at line %d",e);
    throw(__LINE__);
  }
  
  //Make requests
  try{
    uniqueRequest.makeRequests(drawImage,imageWarper,dataSource,dX,dY,gfiStructure);
  }catch(int e){
    CDBError("Error in makeRequests at line %d",e);
    throw(__LINE__);
  }

  


      

  return 0;
};
