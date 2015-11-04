#include "CAutoResource.h"
#include "CTypes.h"
#include "CServerError.h"
#include "CDFObjectStore.h"

const char *CAutoResource::className = "CAutoResource";

int CAutoResource::configure(CServerParams *srvParam,bool plain){
  int status;
  status = configureDataset(srvParam,plain);if(status!=0)return status;
  status = configureAutoResource(srvParam,plain);if(status!=0)return status;
  return 0;
};
  
int CAutoResource::configureDataset(CServerParams *srvParam,bool plain){
  //Configure the server based an an available dataset
  if(srvParam->datasetLocation.empty()==false){
    //datasetLocation is usually an inspire dataset unique identifier

    //Check if dataset extension is enabled           
    bool datasetEnabled = false;
    if(srvParam->cfg->Dataset.size()==1){
      if(srvParam->cfg->Dataset[0]->attr.enabled.equals("true")&&srvParam->cfg->Dataset[0]->attr.location.empty()==false){
        datasetEnabled = true;
      }
    }
    
    if(datasetEnabled==false){
      CDBError("Dataset is not enabled for this service. ");
      return 1;
    }
    
    if(CServerParams::checkForValidTokens(srvParam->datasetLocation.c_str(),"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-:/.")==false){
      CDBError("Invalid dataset name. ");
      return 1;
    }
    
    CT::string internalDatasetLocation = srvParam->datasetLocation.c_str();
    
    internalDatasetLocation.replaceSelf(":","_");
    internalDatasetLocation.replaceSelf("/","_");
    
    CT::string datasetConfigFile = srvParam->cfg->Dataset[0]->attr.location.c_str();
    
    datasetConfigFile.printconcat("/%s.xml",internalDatasetLocation.c_str());
    
    CDBDebug("Found dataset %s",datasetConfigFile.c_str());
    
    //Check whether this config file exists.
    struct stat stFileInfo;
    int intStat;
    intStat = stat(datasetConfigFile.c_str(),&stFileInfo);
    CT::string cacheBuffer;
    //The file exists, so remove it.
    if(intStat != 0) {
      CDBDebug("Dataset config file does not exist: %s ",datasetConfigFile.c_str());
      CDBError("No such dataset");
      return 1;
    }

    //Add the dataset file to the current configuration      
    int status = srvParam->parseConfigFile(datasetConfigFile);
    if(status!=0){
      CDBError("Invalid dataset configuration file. ");
      return 1;
    }
    
    //Adjust online resource in order to pass on dataset parameters
    CT::string onlineResource=srvParam->getOnlineResource();
    CT::string stringToAdd;
    stringToAdd.concat("DATASET=");stringToAdd.concat(srvParam->datasetLocation.c_str());
    stringToAdd.concat("&amp;");
    onlineResource.concat(stringToAdd.c_str());
    srvParam->setOnlineResource(onlineResource.c_str());
    
    
    //Adjust unique cache file identifier for each dataset.
    if(srvParam->cfg->CacheDocs.size()==0){srvParam->cfg->CacheDocs.push_back(new CServerConfig::XMLE_CacheDocs());}
    CT::string validFileName;
    validFileName.print("dataset_%s",internalDatasetLocation.c_str());
    srvParam->cfg->CacheDocs[0]->attr.cachefile.copy(validFileName.c_str());
    
    //Disable autoResourceLocation
    srvParam->autoResourceLocation="";
    
    //DONE check if dataset has valid tokens [OK]
    //DONE check if sub config file exists for this identifier [OK]
    //DONE load config file and add it to existing object. [OK]
    //DONE Adjust online resource: provide dataset [OK]
    //DONE Adjust unique cache file identifier for each dataset. [OK]
    //DONE Disable autoResourceLocation [OK]
    //Done Escape identifier, : and / tokens to _
    //TODO check if WMS INSPIRE global metadata URL can be the same for all services      
  }
  return 0;
};

int CAutoResource::configureAutoResource(CServerParams *srvParam, bool plain){
  // Configure the server automically based on an OpenDAP resource
  if(srvParam->autoResourceLocation.empty()==false){
    srvParam->internalAutoResourceLocation=srvParam->autoResourceLocation.c_str();
    
    //Create unique CACHE identifier for this resource
    //When an opendap source is added, we should add unique id to the cachefile
    if(srvParam->cfg->CacheDocs.size()==0){srvParam->cfg->CacheDocs.push_back(new CServerConfig::XMLE_CacheDocs());}
    CT::string validFileName(srvParam->internalAutoResourceLocation.c_str());
    //Replace : and / by nothing, so we can use the string as a directory name for cache documents
    validFileName.replaceSelf(":",""); 
    validFileName.replaceSelf("/","");
    validFileName.replaceSelf("\\",""); 
    srvParam->cfg->CacheDocs[0]->attr.cachefile.copy(validFileName.c_str());
    
    if(srvParam->isAutoResourceEnabled()==false){
      CDBError("Automatic resource is not enabled");
      return 1;
    }
    
    bool isValidResource = false;
    //TODO should be placed in a a more generic place
    if(srvParam->isAutoOpenDAPResourceEnabled()){
      if(srvParam->autoResourceLocation.indexOf("http://")==0)isValidResource=true;
      if(srvParam->autoResourceLocation.indexOf("https://")==0)isValidResource=true;
      if(srvParam->autoResourceLocation.indexOf("dodsc://")==0)isValidResource=true;
      if(srvParam->autoResourceLocation.indexOf("dods://")==0)isValidResource=true;
      if(srvParam->autoResourceLocation.indexOf("ncdods://")==0)isValidResource=true;
    }
    
    //Error messages should be the same for different 'dir' attempts, otherwise someone can find out directory structures
    if(isValidResource==false){
      if(srvParam->isAutoLocalFileResourceEnabled()){
        if(srvParam->checkIfPathHasValidTokens(srvParam->autoResourceLocation.c_str())==false){
          CDBError("Invalid token(s), unable to read file %s",srvParam->autoResourceLocation.c_str());
          return 1;
        }
        if(srvParam->checkResolvePath(srvParam->autoResourceLocation.c_str(),&srvParam->internalAutoResourceLocation)==false){
          CDBError("Unable to resolve path for file %s",srvParam->autoResourceLocation.c_str());
          return 1;
        }
        isValidResource=true;
      }
    }
    
    if(isValidResource==false){
      CDBError("Invalid OpenDAP URL");
      readyerror();return 1;
    }
    
    //Generate the list of OpenDAP variables automatically based on the variables available in the OpenDAP dataset
    if(srvParam->autoResourceVariable.empty()||srvParam->autoResourceVariable.equals("*")){
      //Try to retrieve a list of variables from the OpenDAPURL.
      srvParam->autoResourceVariable.copy("");
      //Open the opendap resource
      CDBDebug("OGC REQUEST Remote resource %s",srvParam->internalAutoResourceLocation.c_str());
      CDFObject * cdfObject =  NULL;
      if(plain == false){
        cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(srvParam,srvParam->internalAutoResourceLocation.c_str());
      }else{
        cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeaderPlain(srvParam,srvParam->internalAutoResourceLocation.c_str());
       
      }
      //int status=cdfObject->open(srvParam->internalAutoResourceLocation.c_str());
      if(cdfObject!=NULL){
        for(size_t j=0;j<cdfObject->variables.size();j++){
          if(cdfObject->variables[j]->dimensionlinks.size()>=2||plain==true){
            if(cdfObject->variables[j]->getAttributeNE("ADAGUC_SKIP")==NULL||plain==true){
              if(!cdfObject->variables[j]->name.equals("lon")&&
                !cdfObject->variables[j]->name.equals("lat")&&
                !cdfObject->variables[j]->name.equals("lon_bounds")&&
                !cdfObject->variables[j]->name.equals("lat_bounds")&&
                !cdfObject->variables[j]->name.equals("time_bounds")&&
                !cdfObject->variables[j]->name.equals("lon_bnds")&&
                !cdfObject->variables[j]->name.equals("lat_bnds")&&
                !cdfObject->variables[j]->name.equals("time_bnds")&&
                !cdfObject->variables[j]->name.equals("time")&&
                (cdfObject->variables[j]->name.indexOf("_bnds")==-1)){
                  if(srvParam->autoResourceVariable.length()>0)srvParam->autoResourceVariable.concat(",");
                  srvParam->autoResourceVariable.concat(cdfObject->variables[j]->name.c_str());
                  //CDBDebug("%s",cdfObject->variables[j]->name.c_str());
              }
            }
          }
        }
      }
      int status=0;
      if(srvParam->autoResourceVariable.length()==0)status=1;
      if(status!=0){
        CDBError("Unable to load dataset");
        return 1;
      }
    }
    
    
    //Generate a generic title for this OpenDAP service, based on the title element in the OPeNDAP header
    //Open the opendap resource
    //CDBDebug("Opening opendap %s",srvParam->internalAutoResourceLocation.c_str());
    #ifdef MEASURETIME
    StopWatch_Stop("Opening file");
    #endif
    //CDBDebug("Opening %s",srvParam->internalAutoResourceLocation.c_str());
    CDFObject * cdfObject =  NULL;
    if(plain == false){
      cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(srvParam,srvParam->internalAutoResourceLocation.c_str());
    }else{
      cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeaderPlain(srvParam,srvParam->internalAutoResourceLocation.c_str());
     
    }
    //int status=cdfObject->open(srvParam->internalAutoResourceLocation.c_str());
    if(cdfObject==NULL){
      CDBError("Unable to open resource %s",srvParam->autoResourceLocation.c_str());
      return 1;
    }
    #ifdef MEASURETIME
    StopWatch_Stop("File opened");
    #endif
    
    CT::string serverTitle="";
    try{cdfObject->getAttribute("title")->getDataAsString(&serverTitle);}catch(int e){}
      
    if(serverTitle.length()>0){
      if(srvParam->cfg->WMS.size()>0){
        if(srvParam->cfg->WMS[0]->Title.size()>0){
          //CT::string title="ADAGUC AUTO WMS ";
          CT::string title="";
          title.concat(serverTitle.c_str());
          //title.replaceSelf(" ","_");
          srvParam->cfg->WMS[0]->Title[0]->value.copy(title.c_str());
        }
        if(srvParam->cfg->WMS[0]->RootLayer.size()>0){
          if(srvParam->cfg->WMS[0]->RootLayer[0]->Title.size()>0){
            CT::string title="WMS of  ";
            title.concat(serverTitle.c_str());
            srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->value.copy(title.c_str());
          }
        }          
      }
      if(srvParam->cfg->WCS.size()>0){
        if(srvParam->cfg->WCS[0]->Title.size()>0){
          CT::string title="ADAGUC_AUTO_WCS_";
          title.concat(serverTitle.c_str());
          srvParam->cfg->WCS[0]->Title[0]->value.copy(title.c_str());
        }
      }
    }
    CDBDebug("A");
    CT::string serverSummary="";
    CT::string serverDescription="";
    CT::string serverSource="";
    CT::string serverReferences="";
    CT::string serverInstitution="";
    CT::string serverHistory="";
    CT::string serverComments="";
    CT::string serverDisclaimer="";
    
    
    
    try{cdfObject->getAttribute("summary")->getDataAsString(&serverSummary);}catch(int e){}
    try{cdfObject->getAttribute("description")->getDataAsString(&serverDescription);}catch(int e){}
    try{cdfObject->getAttribute("source")->getDataAsString(&serverSource);}catch(int e){}
    try{cdfObject->getAttribute("references")->getDataAsString(&serverReferences);}catch(int e){}
    try{cdfObject->getAttribute("institution")->getDataAsString(&serverInstitution);}catch(int e){}
    try{cdfObject->getAttribute("history")->getDataAsString(&serverHistory);}catch(int e){}
    try{cdfObject->getAttribute("comments")->getDataAsString(&serverComments);}catch(int e){}
    try{cdfObject->getAttribute("disclaimer")->getDataAsString(&serverDisclaimer);}catch(int e){}
    CDBDebug("A");
    CT::string serverAbstract="";
    if(serverInstitution.length()>0){serverAbstract.printconcat("Institution: %s.\n",serverInstitution.c_str());}
    if(serverSummary.length()>0){serverAbstract.printconcat("Summary: %s.\n",serverSummary.c_str());}
    if(serverDescription.length()>0){serverAbstract.printconcat("Description: '%s'.\n",serverDescription.c_str());}
    if(serverSource.length()>0){serverAbstract.printconcat("Source: %s.\n",serverSource.c_str());}
    if(serverReferences.length()>0){serverAbstract.printconcat("References: %s.\n",serverReferences.c_str());}
    if(serverHistory.length()>0){serverAbstract.printconcat("History: %s.\n",serverHistory.c_str());}
    if(serverComments.length()>0){serverAbstract.printconcat("Comments: %s.\n",serverComments.c_str());}
    if(serverDisclaimer.length()>0){serverAbstract.printconcat("Disclaimer: %s.\n",serverDisclaimer.c_str());}
CDBDebug("A");
    //Replace invalid XML tokens with valid ones
    serverAbstract.replaceSelf("@" ," at ");
    serverAbstract.replaceSelf("<" ,"[");
    serverAbstract.replaceSelf(">" ,"]");
    serverAbstract.replaceSelf("&" ,"&amp;");
    
    if(serverAbstract.length()>0){
      if(srvParam->cfg->WMS.size()>0){
        if(srvParam->cfg->WMS[0]->Abstract.size()>0){
          srvParam->cfg->WMS[0]->Abstract[0]->value.copy(serverAbstract.c_str());
        }
      }
    }
    CDBDebug("A");
  
    //Generate layers based on the OpenDAP variables
    CT::StackList<CT::string> variables=srvParam->autoResourceVariable.splitToStack(",");
    for(size_t j=0;j<variables.size();j++){
      std::vector<CT::string> variableNames;
      variableNames.push_back(variables[j].c_str());
      addXMLLayerToConfig(srvParam,cdfObject,&variableNames,NULL,srvParam->internalAutoResourceLocation.c_str());
      
    }
    
    //Find derived wind parameters
    std::vector<CT::string> detectStrings;
    for(size_t v=0;v<variables.size();v++){
      int dirLoc=variables[v].indexOf("_dir");
      if(dirLoc>0){
        detectStrings.push_back(variables[v].substringr(0,dirLoc));
      }
    }
    
    //Detect  <...>_speed and <...>_dir for ASCAT data
    CT::string searchVar;
    for(size_t v=0;v<detectStrings.size();v++){
      //CDBDebug("detectStrings %s",detectStrings[v].c_str());
      searchVar.print("%s_speed",detectStrings[v].c_str());
      CDF::Variable *varSpeed = cdfObject->getVariableNE(searchVar.c_str());
      searchVar.print("%s_dir",detectStrings[v].c_str());
      CDF::Variable *varDirection = cdfObject->getVariableNE(searchVar.c_str());
      if(varSpeed!=NULL&&varDirection!=NULL){
        std::vector<CT::string> variableNames;
        variableNames.push_back(varSpeed->name.c_str());
        variableNames.push_back(varDirection->name.c_str());
        addXMLLayerToConfig(srvParam,cdfObject,&variableNames,"derived",srvParam->internalAutoResourceLocation.c_str());
      }
    }CDBDebug("A");
    
    //Detect dd and ff for wind direction and wind speed
    if(1==1){
      CDF::Variable *varSpeed = cdfObject->getVariableNE("ff");
      CDF::Variable *varDirection = cdfObject->getVariableNE("dd");
      if(varSpeed!=NULL&&varDirection!=NULL){
        std::vector<CT::string> variableNames;
        variableNames.push_back(varSpeed->name.c_str());
        variableNames.push_back(varDirection->name.c_str());
        addXMLLayerToConfig(srvParam,cdfObject,&variableNames,"derived",srvParam->internalAutoResourceLocation.c_str());
      }
    }
    CDBDebug("A");
    //Detect wind vectors based on standardnames
    if(1==1){
      
      int varindex_x=-1,varindex_y=-1;
      for(size_t j=0;j<cdfObject->variables.size();j++){
        try{
          CT::string standard_name = cdfObject->variables[j]->getAttribute("standard_name")->getDataAsString();
          if(standard_name.equals("eastward_wind")||standard_name.equals("x_wind")){
            varindex_x=j;
          }
          if(standard_name.equals("northward_wind")||standard_name.equals("y_wind")){
            varindex_y=j;
          }
        }catch(int e){
        }
      
        if(varindex_x!=-1&&varindex_y!=-1){
          CDBDebug("WindData");
          std::vector<CT::string> variableNames;
          variableNames.push_back(cdfObject->variables[varindex_x]->name.c_str());
          variableNames.push_back(cdfObject->variables[varindex_y]->name.c_str());
          addXMLLayerToConfig(srvParam,cdfObject,&variableNames,"derived",srvParam->internalAutoResourceLocation.c_str());
          varindex_x = -1;
          varindex_y = -1;
        }
      }
    }
    
    CDBDebug("A");
    
    //Adjust online resource in order to pass on variable and source parameters
    CT::string onlineResource=srvParam->getOnlineResource();
    CT::string stringToAdd;
    stringToAdd.concat("&source=");stringToAdd.concat(srvParam->autoResourceLocation.c_str());
    //stringToAdd.concat("&variable=");stringToAdd.concat(srvParam->autoResourceVariable.c_str());
    
    stringToAdd.encodeURLSelf();
    stringToAdd.concat("&amp;");
    onlineResource.concat(stringToAdd.c_str());
    srvParam->setOnlineResource(onlineResource.c_str());
    //CDBDebug("OGC REQUEST RESOURCE %s",srvParam->internalAutoResourceLocation.c_str());//,srvParam->autoResourceLocation.c_str(),);
    
    
    CDBDebug("A");
  
    #ifdef MEASURETIME
    StopWatch_Stop("Auto opendap configured");
    #endif
    
 
  }
  return 0;
};

void CAutoResource::addXMLLayerToConfig(CServerParams *srvParam,CDFObject *cdfObject,std::vector<CT::string>*variableNames, const char *group,const char *location){
  CServerConfig::XMLE_Layer *xmleLayer=new CServerConfig::XMLE_Layer();
  CServerConfig::XMLE_FilePath* xmleFilePath = new CServerConfig::XMLE_FilePath();
  

  xmleLayer->attr.type.copy("database");
  xmleFilePath->value.copy(location);
  xmleFilePath->attr.filter.copy("");
  
  if(group!=NULL){
    CServerConfig::XMLE_Group* xmleGroup = new CServerConfig::XMLE_Group();
    xmleGroup->attr.value.copy(group);
    xmleLayer->Group.push_back(xmleGroup);
  }
  
  for(size_t j=0;j<variableNames->size();j++){
    CServerConfig::XMLE_Variable* xmleVariable = new CServerConfig::XMLE_Variable();
    xmleVariable->value.copy((*variableNames)[j].c_str());
    xmleLayer->Variable.push_back(xmleVariable);
  }
  
  if(variableNames->size()>0){
    CDF::Variable *variable = cdfObject->getVariableNE((*variableNames)[0].c_str());
    if(variable!=NULL){
      CDF::Attribute *featureType = cdfObject->getAttributeNE("featureType");
      if(featureType!=NULL){
        if(featureType->getDataAsString().equals("timeSeries")||featureType->getDataAsString().equals("point")){
          CServerConfig::XMLE_RenderMethod* xmleRenderMethod = new CServerConfig::XMLE_RenderMethod();
          xmleRenderMethod->value.copy("point");
          xmleLayer->RenderMethod.insert(xmleLayer->RenderMethod.begin(),xmleRenderMethod);

        }
      }
    }
  }
  
  if(variableNames->size()==1){
      CDF::Variable *variable = cdfObject->getVariableNE((*variableNames)[0].c_str());
      if(variable!=NULL){
        CDF::Attribute *attribute = variable->getAttributeNE("standard_name");
        if(attribute!=NULL){
          if(attribute->getDataAsString().equals("rgba")){
            CServerConfig::XMLE_RenderMethod* xmleRenderMethod = new CServerConfig::XMLE_RenderMethod();
            xmleRenderMethod->value.copy("rgba");
            xmleLayer->RenderMethod.push_back(xmleRenderMethod);
          }
        }
      }
  }
  
  if(variableNames->size()==2){
    CT::string newName;
    newName.print("%s + %s",(*variableNames)[0].c_str(),(*variableNames)[1].c_str());
    
    CServerConfig::XMLE_Title* xmleTitle = new CServerConfig::XMLE_Title();
    xmleTitle->value.copy(newName.c_str());
    xmleLayer->Title.push_back(xmleTitle);
    
    CServerConfig::XMLE_Name* xmleName = new CServerConfig::XMLE_Name();
    
    newName.replaceSelf("+","and");
    newName.replaceSelf(" ","_");
    newName.encodeURLSelf();
    xmleName->value.copy(newName.c_str());
    xmleLayer->Name.push_back(xmleName);
    CServerConfig::XMLE_RenderMethod* xmleRenderMethod = new CServerConfig::XMLE_RenderMethod();
    xmleRenderMethod->value.copy("nearestpoint");
    xmleLayer->RenderMethod.push_back(xmleRenderMethod);
    
  }
  
  
  
  xmleLayer->FilePath.push_back(xmleFilePath);
  
  if(srvParam->isAutoResourceCacheEnabled()){
    CServerConfig::XMLE_Cache* xmleCache = new CServerConfig::XMLE_Cache();
    xmleCache->attr.enabled.copy("true");
    xmleLayer->Cache.push_back(xmleCache);
  }
  
  //Set imagetext property
  if(srvParam->cfg->AutoResource.size()>0){
    if(srvParam->cfg->AutoResource[0]->ImageText.size()>0){
      CServerConfig::XMLE_ImageText *xmleImageText=new CServerConfig::XMLE_ImageText();
      xmleLayer->ImageText.push_back(xmleImageText);
      if(srvParam->cfg->AutoResource[0]->ImageText[0]->value.empty()==false){
        xmleImageText->value.copy(srvParam->cfg->AutoResource[0]->ImageText[0]->value.c_str());
      }
      if(srvParam->cfg->AutoResource[0]->ImageText[0]->attr.attribute.empty()==false){
        xmleImageText->attr.attribute.copy(srvParam->cfg->AutoResource[0]->ImageText[0]->attr.attribute.c_str());
      }
      
    }
  }
  
  srvParam->cfg->Layer.push_back(xmleLayer);
};


