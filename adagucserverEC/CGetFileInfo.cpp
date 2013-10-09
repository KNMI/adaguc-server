#include "CGetFileInfo.h"
#include "CDFObjectStore.h"

const char *CGetFileInfo::className="CGetFileInfo";

CT::string CGetFileInfo::getLayersForFile(const char *filename){
  int status = 0;
  CDFObject * cdfObject =  CDFObjectStore::getCDFObjectStore()->getCDFObject(NULL,filename);

  CT::string fileInfo="";
       
  try{
    if(cdfObject == NULL){
      CDBError("Unable to open file %s",filename);
      throw(__LINE__);
    }  
    CT::StackList<CT::string> variableList = CDFObjectStore::getListOfVisualizableVariables(cdfObject);
    
    
    fileInfo+="<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    fileInfo+="<Configuration>\n";
    fileInfo+="\n";
       
    for(size_t j=0;j<variableList.size();j++){
      //printf("%s\n",variableList[j].c_str());
      
      CDF::Variable *var = cdfObject->getVariable(variableList[j].c_str());
      
      CT::string name = variableList[j].c_str();
      
      CT::string title = variableList[j].c_str();
      try{
        title = var->getAttribute("long_name")->toString();
      }catch(int e){
      }
      
      CT::string standardName = "";
      try{
        standardName = var->getAttribute("standard_name")->toString();
      }catch(int e){
      }
      
      if(standardName.length()>0){
        
       if(standardName.equals(variableList[j].c_str())==false){
          name.printconcat("_%s",standardName.c_str());
        }
      }
      
      fileInfo+="  <Layer type=\"database\">\n";
      fileInfo+="    <FilePath filter=\".*\\.nc$\">[DATASETDIRECTORY]</FilePath>\n";
      fileInfo.printconcat("    <Name>%s</Name>\n",name.c_str());
      fileInfo.printconcat("    <Title>%s</Title>\n",title.c_str());
      fileInfo.printconcat("    <Variable>%s</Variable>\n",variableList[j].c_str());
      fileInfo.printconcat("    <MetadataURL>[METADATAURL]</MetadataURL>\n");
      fileInfo.printconcat("    <Abstract>test abstract</Abstract>\n");
      fileInfo+="  </Layer>\n";
      fileInfo+="\n";  
    }
    

    
    fileInfo+="</Configuration>\n";
  }catch(int e){
    
    fileInfo = "";
  }
  
  
  CDFObjectStore::getCDFObjectStore()->clear();
  
  return fileInfo; 
}
