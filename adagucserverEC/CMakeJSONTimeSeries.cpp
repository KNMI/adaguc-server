#include "CMakeJSONTimeSeries.h"
#include "CImageDataWriter.h"
const char * CMakeJSONTimeSeries::className = "CMakeJSONTimeSeries";




template <class T> class MyUnorderedSet{
public:
  std::vector<T> array;
  T insert(T i){
    for(size_t j=0;j<array.size();j++){
      if(array[j] == i){
        return j;
      }
    }
    array.push_back(i);
    return -1;
  }
  size_t size(){
    return array.size();
  }
  T get(size_t i){
    return array[i];
  }
};

class UniqueRequests{
public:
 
  class FileInfo{
  public:
      
    class DimInfo{
    public:
      CT::string dimname;
      int start,count;
      std::vector<CT::string*> dimensionKeys;
      
    };
    class Request{
    public:
      std::vector<DimInfo> dimsetting;
    };
    CT::string file;
    std::set<std::string>dimset;
    

    size_t size(){
      return dimInfoMap.size();
    }

    DimInfo *get(size_t index){
      typedef std::map<std::string ,DimInfo*>::iterator it_type_dim;
      size_t s=0;
      for(it_type_dim diminfomapiterator = dimInfoMap.begin(); diminfomapiterator != dimInfoMap.end(); diminfomapiterator++) {
          if(s == index)return diminfomapiterator->second;
          s++;
      }
      return NULL;
    }
    std::vector<DimInfo *>get(const char *name){
      typedef std::map<std::string ,DimInfo*>::iterator it_type_dim;
      std::vector<DimInfo *>d;
      for(it_type_dim diminfomapiterator = dimInfoMap.begin(); diminfomapiterator != dimInfoMap.end(); diminfomapiterator++) {
          if(diminfomapiterator->second->dimname.equals(name))d.push_back(diminfomapiterator->second);
      }
      return d;
    }
    
    void go(std::set<std::string>::iterator dimsetIterator,int depth,size_t numdims,CT::string path,size_t *start,size_t *count,std::vector<CT::string*> *dimensionKeys,CT::string *dimnames,std::vector<Request> *requestsToDo){
     
      std::vector<DimInfo *> dimInfos = get(dimsetIterator->c_str());
      dimsetIterator++;
//       if(dimsetIterator==dimset.end()){
//         return;
//       }
      CT::string d = "";
      for(int j=0;j<depth+1;j++){
        d.concat("->");
      }
      
      for(size_t j=0;j<dimInfos.size();j++){
        CT::string newPath = path;
        
        start[depth] = dimInfos[j]->start;
        count[depth] = dimInfos[j]->count;
        dimnames[depth] = dimInfos[j]->dimname;
        dimensionKeys[depth] = dimInfos[j]->dimensionKeys;
        newPath.printconcat("->%s[%d/%d:%d %d]",dimInfos[j]->dimname.c_str(),depth,dimInfos[j]->start,dimInfos[j]->count,dimensionKeys[depth].size());
        
        
        if(dimsetIterator!=dimset.end()){
          
          go(dimsetIterator,depth+1,numdims,newPath,start,count,dimensionKeys,dimnames,requestsToDo);
        }else{
          
          Request request;
          
          for(size_t j=0;j<numdims;j++){
            DimInfo d;
            d.dimname = dimnames[j];
            d.start = start[j];
            d.count = count[j];
            d.dimensionKeys = dimensionKeys[j];
            request.dimsetting.push_back(d);
          }
          
          requestsToDo->push_back(request);
          //CDBDebug("%s",newPath.c_str()); 
        }
        
      }
      
      
      
    }
    
    std::vector<Request> getRequestToDo(){
      std::vector<Request> requestsToDo;
      size_t numberOfDims = dimset.size();
      size_t start[numberOfDims],count[numberOfDims];
      std::vector<CT::string*> dimensionKeys[numberOfDims];
      CT::string dimnames[numberOfDims];
      std::set<std::string>::iterator dimsetIterator = dimset.begin();
      go(dimsetIterator,0,numberOfDims,"BASE",start,count,dimensionKeys,dimnames,&requestsToDo);
     
      return requestsToDo;
    }
    
    
    //Map on dimname */
    std::map <std::string ,DimInfo*> dimInfoMap;
    ~FileInfo(){
      typedef std::map<std::string ,DimInfo*>::iterator it_type_dim;
      for(it_type_dim it = dimInfoMap.begin(); it != dimInfoMap.end(); it++) {
        delete it->second;
      }
    }
  };
  //Map on filename */
  std::map <std::string ,FileInfo*> fileInfoMap;
  
  UniqueRequests(){
  }
  ~UniqueRequests(){
    typedef std::map<std::string ,FileInfo*>::iterator it_type_file;
    for(it_type_file filemapiterator = fileInfoMap.begin(); filemapiterator != fileInfoMap.end(); filemapiterator++) {
        delete filemapiterator->second;
    }
  }
  
  
  void set(const char*filename,const char*dimname,size_t start,size_t count, std::vector<CT::string*> dimensionKeys){
    //CDBDebug("%s[%d:%d]",dimname,start,count);
    
    //Find or create a fileinfo object
    FileInfo *f = NULL;
    std::map<std::string,FileInfo*>::iterator itf=fileInfoMap.find(filename);
    if(itf!=fileInfoMap.end()){f = (*itf).second;}else{f = new FileInfo();fileInfoMap.insert(std::pair<std::string,FileInfo*>(filename,f));}
    
    FileInfo::DimInfo *d = NULL;
    CT::string dimkey = dimname;
    dimkey.printconcat("%d:%d",start,count);
    std::map<std::string, FileInfo::DimInfo*>::iterator itd=f->dimInfoMap.find(dimkey.c_str());
    if(itd!=f->dimInfoMap.end()){d = (*itd).second;}else{d = new FileInfo::DimInfo();f->dimInfoMap.insert(std::pair<std::string, FileInfo::DimInfo*>(dimkey.c_str(),d));}
    
    d->dimname = dimname;
    f->file= filename;
    f->dimset.insert(dimname);
    d->start = start;
    d->count = count;
    d->dimensionKeys = dimensionKeys;
    
    //CDBDebug("dimensionKeys size %d",dimensionKeys.size());
    
    
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
  
//   UniqueRequests(CT::string file,CT::string varname,CT::string dimname, int start, int count,std::vector<CT::string*> dimensionKeys){
//     this->file = file;
//     this->dimname = dimname;
//     this->start = start;
//     this->count = count;
//     this->varname = varname;
//     this->dimensionKeys = dimensionKeys;
//   }
};


int CMakeJSONTimeSeries::MakeJSONTimeSeries(CDrawImage *drawImage,CImageWarper *imageWarper,std::vector<CDataSource *>dataSources,int dataSourceIndex,int dX,int dY,CXMLParser::XMLElement *gfiStructure){
   
  
    CDataSource *dataSource=dataSources[dataSourceIndex];
                bool readDataAsCDFDouble = false;
          
          /**
          * DataPostProc: Here our datapostprocessor comes into action!
          */
          for(size_t dpi=0;dpi<dataSource->cfgLayer->DataPostProc.size();dpi++){
            CServerConfig::XMLE_DataPostProc * proc = dataSource->cfgLayer->DataPostProc[dpi];
            //Algorithm ax+b:
            if(proc->attr.algorithm.equals("ax+b")){
              readDataAsCDFDouble = true;
              break;
            }
          }
          
      /*Gather dimension information*/
      int numberOfDims = dataSource->requiredDims.size();
      //int dimUniqueSize[numberOfDims];
      int uniqueMultiplier[numberOfDims];
      int noTimeSteps = dataSource->getNumTimeSteps();
      for(int i=0;i<numberOfDims;i++){
        COGCDims *dims = dataSource->requiredDims[i];
        CDBDebug("Datasource required dim: %d [ogc:%s val:%s nc:%s numunique:%d]",i,dims->name.c_str(),dims->value.c_str(),dims->netCDFDimName.c_str(),dims->uniqueValues.size());
        //dimUniqueSize[i] =  dataSource->requiredDims[i]->uniqueValues.size();
        uniqueMultiplier[i]=1;
      }
//       for(int i=0;i<numberOfDims;i++){
//         for(int j=0;j<i;j++){
//           uniqueMultiplier[j]*=dimUniqueSize[i];
//         }
//       }
      
      #ifdef CIMAGEDATAWRITER_DEBUG
      for(int j=0;j<numberOfDims;j++){
        CDBDebug("Multiplier %d:%d = %d",j,dimUniqueSize[j],uniqueMultiplier[j]);
      }
      #endif

      /*Find all individual files*/
      std::set<std::string> fileSet;  
      std::set<std::string>::iterator fileSetIterator;
      
      for(int step=0;step<noTimeSteps;step++){
        dataSource->setTimeStep(step);
        fileSet.insert(dataSource->getFileName());
      }
      
      CDBDebug("Nr of files: %d",fileSet.size());
      int numberOfFiles = fileSet.size();
      
      class CreateGDIDataStructure{
        public:
        static void createGDIDataStructure(int currentDim,int numberOfDims,std::vector<UniqueRequests::FileInfo::DimInfo> *dimInfo,int rank[],size_t *index,CXMLParser::XMLElement *dataStructure,CDataSource::DataObject *dataObject){
          if(currentDim<numberOfDims){
            //CDBDebug("currentDim %d/%d",currentDim,numberOfDims);
            int lookupDim = currentDim;//dimLookUp[currentDim];
            UniqueRequests::FileInfo::DimInfo* dimInfoPointer = &(*dimInfo)[lookupDim];
       
            for(int i=0;i<dimInfoPointer->count;i++){
              index[lookupDim] = i;
             //int newindex = index+(i*multiplier);
              CT::string dimindexvalue = dimInfoPointer->dimensionKeys[i]->c_str();
              
              if(dimindexvalue.length()==19){
                if(dimindexvalue.charAt(10)==32){
                dimindexvalue.setChar(10,'T' );
                dimindexvalue.concat("Z");
                }
              }
              //dimindexvalue.concat(dimInfoPointer->dimname.c_str());
              //CDBDebug("%s",dimindexvalue.c_str());
              CXMLParser::XMLElement *el = NULL;
              try{
                el = dataStructure->get(dimindexvalue.c_str());
              }catch(int e){
                dataStructure->add(CXMLParser::XMLElement(dimindexvalue.c_str()));
                el = dataStructure->getLast();
              }
//               dataStructure->add(CXMLParser::XMLElement(dimindexvalue.c_str()));
//               el = dataStructure->getLast();
             // CDBDebug("c");
              createGDIDataStructure(currentDim+1,numberOfDims,dimInfo,rank,index,el,dataObject);
              
            }
            
          }else{
          //     CDBDebug("d");
            CT::string value="nodata";
            
//             for(int j=0;j<numberOfDims;j++){
//               if(rank[j]!=-1){
//                 value.printconcat(":%d",index[rank[j]]);
//               }
//             }
//             value.printconcat(" -- ");
            int a = 1;
            int dataIndex = 0;
            for(int j=numberOfDims;j>=0;j--){
              if(rank[j]!=-1){
                //value.printconcat(":%d", (*dimInfo)[rank[j]].count);
                dataIndex+=index[rank[j]]*a;
               //value.printconcat("[%d]", a);
                a = a *((*dimInfo)[rank[j]].count);
               }
            }
              //CDBDebug("d %d %d",dataIndex,dataObject->cdfVariable->data);
//             value.printconcat(" -- ");
//             for(int j=0;j<numberOfDims;j++){
//                if(rank[j]!=-1){
//               value.printconcat(":%d",rank[j]);
//                }
//             }
//             value.printconcat("== %d",dataIndex);
            //CDBDebug("Index = %d",index);
            //double pixel = ((float*)data)[dataIndex];
              if(dataObject->cdfVariable->data!=NULL){
            double pixel=CImageDataWriter::convertValue(dataObject->cdfVariable->getType(),dataObject->cdfVariable->data,dataIndex);
            //      CDBDebug("d");
            if((pixel!=dataObject->dfNodataValue&&dataObject->hasNodataValue==true&&pixel==pixel)||dataObject->hasNodataValue==false){
              if(dataObject->hasStatusFlag){
                //Add status flag
                CT::string flagMeaning;
                CDataSource::getFlagMeaningHumanReadable(&flagMeaning,&dataObject->statusFlagList,pixel);
                value.print("%s (%d)",flagMeaning.c_str(),(int)pixel);
               //   CDBDebug("d");
              }else{
                //    CDBDebug("d");
                //Add raster value
                //char szTemp[1024];
                //floatToString(szTemp,1023,pixel);
                
                value.print("%f",pixel);//=szTemp;
              }
            }
            }
            //   CDBDebug("e");
            dataStructure->setValue(value.c_str());
          }

        }
      };
        
      /*Find all individual dimension sets*/
      std::vector<CT::string*> uniqueValues[numberOfDims][numberOfFiles];
      MyUnorderedSet <int>dimSets[numberOfDims][numberOfFiles];
      int timeDimIndex = -1;
      for(int dimnr = 0;dimnr<numberOfDims;dimnr++){
        //TODO: uniqueMultiplier sometimes too small, causes many unnecessary iterations
        for(int step=0;step<noTimeSteps;step = step + uniqueMultiplier[dimnr]){
          dataSource->setTimeStep(step);
          CCDFDims *dims = dataSource->getCDFDims();
          fileSetIterator=fileSet.find(dataSource->getFileName());
          int filenr = std::distance(fileSet.begin(),fileSetIterator);
          #ifdef CIMAGEDATAWRITER_DEBUG
          CDBDebug("dimnr, filenr %s %d [%d][%d] = %d",dims->getDimensionName(dimnr),step,dimnr,filenr,dims->getDimensionIndex(dimnr));
          #endif
          int r = dimSets[dimnr][filenr].insert(dims->getDimensionIndex(dimnr));
          if(r==-1){
            /*Requested dimension value not available, add it:*/
            uniqueValues[dimnr][filenr].push_back(dims->getDimensionValuePointer(dimnr));
          }
          if (timeDimIndex == -1 && dims->isTimeDimension(dimnr)) {
            timeDimIndex = dimnr;
          }
        }
      }


      /*Aggregate individual dimension requests to dimension ranges per dimension, e.g. time index (1,2,3,4,5,6,11,12,13) becomes (1-6,11-13)*/
      UniqueRequests uniqueRequest;
      for(fileSetIterator = fileSet.begin();fileSetIterator!=fileSet.end();++fileSetIterator){
        #ifdef CIMAGEDATAWRITER_DEBUG
        CDBDebug("For file %s:",fileSetIterator->c_str());
        #endif
        int filenr = std::distance( fileSet.begin(),fileSetIterator);
        for(int dimnr = 0;dimnr<numberOfDims;dimnr++){
          #ifdef CIMAGEDATAWRITER_DEBUG
          //CDBDebug("--Indices for dim %s:", dataSource->requiredDims[dimnr]->name.c_str());
          #endif
          int previousDif = -1 , start = -1,count = -1;
          //bool hasChanged = false;
          std::vector<CT::string*> dimensionKeys;
          //CDBDebug("SIze: %d",dimSets[dimnr][filenr].size());
          for(size_t iter=0;iter<dimSets[dimnr][filenr].size();iter++){
            int index = dimSets[dimnr][filenr].get(iter);
            //CDBDebug("Index = %d",index);
            if(previousDif == -1){

              start = index;
              count = index;
              //hasChanged=true;
              //CDBDebug("%d %d",start,count);
            }
            if(previousDif!=-1){
              if(index - previousDif != 1){
                count = (count -start)+1;
                CDBDebug("For dimension %d:[%d:%d]",dimnr,start,count);
                //uniqueRequestList[dimnr].list.push_back(UniqueRequests(fileSetIterator->c_str(),dataSource->getDataObject(0)->variableName.c_str(),dataSource->requiredDims[dimnr]->name.c_str(),start,count,dimensionKeys));
                uniqueRequest.set(fileSetIterator->c_str(),dataSource->requiredDims[dimnr]->name.c_str(),start,count,dimensionKeys);
                //uniqueRequestList.add();
                //hasChanged=false;
                dimensionKeys.clear();
                previousDif = -2;
                start = index;
                //CDBDebug("%d %d",start,count);
              }
            }
          
            previousDif = index;
            count = index;
            
            //CDBDebug("ITer = %d %d %s",iter,index,uniqueValues[dimnr][filenr][iter]->c_str());
      
            dimensionKeys.push_back(uniqueValues[dimnr][filenr][iter]);
          }
          //if(hasChanged == true){
            count = (count -start)+1;
            //CDBDebug("For dimension %d:%s->%s[%d:%d]",dimnr,dataSource->getDataObject(0)->variableName.c_str(),dataSource->requiredDims[dimnr]->name.c_str(),start,count);
            uniqueRequest.set(fileSetIterator->c_str(),dataSource->requiredDims[dimnr]->name.c_str(),start,count,dimensionKeys);
            //uniqueRequestList[dimnr].list.push_back(UniqueRequests(fileSetIterator->c_str(),dataSource->getDataObject(0)->variableName.c_str(),dataSource->requiredDims[dimnr]->name.c_str(),start,count,dimensionKeys));
            
          //}
            //CDBDebug("uniqueRequestList size = %d",uniqueRequestList[dimnr].list.size());
        }
        
      }
      
     
//       int dimLookUp[numberOfDims];
// 
//       int endIndex = 0;//numberOfDims-1;
//       for(int j=0;j<numberOfDims;j++){
//         dimLookUp[j]=j;
//       }
//       /*Order time dimension as latest dimension (swap it with another one */
//       if(timeDimIndex != -1){
//         if(timeDimIndex != endIndex){
//           int a = dimLookUp[timeDimIndex];
//           int b = dimLookUp[endIndex];
//           dimLookUp[timeDimIndex] = b;
//           dimLookUp[endIndex] = a;
//         }
//       }

      
      CDataReader reader;
      reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
      
      for(size_t dataObjectNr=0;dataObjectNr<dataSource->dataObjects.size();dataObjectNr++){
        CDataSource::DataObject *dataObject = dataSource->getDataObject(dataObjectNr);
        CXMLParser::XMLElement *layerStructure = gfiStructure->add("root");
        layerStructure->add(CXMLParser::XMLElement("name", dataSource->getLayerName()));
        
        /* Add metadata */
        CT::string standardName = dataObject->variableName.c_str();
        CDF::Attribute * attr_standard_name=dataObject->cdfVariable->getAttributeNE("standard_name");
        if(attr_standard_name!=NULL){
          standardName = attr_standard_name->toString();
        }
    
        layerStructure->add(CXMLParser::XMLElement("standard_name",standardName.c_str()));
        layerStructure->add(CXMLParser::XMLElement("units",dataObject->units.c_str()));

        CT::string ckey;
        ckey.print("%d%d%s",dX,dY,dataSource->nativeProj4.c_str());
        CImageDataWriter::ProjCacheInfo projCacheInfo = CImageDataWriter::GetProjInfo(ckey,drawImage,dataSource,imageWarper, dataSource->srvParams,dX,dY);
        CXMLParser::XMLElement point("point");
        point.add(CXMLParser::XMLElement("SRS", "EPSG:4326"));
        CT::string coord;
        coord.print("%f,%f", projCacheInfo.lonX, projCacheInfo.lonY);
        point.add(CXMLParser::XMLElement("coords", coord.c_str()));
        layerStructure->add(point);
      
      
        
      //  if(projCacheInfo.isOutsideBBOX == false){
          
          CT::string variableName = dataObject->cdfVariable->name;
          
            

          /* Now do all the requests */

          for(size_t uniqueRequestNr = 0;uniqueRequestNr<uniqueRequest.size();uniqueRequestNr++){
            UniqueRequests::FileInfo *fileInfo = uniqueRequest.get(uniqueRequestNr);
              std::vector<UniqueRequests::FileInfo::Request> requests = fileInfo->getRequestToDo();
              if(uniqueRequestNr == 0){
//                 CXMLParser::XMLElement *dims = 
                for(int dimNr = 0;dimNr<numberOfDims;dimNr++){
                  layerStructure->add(CXMLParser::XMLElement("dims",requests[0].dimsetting[dimNr].dimname.c_str()));
                  //CT::string dimstring;
//                   //dimstring.print("%d",requests[0].dimsetting[dimNr].count);
//                   CXMLParser::XMLElement *dim=dims->add(requests[0].dimsetting[dimNr].dimname.c_str());
                  //dim->add("count",dimstring.c_str());
                  //layerStructure->add(dims);
                }
              }
                  
              CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource->srvParams,fileInfo->file.c_str());
              reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
              
              CDF::Variable *variable =cdfObject->getVariableNE(variableName.c_str());
             dataObject->cdfVariable= variable;
              if(variable == NULL){
                CDBError("Variable %s not found",variableName.c_str());
                throw (__LINE__);
              }
              size_t start[numberOfDims+2],count[numberOfDims+2];ptrdiff_t stride[numberOfDims+2];int rank[numberOfDims+2];
            
              for(int j=0;j<numberOfDims+2;j++){
                start[j]=0;
                count[j]=1;
                stride[j]=1;
                rank[j]=-1;
              }
              for(size_t requestNr=0;requestNr<requests.size();requestNr++){
              
            //TODO CHECK::: numberOfDims
              
                for(int dimNr = 0;dimNr<numberOfDims;dimNr++){

                  int netcdfDimIndex = 0;
                  try{
                    CT::string netCDFDimName = requests[requestNr].dimsetting[dimNr].dimname.c_str();
                    for(size_t i=0;i<dataSource->requiredDims.size();i++){
                      COGCDims *dims = dataSource->requiredDims[i];
                      if(dims->name.equals(requests[requestNr].dimsetting[dimNr].dimname.c_str())){
                        netCDFDimName = dims->netCDFDimName; 
                        break;
                      }
                    }
                    netcdfDimIndex = variable->getDimensionIndex(netCDFDimName.c_str());
  //                  CDBDebug("Dimension %d%d/[%s] has index %d",requestNr,dimNr,requests[requestNr].dimsetting[dimNr].dimname.c_str(),netcdfDimIndex);
                    start[netcdfDimIndex]=requests[requestNr].dimsetting[dimNr].start;
                    count[netcdfDimIndex]=requests[requestNr].dimsetting[dimNr].count;
                    rank[netcdfDimIndex]=dimNr;
                  }catch(int e){
                  // CDBWarning("Dimension %d%d/[%s] not found",requestNr,dimNr,requests[requestNr].dimsetting[dimNr].dimname.c_str());
                  }
                
                  
                }
                start[dataSource->dimXIndex] = projCacheInfo.imx;
                start[dataSource->dimYIndex] = projCacheInfo.imy;
                variable->freeData();
  //               CDBDebug("Request NR %d/%d",requestNr,requests.size());
  //               for(size_t j=0;j<variable->dimensionlinks.size();j++){
  //                 CDBDebug("[start][count] for %s: [%d][%d]",variable->dimensionlinks[j]->name.c_str(),start[j],count[j]);
  //               }
  //               
  //               CDBDebug("Reading data %s",fileInfo->file.c_str());
  //               for(int j=0;j<numberOfDims;j++){
  //                 for(size_t i=0;i<requests[requestNr].dimsetting[j].count;i++){
  //                   CDBDebug("%d,%s, %s",requests[requestNr].dimsetting[j].start,requests[requestNr].dimsetting[j].dimname.c_str(),requests[requestNr].dimsetting[j].dimensionKeys[i]->c_str());
  //                 }
  //                 CDBDebug("%s->[%d:%d]",requests[requestNr].dimsetting[j].dimname.c_str(),requests[requestNr].dimsetting[j].start,requests[requestNr].dimsetting[j].count);
  //               }
                
                int status = 0;
                if(projCacheInfo.isOutsideBBOX == false){
  //                       CDBDebug("Reading %s",variable->name.c_str());
  //                     for(size_t j=0;j<variable->dimensionlinks.size();j++){
  //                       CDBDebug("[start][count] for %s: [%d][%d]",variable->dimensionlinks[j]->name.c_str(),start[j],count[j]);
  //                     }
                  CDBDebug("Read");
                  
                  //If we have a data postproc, we want to have the data in CDF_DOUBLE format
                
                  
                  if(readDataAsCDFDouble){
                    variable->setType(CDF_DOUBLE);
                  }
                  
                  
                  status = variable->readData(variable->currentType,start,count,stride,true);
                  CDBDebug("Done");
                  if(variable->data == NULL){
                    status = 1;
                  }
                  if(dataObject->cdfVariable->data == NULL){
                    status = 1;
                  }
                  
                }else{
                  size_t imageSize=1;
                  for(int j=0;j<numberOfDims;j++){
                    imageSize*=count[j];
                  }
                  CDF::allocateData(CDF_DOUBLE,&variable->data,imageSize);
                  variable->setType(CDF_DOUBLE);
                  for(size_t j=0;j<imageSize;j++){
                    ((double*)variable->data)[j]=NAN;
                  }
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
                      dataSource->getDataObject(dataObjectNr)->units=proc->attr.units.c_str();
                    }
                
                  }
                  /* End of data postproc */
                    
                    
                    CXMLParser::XMLElement *dataStructure= NULL;
                      try{
                      dataStructure = layerStructure->get("data");
                    }catch(int e){
                      layerStructure->add(CXMLParser::XMLElement("data"));
                      dataStructure = layerStructure->getLast();
                    }

                    //CXMLParser::XMLElement *dataStructure = layerStructure->add("data");
                    size_t index[numberOfDims];
                    //CDBDebug("Create GDI");
                    CreateGDIDataStructure::createGDIDataStructure(0,numberOfDims,&requests[requestNr].dimsetting,rank,index,dataStructure,dataObject);
                  }else{
                      CDBError("Unable to read variable %s",variable->name.c_str());
                      for(size_t j=0;j<variable->dimensionlinks.size();j++){
                        CDBError("[start][count] for %s: [%d][%d]",variable->dimensionlinks[j]->name.c_str(),start[j],count[j]);
                      }
                      return 1;
                  }
                
              }
              //CDBDebug("%s[%d:%d]",fileInfo->dimname.c_str(),dimInfo->start,dimInfo->count);
              
            }
        }
          
//         for(size_t requestNr = 0;requestNr<requestsToDo.requestlist.size();requestNr++){
//         
//           UniqueRequests u = requestsToDo.requestlist[requestNr];
//           
//           CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource->srvParams,u.file.c_str());
//           CDF::Variable *variable = cdfObject->getVariableNE(u.varname.c_str());
//           if(variable == NULL){
//             CDBError("Variable %s not found",u.varname.c_str());
//             throw (__LINE__);
//           }
//           size_t start[numberOfDims+2],count[numberOfDims+2];
//           ptrdiff_t stride[numberOfDims+2];
//           std::vector<CT::string*> dimensionKeys[numberOfDims];  
//           for(int j=0;j<numberOfDims+2;j++){
//             start[j]=0;
//             count[j]=1;
//             stride[j]=1;
//           }
//           for(int dimNr = 0;dimNr<numberOfDims;dimNr++){
//             //UniqueRequests u = requestsToDo.requestlist[requestNr].dimlist[dimNr];
//             dimensionKeys[dimNr]=u.dimensionKeys;
//             int netcdfDimIndex = 0;
//             try{
//               CT::string netCDFDimName = u.dimname;
//               
//               for(size_t i=0;i<dataSource->requiredDims.size();i++){
//                 COGCDims *dims = dataSource->requiredDims[i];
//                 //CDBDebug("%d: %s == %s",i,dims->name.c_str(),dims->netCDFDimName.c_str());
//                 
//                 
//                 if(dims->name.equals(u.dimname.c_str())){
//                   netCDFDimName = dims->netCDFDimName; 
//                  
//                 }
//               }
//               #ifdef CIMAGEDATAWRITER_DEBUG
// //              CDBDebug("for ogcname %s: netCDFDimName == %s",u.dimname.c_str(),netCDFDimName.c_str());
//               #endif
//               netcdfDimIndex = variable->getDimensionIndex(netCDFDimName.c_str());
//               start[netcdfDimIndex]=u.start;
//               count[netcdfDimIndex]=u.count;
//               stride[netcdfDimIndex]=1;
//             }catch(int e){
//               //CDBWarning("Dimension %s not found",u.dimname.c_str());
//             }
//             #ifdef CIMAGEDATAWRITER_DEBUG
// //            CDBDebug("R%d/D%d: %s %s|%s=[%d:%d] %d",requestNr,dimNr,u.file.c_str(),u.varname.c_str(),u.dimname.c_str(),u.start,u.count,u.dimensionKeys.size());
//             #endif
//           }
//           //CDBDebug("Reading %d/%d ",requestNr,requestsToDo,
//           if(projCacheInfo.isOutsideBBOX == false){
//             start[dataSource->dimXIndex] = projCacheInfo.imx;
//             start[dataSource->dimYIndex] = projCacheInfo.imy;
//             variable->freeData();
//             int status = variable->readData(CDF_DOUBLE,start,count,stride);
//             if(status == 0){
//               CreateGDIDataStructure::createGDIDataStructure(0,numberOfDims,start,count,dimLookUp,((double*)variable->data),0,dimensionKeys,dataStructure);
//             }else{
//                 CDBError("Unable to read variable %s",variable->name.c_str());
//                 for(size_t j=0;j<variable->dimensionlinks.size();j++){
//                   CDBError("[start][count] for %s: [%d][%d]",variable->dimensionlinks[j]->name.c_str(),start[j],count[j]);
//                 }
//                 return 1;
//             }
//           }
//         }
//       }else{
//         CDBDebug("OUTSIDE BBOX");
//       }
      return 0;
};
