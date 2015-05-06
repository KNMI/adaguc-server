#include "COpenDAPHandler.h"
#include "CRequest.h"
#include "CDBFactory.h"
const char * COpenDAPHandler::className = "COpenDAPHandler";

class VarInfo{
  public:
    class Dim{
    public:
      Dim(size_t start,size_t count, ptrdiff_t stride){
        this->start = start;
        this->count = count;
        this->stride = stride;
      }
      size_t start;
      size_t count;
      ptrdiff_t stride;
    };
  VarInfo(const char *name){
    this->name = name;
  }
  CT::string name;
  std::vector<Dim> dimInfo;
  
};

int COpenDAPHandler::HandleOpenDAPRequest(const char *path,const char *query,CServerParams *srvParam){
  printf("%s%c%c\n","Content-Type: text/plain",13,10);

  CDBDebug("OpenDAP Received [%s] [%s]",path,query);
  
  CT::string dapName = path+8;
  CT::string layerName  = dapName;
  CT::string pathQuery = "";
  bool isDDSRequest = false;
  bool isDASRequest = false;
  bool isDODRequest = false;
  
  int i = dapName.lastIndexOf(".dds");
  if(i != -1){
    layerName = dapName.substring(0,i);
    pathQuery = dapName.substring(i+4,-1);
    isDDSRequest = true;
  }else{
    int i = dapName.lastIndexOf(".das");
    if(i != -1){
      layerName = dapName.substring(0,i);
      pathQuery = dapName.substring(i+4,-1);
      isDASRequest = true;
    }else{
      int i = dapName.lastIndexOf(".dods");
      if(i != -1){
        layerName = dapName.substring(0,i);
        pathQuery = dapName.substring(i+5,-1);
        isDDSRequest = true;
        isDODRequest = true;
      }
    }
  }
  
  CDBDebug("Layername = %s",layerName.c_str());
  CDBDebug("pathQuery = %s",pathQuery.c_str());
  
  CDataSource *dataSource = new CDataSource ();
  bool foundLayer = false;
  
  for(size_t layerNo=0;layerNo<srvParam->cfg->Layer.size();layerNo++){
    CT::string intLayerName;
    srvParam->makeUniqueLayerName(&intLayerName,srvParam->cfg->Layer[layerNo]);
    if(intLayerName.equals(layerName.c_str())){
      if(dataSource->setCFGLayer(srvParam,srvParam->configObj->Configuration[0],srvParam->cfg->Layer[layerNo],intLayerName.c_str(),0)!=0){
        return 1;
      }
      foundLayer = true;
      break;
    }
  }
  
  if(foundLayer == false){
    delete dataSource;
    return 1;
  }
      
  CDBDebug("Found layer");
  
  if(dataSource->dLayerType==CConfigReaderLayerTypeDataBase||
      dataSource->dLayerType==CConfigReaderLayerTypeStyled)
  {
    //When this layer has no dimensions, we do not need to query 
    // When there are no dims, we can get the filename from the config
    if(dataSource->cfgLayer->Dimension.size()==0){
      
      if(CDataReader::autoConfigureDimensions(dataSource)!=0){
        CDBError("Unable to configure dimensions automatically");
        return 1;
      }
    }
    if(dataSource->cfgLayer->Dimension.size()!=0){
      if(CRequest::getDimValuesForDataSource(dataSource,srvParam)!=0){
        CDBError("Unable to fill in dimensions");
        return 1;
      }
    }else{
      //This layer has no dimensions, but we need to add one timestep with data in order to make the next code work.        
      CDirReader dirReader;
      if(CDBFileScanner::searchFileNames(&dirReader,dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter,NULL)!=0){
        CDBError("Could not find any filename");
        return 1; 
      }
      if(dirReader.fileList.size()==0){
        CDBError("dirReader.fileList.size()==0");return 1;
      }
      dataSource->addStep(dirReader.fileList[0]->fullName.c_str(),NULL);
      dataSource->getCDFDims()->addDimension("time","0",0);
    }
  }
  
  class DimQuery{
  public:
    static int getDimSize(CDataSource *dataSource, const char *name){
      CDBDebug("There are %d dims",dataSource->cfgLayer->Dimension.size());
      for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
        if(dataSource->cfgLayer->Dimension[d]->attr.name.equals(name)){
          CDBDebug(": %s",dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
          CT::string tableName;
          CT::string dim = dataSource->cfgLayer->Dimension[d]->attr.name.c_str();
                    
          try{
            tableName =  CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),dim.c_str(),dataSource);
          }catch(int e){
            CDBError("Unable to create tableName from '%s' '%s' '%s'",dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dim.c_str());
            return -1;
          }
          
          CDBDebug("tableName = %s",tableName.c_str());
          size_t dimSize = 0;
//           CT::string query;
//           query.print("select count(%s) from (select distinct %s from %s)temp",dim.c_str(),dim.c_str(),tableName.c_str());
//           CPGSQLDB * DB = dataSource->srvParams->getDataBaseConnection();
//           CDBStore::Store * dimSizeStore = DB->queryToStore(query.c_str());
//           if(dimSizeStore != NULL){
//             dimSize = dimSizeStore->getRecord(0)->get(0)->toInt();
//           }else{
//             CDBError("Query %s failed",query.c_str());
//           }
//           delete dimSizeStore;
          CDBDebug("Dimsize= %d",dimSize);
          return dimSize;
        }
      }
      return -1;
    }
    
    static int doDimQuery(){
    //(select distinct(time) from t20141023t161424734_6f2njpjcbnic9ykow48 order by time limit 1 offset 11)a;
      return 0;
    }
    
  };

  
  
  //Read the header!
  
  
  try{
  
   CDataReader reader;
    int status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
    if(status!=0){
      CDBError("Could not open file: %s",dataSource->getFileName());
      throw(__LINE__);
    }
    
    class CDFTypeToOpenDAPType{
    public:
      static CT::string getvar(const int type){
        CT::string rtype="unknown";
        if(type==CDF_NONE  )rtype=("CDF_NONE");
        if(type==CDF_BYTE  )rtype=("Byte");
        if(type==CDF_UBYTE )rtype=("Byte");
        if(type==CDF_CHAR  )rtype=("Byte");
        if(type==CDF_SHORT )rtype=("Int16");
        if(type==CDF_USHORT)rtype=("UInt16");
        if(type==CDF_INT   )rtype=("Int32");
        if(type==CDF_UINT  )rtype=("Uint32");
        if(type==CDF_FLOAT )rtype=("Float32");
        if(type==CDF_DOUBLE)rtype=("Float64");
        if(type==CDF_STRING)rtype=("String");
        return rtype;
      }
       static CT::string getatt(const int type){
        CT::string rtype="unknown";
        if(type==CDF_NONE  )rtype=("CDF_NONE");
        if(type==CDF_BYTE  )rtype=("Byte");
        if(type==CDF_UBYTE )rtype=("Byte");
        if(type==CDF_CHAR  )rtype=("String");
        if(type==CDF_SHORT )rtype=("Int16");
        if(type==CDF_USHORT)rtype=("UInt16");
        if(type==CDF_INT   )rtype=("Int32");
        if(type==CDF_UINT  )rtype=("Uint32");
        if(type==CDF_FLOAT )rtype=("Float32");
        if(type==CDF_DOUBLE)rtype=("Float64");
        if(type==CDF_STRING)rtype=("String");
        return rtype;
      }
    };
    
    CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
      
    for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
      //Check for the configured dimensions or scalar variables
      //1 )Is this a scalar?
      CDF::Variable *  dimVar = cdfObject->getVariableNE(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
      CDF::Dimension * dimDim = cdfObject->getDimensionNE(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
      
      
      if(dimVar!=NULL&&dimDim==NULL){
        //Check for scalar variable
        if(dimVar->dimensionlinks.size() == 0){
          CDBDebug("Found scalar variable %s with no dimension. Creating dim",dimVar->name.c_str());
          dimDim = new CDF::Dimension();
          dimDim->name = dimVar->name;
          dimDim->setSize(1);
          cdfObject->addDimension(dimDim);
          dimVar->dimensionlinks.push_back(dimDim);
        }

      }
    }
    
    
    if(isDDSRequest||isDODRequest){
     
      
      std::vector <VarInfo> selectedVariables;
      
      if(strlen(query)>0){
        CT::string q= query;
        CT::StackList<CT::string> items=q.splitToStack(",");
        for(size_t j=0;j<items.size();j++){
          //CDBDebug("Selected variable = \"%s\"",items[j].c_str());
          
          CT::StackList<CT::string> varsettings=items[j].splitToStack("[");
          CDBDebug("Selected variable = \"%s\"",varsettings[0].c_str());
          selectedVariables.push_back(VarInfo(varsettings[0].c_str()));
          if(varsettings.size() > 1){
            for(size_t d=1;d<varsettings.size();d++){
              varsettings[d].replaceSelf("]","");
              
              
              size_t count = 1;
              size_t stride = 1;
              
              size_t start = varsettings[d].toInt();
              CDBDebug("DIMINFO: %d,%s",start,varsettings[d].c_str());
              selectedVariables.back().dimInfo.push_back(VarInfo::Dim(start,count,stride));
            }
          }
          
        }
      }
      
    
      
      if(selectedVariables.size() == 0){
        for(size_t j=0;j<cdfObject->variables.size();j++){
          selectedVariables.push_back(VarInfo(cdfObject->variables[j]->name.c_str()));
        }
      }
      

      

      
      
      CT::string output = "";
      output.concat("Dataset {\n");
      for(size_t i=0;i<selectedVariables.size();i++){
        for(size_t j=0;j<cdfObject->variables.size();j++){
          CDF::Variable *v = cdfObject->variables[j];
          if( selectedVariables[i].name.equals(&v->name)){
            output.printconcat("    %s ",CDFTypeToOpenDAPType::getvar(v->getType()).c_str());
            output.concat(v->name.c_str());
            
            //This is the variable it is all about:            
            if(v == dataSource->getDataObject(0)->cdfVariable){
              //Add the dimensions which are not in the cdfVariable, but ara externally configured.
              for(size_t j=0;j<dataSource->cfgLayer->Dimension.size();j++){
                CT::string dimname = dataSource->cfgLayer->Dimension[j]->attr.name;
                try{
                  v->getDimension(dimname.c_str())->getSize();
                }catch(int e){
                  int dimSize = DimQuery::getDimSize(dataSource,dimname.c_str());
                  if(selectedVariables[i].dimInfo.size() == dataSource->cfgLayer->Dimension.size()+2){
                    dimSize = selectedVariables[i].dimInfo[j].count;
                  }
                  output.printconcat("[%s = %d]",dimname.c_str(),dimSize);
                }
              }
            }
              
            
            for(size_t j=0;j<v->dimensionlinks.size();j++){
              int size = v->dimensionlinks[j]->getSize();
              int dimSize = DimQuery::getDimSize(dataSource,v->dimensionlinks[j]->name.c_str());
              if(dimSize > 0){
                CDBDebug("DimSize = %d",dimSize);
                size = dimSize;
              }
  
              if(selectedVariables[i].dimInfo.size() == v->dimensionlinks.size()){
                size = selectedVariables[i].dimInfo[j].count;
              }
              output.printconcat("[%s = %d]",v->dimensionlinks[j]->name.c_str(),size);
            }
            
            output.concat(";\n");
          }
        }
      }
      output.printconcat("} %s;\n",layerName.c_str());
      
      printf("%s\n",output.c_str());
      fflush(stdout);
      if(isDODRequest){
        printf("\nData:\n"); 
        for(size_t i=0;i<selectedVariables.size();i++){
          for(size_t j=0;j<cdfObject->variables.size();j++){
            CDF::Variable *v = cdfObject->variables[j];
            if( selectedVariables[i].name.equals(&v->name)){
              if(selectedVariables[i].dimInfo.size() == v->dimensionlinks.size()){
                size_t start[ v->dimensionlinks.size()];
                size_t count[ v->dimensionlinks.size()];
                ptrdiff_t stride[ v->dimensionlinks.size()];
                for(size_t k=0;k< v->dimensionlinks.size();k++){
                  start[k] = selectedVariables[i].dimInfo[k].start;
                  count[k] = selectedVariables[i].dimInfo[k].count;
                  stride[k] = selectedVariables[i].dimInfo[k].stride;
                }
                v->readData((CDFType)v->getType(),start,count,stride);
              }else{
                v->readData((CDFType)v->getType());
              }
              size_t typeSize = CDF::getTypeSize(v->getType());
              unsigned char *data = (unsigned char*)v->data;
              int a = v->getSize();
              fwrite(&a,4,1,stdout);
              fwrite(&a,4,1,stdout);
              for(size_t d=0;d<v->getSize();d++){
                for(size_t e=0;e<typeSize;e++){
                  putc(data[d*typeSize+(typeSize-1)-e],stdout);
                }
              }
              
            }
          }
        }
        fflush(stdout);
      }
     
    }
    
    if(isDASRequest){
      CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
      CT::string output = "";
      output.concat("Attributes {\n");
      for(size_t j=0;j<cdfObject->variables.size();j++){
        CDF::Variable *v = cdfObject->variables[j];
        output.printconcat("    %s {\n",v->name.c_str());
        for(size_t j=0;j<v->attributes.size();j++){
          output.printconcat("        %s %s ",CDFTypeToOpenDAPType::getatt(v->attributes[j]->type).c_str(),v->attributes[j]->name.c_str());
          if(v->attributes[j]->type == CDF_CHAR){
            output.concat("\"");
            CT::string s = v->attributes[j]->getDataAsString().c_str();
            //s.encodeURLSelf();
            //s.replaceSelf(":","");
            //s.replaceSelf("[","");
            output.concat(s.c_str());
            if(v->attributes[j]->type == CDF_CHAR)output.concat("\"");
          }else{
            output.concat(v->attributes[j]->getDataAsString().c_str());
          }
          output.printconcat(";\n",v->attributes[j]->name.c_str());
        }
        output.concat("    }\n");
      }
      output.printconcat("}");
      printf("%s\n",output.c_str());
    }

    
    reader.close();
  }catch(int e){
  }
  
  
  delete dataSource;

  return 0;
}
