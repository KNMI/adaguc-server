/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 ******************************************************************************/

#include "CRequest.h"
const char *CRequest::className="CRequest";
int CRequest::CGI=0;

int CRequest::runRequest(){
  int status=process_querystring();
  CDFObjectStore::getCDFObjectStore()->clear();
  ProjectionStore::getProjectionStore()->clear();
  return status;
}

void writeLogFile3(const char * msg){
  char * logfile=getenv("ADAGUC_LOGFILE");
  if(logfile!=NULL){
    FILE * pFile = NULL;
    pFile = fopen (logfile , "a" );
    if(pFile != NULL){
      fputs  (msg, pFile );
      if(strncmp(msg,"[D:",3)==0||strncmp(msg,"[W:",3)==0||strncmp(msg,"[E:",3)==0){
        time_t myTime = time(NULL);
        tm *myUsableTime = localtime(&myTime);
        char szTemp[128];
        snprintf(szTemp,127,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ ",
                 myUsableTime->tm_year+1900,myUsableTime->tm_mon+1,myUsableTime->tm_mday,
                 myUsableTime->tm_hour,myUsableTime->tm_min,myUsableTime->tm_sec
                );
        fputs  (szTemp, pFile );
      }
      fclose (pFile);
    }//else CDBError("Unable to write logfile %s",logfile);
  }
}
void CRequest::addXMLLayerToConfig(CServerParams *srvParam,std::vector<CT::string>*variableNames, const char *group,const char *location){
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
    xmleRenderMethod->value.copy("vector_nearest,barb_nearest,vector,barb,nearest");
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
}

int CRequest::dataRestriction = -1;

/**
 * Checks whether data is restricted or not based on the environment variable ADAGUC_DATARESTRICTION.
 * If not defined or set to FALSE, there are no dataset restrictions. If set to TRUE, dataaccess is restricted for WCS, GFI, metadata and queryinfo.
 * Possible values are: ALLOW_GFI, ALLOW_WCS, ALLOW_METADATA and SHOW_QUERYINFO. Values can be combined by using the | sign without any space.
 * 
 * ALLOW_GFI: Allows getFeatureInfo request to work.
 * 
 * ALLOW_WCS: Allows dataaccess by the Web Coverage Service.
 * 
 * ALLOW_METADATA: Allows to display detailed netcdf header information.
 * 
 * SHOW_QUERYINFO: Displays failed queries, if not set "hidden" is shown instead.
 */
int CRequest::checkDataRestriction(){
  if(dataRestriction!=-1)return dataRestriction;

  //By default no restrictions
  int dr = ALLOW_WCS|ALLOW_GFI|ALLOW_METADATA;
  const char *data=getenv("ADAGUC_DATARESTRICTION");
  if(data != NULL){
    dr = ALLOW_NONE;
    CT::string temp(data);
    temp.toUpperCaseSelf();
    if(temp.equals("TRUE")){
      dr = ALLOW_NONE;
    }
    if(temp.equals("FALSE")){
      dr = ALLOW_WCS|ALLOW_GFI|ALLOW_METADATA;
    }
    //Decompose into stringlist and check each item
    CT::StackList<CT::string> items = temp.splitToStack("|");
    for(size_t j=0;j<items.size();j++){
      items[j].replaceSelf("\"","");
      if(items[j].equals("ALLOW_GFI"))dr|=ALLOW_GFI;
      if(items[j].equals("ALLOW_WCS"))dr|=ALLOW_WCS;
      if(items[j].equals("ALLOW_METADATA"))dr|=ALLOW_METADATA;
      if(items[j].equals("SHOW_QUERYINFO"))dr|=SHOW_QUERYINFO;
    }
 
  }
  
  dataRestriction = dr;
  return dataRestriction;
}


int CRequest::setConfigFile(const char *pszConfigFile){
  #ifdef MEASURETIME
  StopWatch_Stop("Set config file");
  #endif
  int status = srvParam->configObj->parseFile(pszConfigFile);
  if(status == 0 && srvParam->configObj->Configuration.size()==1){
    srvParam->configFileName.copy(pszConfigFile);
    srvParam->cfg=srvParam->configObj->Configuration[0];
    
    //Include additional config files
    for(size_t j=0;j<srvParam->cfg->Include.size();j++){
      if(srvParam->cfg->Include[j]->attr.location.empty()==false){
        CDBDebug("Include '%s'",srvParam->cfg->Include[j]->attr.location.c_str());
        status = srvParam->configObj->parseFile(srvParam->cfg->Include[j]->attr.location.c_str());
        if(status != 0){
          CDBError("There is an error with include '%s'",srvParam->cfg->Include[j]->attr.location.c_str());
          return 1;
        }
      }
    }
    
    if(srvParam->cfg->CacheDocs.size()==1){
      if(srvParam->cfg->CacheDocs[0]->attr.enabled.equals("true")){
        srvParam->enableDocumentCache=true;
      }else if(srvParam->cfg->CacheDocs[0]->attr.enabled.equals("false")){
        srvParam->enableDocumentCache=false;
      }
    }
    
  }else{
    srvParam->cfg=NULL;
    CDBError("Invalid XML file %s",pszConfigFile);
    return 1;
  }
  
  #ifdef MEASURETIME
  StopWatch_Stop("Config file parsed");
  #endif
  
  //Check for mandatory attributes
  for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
    if(srvParam->cfg->Layer[j]->attr.type.equals("database")){
      if(srvParam->cfg->Layer[j]->Variable.size()==0){
        CDBError("Configuration error at layer %d: <Variable> not defined",j);
        return 1;
      }
      if(srvParam->cfg->Layer[j]->FilePath.size()==0){
        CDBError("Configuration error at layer %d: <FilePath> not defined",j);
        return 1;
      }
    }
  }
  //Check for autoscan elements
  for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
    if(srvParam->cfg->Layer[j]->attr.type.equals("autoscan")){
      
      if(srvParam->cfg->Layer[j]->FilePath.size()==0){
        CDBError("Configuration error at layer %d: <FilePath> not defined",j);
        return 1;
      }
      
      //bool layerConfigCacheAvailable = false;
      try{
        /* Create the list of layers from a directory list */
        const char * baseDir=srvParam->cfg->Layer[j]->FilePath[0]->value.c_str();
        
        CDirReader dirReader;
        CDBDebug("autoscan");
        CDBFileScanner::searchFileNames(&dirReader,baseDir,srvParam->cfg->Layer[j]->FilePath[0]->attr.filter.c_str(),NULL);
        /*if(dirReader.listDirRecursive(baseDir,"^.*nc$")!=0){
          CDBError("Unable to list directory '%s'",baseDir);
          throw(__LINE__);
        }*/
        if(dirReader.fileList.size()==0){
          CDBError("Could not find any file in directory '%s'",baseDir);
          throw(__LINE__);
        }
        size_t nrOfFileErrors=0;
        for(size_t j=0;j<dirReader.fileList.size();j++){
          try{
          CT::string baseDirStr = baseDir;
          CT::string groupName = dirReader.fileList[j]->fullName.c_str();
          CT::string baseName = dirReader.fileList[j]->fullName.c_str();
          groupName.substringSelf(baseDirStr.length(),-1);
          //int lastSlash = baseName.lastIndexOf("/")+1;
          //baseName.substringSelf(lastSlash,-1);
          
        
        
       
      
          //Open file
          //CDBDebug("Opening file %s",dirReader.fileList[j]->fullName.c_str());
          CDFObject * cdfObject =  CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(srvParam,dirReader.fileList[j]->fullName.c_str());
          if(cdfObject == NULL){CDBError("Unable to read file %s",dirReader.fileList[j]->fullName.c_str());throw(__LINE__);}
          
          //std::vector<CT::string> variables;
          //List variables
          for(size_t v=0;v<cdfObject->variables.size();v++){
            CDF::Variable *var=cdfObject->variables[v];
            if(var->isDimension==false){
              if(var->dimensionlinks.size()>=2){
               // variables.push_back(new CT::string(var->name.c_str()));
                CServerConfig::XMLE_Layer *xmleLayer=new CServerConfig::XMLE_Layer();
                CServerConfig::XMLE_Group* xmleGroup = new CServerConfig::XMLE_Group();
                CServerConfig::XMLE_Variable* xmleVariable = new CServerConfig::XMLE_Variable();
                CServerConfig::XMLE_FilePath* xmleFilePath = new CServerConfig::XMLE_FilePath();
                CServerConfig::XMLE_Cache* xmleCache = new CServerConfig::XMLE_Cache();
                xmleCache->attr.enabled.copy("false");
                xmleLayer->attr.type.copy("database");
                xmleVariable->value.copy(var->name.c_str());
                xmleFilePath->value.copy(dirReader.fileList[j]->fullName.c_str());
                xmleGroup->attr.value.copy(groupName.c_str());
                xmleLayer->Variable.push_back(xmleVariable);
                xmleLayer->FilePath.push_back(xmleFilePath);
                xmleLayer->Cache.push_back(xmleCache);
                xmleLayer->Group.push_back(xmleGroup);
                srvParam->cfg->Layer.push_back(xmleLayer);              
              }
            }
          }
          
          
         
          }catch(int e){
            nrOfFileErrors++;
          }
          
        }
        if(nrOfFileErrors!=0){
          CDBError("%d files are not readable",nrOfFileErrors);
        }
        
      }catch(int line){
        return 1;
      }
      
    }
  }
  #ifdef MEASURETIME
  StopWatch_Stop("Config file checked");
  #endif
  
  return status;
}


int CRequest::process_wms_getmetadata_request(){
  if(srvParam->WMSLayers!=NULL)
    for(size_t j=0;j<srvParam->WMSLayers->count;j++){
      CDBDebug("WMS GETMETADATA %s",srvParam->WMSLayers[j].c_str());
    }
    return process_all_layers();
}

int CRequest::process_wms_getlegendgraphic_request(){
  if(srvParam->WMSLayers!=NULL)
    for(size_t j=0;j<srvParam->WMSLayers->count;j++){
      CDBDebug("WMS GETLEGENDGRAPHIC %s",srvParam->WMSLayers[j].c_str());
    }
  return process_all_layers();
}
int CRequest::process_wms_getfeatureinfo_request(){
  if(srvParam->WMSLayers!=NULL)
    for(size_t j=0;j<srvParam->WMSLayers->count;j++){
      CDBDebug("WMS GETFEATUREINFO %s",srvParam->WMSLayers[j].c_str());
    }
  return process_all_layers();
}

int CRequest::process_wcs_getcoverage_request(){
  #ifndef ADAGUC_USE_GDAL
    CServerParams::showWCSNotEnabledErrorMessage();
    return 1;
  #else
  if(srvParam->WMSLayers!=NULL)
    for(size_t j=0;j<srvParam->WMSLayers->count;j++){
      CDBDebug("WCS GETCOVERAGE %s",srvParam->WMSLayers[j].c_str());
    }
  return process_all_layers();
  #endif
}
const char *CSimpleStore::className="CSimpleStore";


int CRequest::storeDocumentCache(CSimpleStore *simpleStore){
  //Store the store
  CT::string cacheBuffer;
  simpleStore->getBuffer(&cacheBuffer);
  CT::string cacheFileName;
  srvParam->getCacheFileName(&cacheFileName);
  FILE * pFile = fopen (cacheFileName.c_str() , "wb" );
  if(pFile != NULL){
    fputs  (cacheBuffer.c_str(), pFile );
    fclose (pFile);
    if(chmod(cacheFileName.c_str(),0777)<0){
      CDBError("Unable to change permissions of cachefile %s",cacheFileName.c_str());
      return 1;
    }
  }else {
    CDBError("Unable to write cachefile %s",cacheFileName.c_str());
    return 1;
  }
  return 0;
}

int CRequest::getDocFromDocCache(CSimpleStore *simpleStore,CT::string *docName,CT::string *document){
  //If CacheDocs is true and the configuration is unmodified
  //the XML can be retrieved from the disk
  bool cacheNeedsRefresh=false;
  //Check if the configuration file is modified
  //Get configration file modification time
  CT::string configModificationDate;
  struct tm* clock;       // create a time structure
  struct stat attrib;     // create a file attribute structure
  stat(srvParam->configFileName.c_str(), &attrib);    // get the attributes of afile.txt
  clock = gmtime(&(attrib.st_mtime)); // Get the last modified time and put it into the time structure
  char buffer [80];
  //strftime (buffer,80,"%I:%M%p.",clock);
  strftime (buffer,80,"%Y-%m-%dT%H:%M:%SZ",clock);
  configModificationDate.copy(buffer);
  
  //Get a filename suited for diskstorage
  CT::string cacheFileName;
  srvParam->getCacheFileName(&cacheFileName);
  //CDBDebug("cacheFileName: %s",cacheFileName.c_str());  
  //Check wether the cache file exists
  struct stat stFileInfo;
  int intStat;
  intStat = stat(cacheFileName.c_str(),&stFileInfo);
  CT::string cacheBuffer;
  // If the file does not exist, the cache needs to be created
  if(intStat != 0) {
    CDBDebug("The cache file %s does not exist",cacheFileName.c_str());
    cacheNeedsRefresh = true; 
  }
  if(cacheNeedsRefresh==false){
    //Try to open the cache file for reading
    FILE * cacheF = fopen(cacheFileName.c_str(),"r");
    if (cacheF!=NULL){
      // obtain file size:
      fseek (cacheF , 0 , SEEK_END);
      size_t fileSize = ftell (cacheF);
      rewind (cacheF);
      char * pszCacheBuffer = new char[fileSize+1];
      // copy the file into the buffer:
      size_t bytesRead = fread (pszCacheBuffer,1,fileSize,cacheF);
      if (bytesRead != fileSize) {CDBError("Reading error of cache file %s",cacheFileName.c_str());delete[] pszCacheBuffer; return 1;}
      fclose (cacheF);
      //Store the data in the CT::string object
      cacheBuffer.copy(pszCacheBuffer);
      delete[] pszCacheBuffer;
    }else {
      CDBDebug("Unable to open cachefile %s",cacheFileName.c_str());
      cacheNeedsRefresh=true;
    }
  }
  if(document==NULL||docName==NULL)return 0;
  if(cacheNeedsRefresh==false){
    //Now compare compare file modification date from last known file modification date
    CT::string oldConfigModificationDate;
    simpleStore->parse(cacheBuffer.c_str());
    if(simpleStore->getCTStringAttribute("configModificationDate",&oldConfigModificationDate)!=0){
      CDBDebug("configModificationDate not available in cachefile %s",cacheFileName.c_str());
      //simpleStore->setStringAttribute("configModificationDate",configModificationDate.c_str());
      cacheNeedsRefresh=true;
    }else{
      if(oldConfigModificationDate.equals(&configModificationDate)){
        //The modification date of the configuration file is the same as the stored one.
        //CDBDebug("Cache needs no update");
      }else{
        CDBDebug("Cache needs update because %s!=%s",oldConfigModificationDate.c_str(),configModificationDate.c_str());
        //simpleStore->setStringAttribute("configModificationDate",configModificationDate.c_str());
        cacheNeedsRefresh=true;
      }
    }
  }
  if(cacheNeedsRefresh==false){
    //Read and Provide the xml document!  
    int status = simpleStore->getCTStringAttribute(docName->c_str(),document);
    if(status!=0){
      //CDBDebug("Unable to get document %s from cache",docName->c_str());
      cacheNeedsRefresh=true;
    }
  }
  if(cacheNeedsRefresh==true){
    simpleStore->setStringAttribute("configModificationDate",configModificationDate.c_str());
    //CDBDebug("cacheNeedsRefresh==true");
    return 2;
  }
  return 0;
}

int CRequest::generateOGCGetCapabilities(CT::string *XMLdocument){
  CXMLGen XMLGen;
    //Set WMSLayers:
  srvParam->WMSLayers = new CT::string[srvParam->cfg->Layer.size()];
  for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
    srvParam->makeUniqueLayerName(&srvParam->WMSLayers[j],srvParam->cfg->Layer[j]);
    srvParam->WMSLayers[j].count=srvParam->cfg->Layer.size();
  }
  return XMLGen.OGCGetCapabilities(srvParam,XMLdocument);
}

int CRequest::generateGetReferenceTimes(CT::string *result){
    //Set WMSLayers:
  
  std::set<std::string>WMSGroups ;
  for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
    CT::string groupName;
    srvParam->makeLayerGroupName(&groupName,srvParam->cfg->Layer[j]);
    if (groupName.testRegEx("[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]]_[[:digit:]][[:digit:]]")) {
      CT::string ymd=groupName.substringr(0,8);
      CT::string hh=groupName.substringr(9,11);
      ymd.concat(hh);
      ymd.concat("00");
      WMSGroups.insert(ymd.c_str());
    } else if (groupName.testRegEx("[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]]")) {
      groupName.concat("00");
      WMSGroups.insert(groupName.c_str());
    } else if (groupName.testRegEx("[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]]")) {
      WMSGroups.insert(groupName.c_str());
    }
  }
  result->copy("[");
  bool first=true;
  for (std::set<std::string>::reverse_iterator it=WMSGroups.rbegin(); it!=WMSGroups.rend(); ++it) {
    if (!first) {
      result->concat(",");
    }
    first=false;
    result->concat("\"");
    result->concat((*it).c_str());
    result->concat("\"");
  }
  result->concat("]");
  return 0;
}

int CRequest::generateOGCDescribeCoverage(CT::string *XMLdocument){
  CXMLGen XMLGen;
  for(size_t j=0;j<srvParam->WMSLayers->count;j++){
    CDBDebug("WCS_DESCRIBECOVERAGE %s",srvParam->WMSLayers[j].c_str());
  }
  return XMLGen.OGCGetCapabilities(srvParam,XMLdocument);
}

int CRequest::process_wms_getcap_request(){
  CDBDebug("WMS GETCAPABILITIES");
  
  CT::string XMLdocument;
  if(srvParam->enableDocumentCache==true){
    CSimpleStore simpleStore;
    CT::string documentName;
    bool storeNeedsUpdate=false;
    int status = getDocumentCacheName(&documentName,srvParam);if(status!=0)return 1;
    //Try to get the getcapabilities document from the store:
    status = getDocFromDocCache(&simpleStore,&documentName,&XMLdocument);if(status==1)return 1;
    //if(status==2, the store is ok, but not up to date
    if(status==2)storeNeedsUpdate=true;
    if(storeNeedsUpdate){
      //CDBDebug("Generating a new document with name %s",documentName.c_str());
      int status = generateOGCGetCapabilities(&XMLdocument);if(status==CXMLGEN_FATAL_ERROR_OCCURED)return 1;
      //Store this document  
      if(status==0){
        simpleStore.setStringAttribute(documentName.c_str(),XMLdocument.c_str());
        if(storeDocumentCache(&simpleStore)!=0)return 1;
      }
    }else{
      CT::string cacheFileName;
      srvParam->getCacheFileName(&cacheFileName);
      CDBDebug("Providing document from store '%s' with name '%s'",cacheFileName.c_str(),documentName.c_str());
    }
  }else{
    //Do not use cache
    int status = generateOGCGetCapabilities(&XMLdocument);;if(status==CXMLGEN_FATAL_ERROR_OCCURED)return 1;
  }
  printf("%s%c%c\n","Content-Type:text/xml",13,10);
  printf("%s",XMLdocument.c_str());
  

  
  return 0;
}

int CRequest::process_wms_getreferencetimes_request(){
  CDBDebug("WMS GETREFERENCETIMES");
  
  CT::string XMLdocument;
  if(srvParam->enableDocumentCache==true){
    CSimpleStore simpleStore;
    CT::string documentName;
    bool storeNeedsUpdate=false;
    int status = getDocumentCacheName(&documentName,srvParam);if(status!=0)return 1;
    //Try to get the getcapabilities document from the store:
    status = getDocFromDocCache(&simpleStore,&documentName,&XMLdocument);if(status==1)return 1;
    //if(status==2, the store is ok, but not up to date
    if(status==2)storeNeedsUpdate=true;
    if(storeNeedsUpdate){
      //CDBDebug("Generating a new document with name %s",documentName.c_str());
      int status = generateGetReferenceTimes(&XMLdocument);if(status==CXMLGEN_FATAL_ERROR_OCCURED)return 1;
      //Store this document  
      if(status==0){
        simpleStore.setStringAttribute(documentName.c_str(),XMLdocument.c_str());
        if(storeDocumentCache(&simpleStore)!=0)return 1;
      }
    }else{
      CDBDebug("Providing document from store with name %s",documentName.c_str());
    }
  }else{
    //Do not use cache
    int status = generateGetReferenceTimes(&XMLdocument);;if(status==CXMLGEN_FATAL_ERROR_OCCURED)return 1;
  }
  if (srvParam->JSONP.length()==0) {
    printf("%s%c%c\n","Content-Type: application/json ",13,10);
    printf("%s",XMLdocument.c_str());
  } else {
    printf("%s%c%c\n","Content-Type: text/javascript ",13,10);
    printf("%s(%s)",srvParam->JSONP.c_str(),XMLdocument.c_str());
  }
  

  
  return 0;
}

int CRequest::process_wcs_getcap_request(){
  CDBDebug("WCS GETCAPABILITIES");
  return process_wms_getcap_request();
}

int CRequest::process_wcs_describecov_request(){
  return process_all_layers();
}

int CRequest::process_wms_getmap_request(){
  if(srvParam->WMSLayers!=NULL){
    CT::string message = "WMS GETMAP ";
    for(size_t j=0;j<srvParam->WMSLayers->count;j++){
      if(j>0)message.concat(",");
      message.printconcat("(%d) %s",j,srvParam->WMSLayers[j].c_str());
    }
    CDBDebug(message.c_str());
  }else{
    CDBDebug("WMS GETMAP no layers");
  }
  return process_all_layers();
}



const char *timeFormatAllowedChars="0123456789:TZ-/. _ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz()";
bool CRequest::checkTimeFormat(CT::string& timeToCheck){
  if(timeToCheck.length()<1)return false;
//  bool isValidTime = false;
  //First test wether invalid characters are in this string
  int numValidChars = strlen(timeFormatAllowedChars);
  const char * timeChars=timeToCheck.c_str();
  int numTimeChars = strlen(timeChars);
  for(int j=0;j<numTimeChars;j++){
    int i=0;
    for(i=0;i<numValidChars;i++)if(timeChars[j]==timeFormatAllowedChars[i])break;
    if(i==numValidChars)return false;
  }
  return true;
  /*for(int j=0;j<NUMTIMEFORMATS&&isValidTime==false;j++){
    isValidTime = timeToCheck.testRegEx(timeFormats[j].pattern);
  }
  return isValidTime;*/
}


int CRequest::getDimValuesForDataSource(CDataSource *dataSource,CServerParams *srvParam){
  int status = 0;
  try{
    CPGSQLDB DB;
    CT::string query;
  
    // Connect do DB
    status = DB.connect(srvParam->cfg->DataBase[0]->attr.parameters.c_str());if(status!=0){CDBError("Unable to connect to DB");return 1;}
    
    /*
      * Check if all tables are available, if not update the db
      */
    if(srvParam->isAutoResourceEnabled()){
      status = dataSource->CDataSource::checkDimTables(&DB);
      if(status != 0){
        CDBError("checkAndUpdateDimTables failed");
        return status;
      }
    }
    
    /*
      * Get the number of required dims from the given dims
      * Check if all dimensions are given
      */
    #ifdef CREQUEST_DEBUG
    CDBDebug("Get DIMS from query string");
    #endif
    for(size_t k=0;k<srvParam->requestDims.size();k++)srvParam->requestDims[k]->name.toLowerCaseSelf();
    
    for(size_t i=0;i<dataSource->cfgLayer->Dimension.size();i++){
      CT::string dimName(dataSource->cfgLayer->Dimension[i]->value.c_str());
      dimName.toLowerCaseSelf();
      #ifdef CREQUEST_DEBUG
      CDBDebug("dimName %s",dimName.c_str());
      #endif
      //Check if this dim is not already added
      bool alreadyAdded=false;
      for(size_t l=0;l<dataSource->requiredDims.size();l++){
        if(dataSource->requiredDims[l]->name.equals(&dimName)){alreadyAdded=true;break;}
      }
      
      #ifdef CREQUEST_DEBUG
      CDBDebug("alreadyAdded = %d",alreadyAdded);
      #endif
      if(alreadyAdded == false){
        for(size_t k=0;k<srvParam->requestDims.size();k++){
          if(srvParam->requestDims[k]->name.equals(&dimName)){
            #ifdef CREQUEST_DEBUG
            CDBDebug("DIM COMPARE: %s==%s",srvParam->requestDims[k]->name.c_str(),dimName.c_str());
            #endif
            
            //This dimension has been specified in the request, so the dimension has been found:
            COGCDims *ogcDim = new COGCDims();
            dataSource->requiredDims.push_back(ogcDim);
            ogcDim->name.copy(&dimName);
            ogcDim->value.copy(&srvParam->requestDims[k]->value);
            ogcDim->netCDFDimName.copy(dataSource->cfgLayer->Dimension[i]->attr.name.c_str());

            // If we have value 'current', give the dim a special status
            if(ogcDim->value.equals("current")){
              CT::string tableName;
              
              try{
                tableName = dataSource->srvParams->lookupTableName(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), ogcDim->netCDFDimName.c_str());
              }catch(int e){
                CDBError("Unable to create tableName from '%s' '%s' '%s'",dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), ogcDim->netCDFDimName.c_str());
                return 1;
              }
              
              
              query.print("select max(%s) from %s",ogcDim->netCDFDimName.c_str(),tableName.c_str());
              CDB::Store *maxStore = DB.queryToStore(query.c_str());
              if(maxStore == NULL){CDBError("query failed");status = DB.close(); return 1;}
              ogcDim->value.copy(maxStore->getRecord(0)->get(0));
              delete maxStore;
            }
          }
        }
      }
    }
    #ifdef CREQUEST_DEBUG
    CDBDebug("Get DIMS from query string ready");
    #endif
    
    /* Fill in the undefined dims */
    
    for(size_t i=0;i<dataSource->cfgLayer->Dimension.size();i++){
      CT::string dimName(dataSource->cfgLayer->Dimension[i]->value.c_str());
      dimName.toLowerCaseSelf();
      bool alreadyAdded=false;
      for(size_t k=0;k<dataSource->requiredDims.size();k++){
        if(dataSource->requiredDims[k]->name.equals(&dimName)){alreadyAdded=true;break;}
      }
      if(alreadyAdded==false){
        CT::string netCDFDimName(dataSource->cfgLayer->Dimension[i]->attr.name.c_str());

        CT::string tableName;
        try{
          tableName = dataSource->srvParams->lookupTableName(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), netCDFDimName.c_str());
        }catch(int e){
          CDBError("Unable to create tableName from '%s' '%s' '%s'",dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), netCDFDimName.c_str());
          return 1;
        }

        
        //Add the undefined dims to the srvParams as additional dims
        COGCDims *ogcDim = new COGCDims();
        dataSource->requiredDims.push_back(ogcDim);
        ogcDim->name.copy(&dimName);
        ogcDim->netCDFDimName.copy(dataSource->cfgLayer->Dimension[i]->attr.name.c_str());
        //Try to find the max value for this dim name from the database
        CT::string query;
        query.print("select max(%s) from %s",
                    dataSource->cfgLayer->Dimension[i]->attr.name.c_str(),
                    tableName.c_str());
        //Execute the query
        CDB::Store *maxStore = DB.queryToStore(query.c_str());
        if(maxStore == NULL){CDBError("query failed");status = DB.close(); return 1;}
        ogcDim->value.copy(maxStore->getRecord(0)->get(0));
        delete maxStore;
      }
    }        
  
    CT::string queryOrderedDESC;
    queryOrderedDESC.print("select a0.path");
    for(size_t i=0;i<dataSource->requiredDims.size();i++){
      queryOrderedDESC.printconcat(",%s,dim%s",dataSource->requiredDims[i]->netCDFDimName.c_str(),dataSource->requiredDims[i]->netCDFDimName.c_str());
      
    }
    
    queryOrderedDESC.concat(" from ");
    bool timeValidationError = false;
    
    //Compose the query
    for(size_t i=0;i<dataSource->requiredDims.size();i++){
      CT::string netCDFDimName(&dataSource->requiredDims[i]->netCDFDimName);

      CT::string tableName;
      try{
        tableName = dataSource->srvParams->lookupTableName(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), netCDFDimName.c_str());
      }catch(int e){
        CDBError("Unable to create tableName from '%s' '%s' '%s'",dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), netCDFDimName.c_str());
        return 1;
      }

      CT::string subQuery;
      subQuery.print("(select path,dim%s,%s from %s where ",netCDFDimName.c_str(),
                  netCDFDimName.c_str(),
                  tableName.c_str());
      CT::string queryParams(&dataSource->requiredDims[i]->value);
      CT::string *cDims =queryParams.splitToArray(",");// Split up by commas (and put into cDims)
      for(size_t k=0;k<cDims->count;k++){
        CT::string *sDims =cDims[k].splitToArray("/");// Split up by slashes (and put into sDims)
        if(sDims->count>0&&k>0)subQuery.concat("or ");
        for(size_t  l=0;l<sDims->count&&l<2;l++){
          if(sDims[l].length()>0){
            //dataSource->requiredDims[i]->allValues.push_back(sDims[l].c_str());
            //CDBDebug("requiredDims %d %s",i,sDims[l].c_str());
            if(l>0)subQuery.concat("and ");
            if(sDims->count==1){
              if(!checkTimeFormat(sDims[l]))timeValidationError=true;
              subQuery.printconcat("%s = '%s' ",netCDFDimName.c_str(),sDims[l].c_str());
            }
            
            //TODO Currently only start/stop is supported, start/stop/resolution is not supported yet.
            if(sDims->count>=2){
              if(l==0){
                if(!checkTimeFormat(sDims[l]))timeValidationError=true;
                subQuery.printconcat("%s >= '%s' ",netCDFDimName.c_str(),sDims[l].c_str());
              }
              if(l==1){
                if(!checkTimeFormat(sDims[l]))timeValidationError=true;
                subQuery.printconcat("%s <= '%s' ",netCDFDimName.c_str(),sDims[l].c_str());
              }
            }
          }
        }
        delete[] sDims;
      }
      delete[] cDims;
      if(i==0){
        subQuery.printconcat("ORDER BY %s DESC limit 24*6)a%d ",netCDFDimName.c_str(),i);
      }else{
        subQuery.printconcat("ORDER BY %s DESC)a%d ",netCDFDimName.c_str(),i);
      }
      //subQuery.printconcat("ORDER BY %s DESC)a%d ",netCDFDimName.c_str(),i);
      if(i<dataSource->requiredDims.size()-1)subQuery.concat(",");
      queryOrderedDESC.concat(&subQuery);
    }
    
    //Join by path
    if(dataSource->requiredDims.size()>1){
      queryOrderedDESC.concat(" where a0.path=a1.path");
      for(size_t i=2;i<dataSource->requiredDims.size();i++){
        queryOrderedDESC.printconcat(" and a0.path=a%d.path",i);
      }
    }
    
    //writeLogFile3(queryOrderedDESC.c_str());
    //writeLogFile3("\n");
    //queryOrderedDESC.concat(" limit 40");
  
    if(timeValidationError==true){
      if((checkDataRestriction()&SHOW_QUERYINFO)==false)queryOrderedDESC.copy("hidden");
      CDBError("queryOrderedDESC fails regular expression: '%s'",queryOrderedDESC.c_str());
      status = DB.close();
      return 1;
    }
    
    query.print("select * from (%s)T order by ",queryOrderedDESC.c_str());
    query.concat(&dataSource->requiredDims[0]->netCDFDimName);
    for(size_t i=1;i<dataSource->requiredDims.size();i++){
      query.printconcat(",%s",dataSource->requiredDims[i]->netCDFDimName.c_str());
    }
    
    //Execute the query
    
      //writeLogFile3(query.c_str());
      //writeLogFile3("\n");
    //values_path = DB.query_select(query.c_str(),0);
    
    CDB::Store *store = NULL;
    try{
      store = DB.queryToStore(query.c_str(),true);
    }catch(int e){
      if((checkDataRestriction()&SHOW_QUERYINFO)==false)query.copy("hidden");
      
      CDBDebug("Query failed with code %d (%s)",e,query.c_str());
      DB.close();
      return 1;
    }
    
    if(store == NULL){
      if((checkDataRestriction()&SHOW_QUERYINFO)==false)query.copy("hidden");
      CDBError("No results for query: '%s'",query.c_str());
      DB.close();
      return 2;
    }
    if(store->getSize() == 0){
      if((checkDataRestriction()&SHOW_QUERYINFO)==false)query.copy("hidden");
      CDBError("No results for query: '%s'",query.c_str());
      delete store;
      DB.close();
      return 2;
    }
          
    for(size_t k=0;k<store->getSize();k++){
      CDB::Record *record = store->getRecord(k);
      CDataSource::TimeStep * timeStep = new CDataSource::TimeStep();
      dataSource->timeSteps.push_back(timeStep);
      //timeStep->fileName.copy(values_path[k].c_str());
      timeStep->fileName.copy(record->get(0)->c_str());
      //CDBDebug("%s",timeStep->fileName.c_str());
      //timeStep->timeString.print("%sZ",date_time[k].c_str());
      timeStep->timeString.print("%sZ",record->get(1)->c_str());
      timeStep->timeString.setChar(10,'T');
      
      
      //For each timesteps a new set of dimensions is added with corresponding dim array indices.
      for(size_t i=0;i<dataSource->requiredDims.size();i++){
        timeStep->dims.addDimension(dataSource->requiredDims[i]->netCDFDimName.c_str(),record->get(1+i*2)->c_str(),atoi(record->get(2+i*2)->c_str()));
        
        //CDBDebug("%s %s",dataSource->requiredDims[i]->netCDFDimName.c_str(),dataSource->requiredDims[i]->value.c_str());
        //CDBDebug("%s = %s",record->get(1+i*2)->c_str(),record->get(2+i*2)->c_str());
        dataSource->requiredDims[i]->addValue(record->get(1+i*2)->c_str());
        //dataSource->requiredDims[i]->allValues.push_back(sDims[l].c_str());
      }
    }
    
    
//     for(size_t i=0;i<dataSource->requiredDims.size();i++){
//       CDBDebug("%d There are %d values for dimension %s",i,dataSource->requiredDims[i]->uniqueValues.size(),dataSource->requiredDims[i]->netCDFDimName.c_str());  
//     }

    status = DB.close();  if(status!=0)return 1;
    delete store;
  }catch(int i){
    CDBError("%d",i);
    return 2;
  }
  return 0;
}

int CRequest::process_all_layers(){
  CT::string pathFileName;
  
  // No layers defined, so maybe the DescribeCoverage request did not define any coverages...
  if(srvParam->WMSLayers==NULL){
    if(srvParam->requestType==REQUEST_WCS_DESCRIBECOVERAGE){
      // Therefore we read all the coverages, we do this by defining WMSLayers, as if the user entered in the coverages section all layers.
      srvParam->requestType=REQUEST_WCS_DESCRIBECOVERAGE;
      srvParam->WMSLayers = new CT::string[srvParam->cfg->Layer.size()];
      for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
        srvParam->makeUniqueLayerName(&srvParam->WMSLayers[j],srvParam->cfg->Layer[j]);
        srvParam->WMSLayers[j].count=srvParam->cfg->Layer.size();
      }
    }
    else {CDBError("No layers/coverages defined" );return 1;}
  }else{
    //Otherwise WMSLayers are defined by the user, so we need only the selection the user has made
    if(srvParam->WMSLayers->count==0){
      CDBError("No layers/coverages defined" );
      return 1;
    }
    
    //dataSources = new CDataSource[srvParam->WMSLayers->count];
    //Now set the properties of these sourceimages
    CT::string layerName;
    
    for(size_t j=0;j<srvParam->WMSLayers->count;j++){
      size_t layerNo=0;
      for(layerNo=0;layerNo<srvParam->cfg->Layer.size();layerNo++){
         

              
        srvParam->makeUniqueLayerName(&layerName,srvParam->cfg->Layer[layerNo]);
        //CDBError("comparing (%d) %s==%s",j,layerName.c_str(),srvParam->WMSLayers[j].c_str());
        if(layerName.equals(srvParam->WMSLayers[j].c_str())){
          CDataSource *dataSource = new CDataSource ();
          dataSources.push_back(dataSource);
          if(dataSource->setCFGLayer(srvParam,srvParam->configObj->Configuration[0],srvParam->cfg->Layer[layerNo],layerName.c_str(),j)!=0){
            return 1;
          }
          break;
        }
      }
      if(layerNo==srvParam->cfg->Layer.size()){
        CDBError("Layer [%s] not found",srvParam->WMSLayers[j].c_str());
        return 1;
      }
    }
  }




  int status;
  if(srvParam->serviceType==SERVICE_WMS){
    if(srvParam->Geo->dWidth>8192){
      CDBError("Parameter WIDTH must be smaller than 8192");
      return 1;
    }
    if(srvParam->Geo->dHeight>8192){
    CDBError("Parameter HEIGHT must be smaller than 8192");
    return 1;
    }
  }


  if(srvParam->serviceType==SERVICE_WCS){
    if(srvParam->requestType==REQUEST_WCS_DESCRIBECOVERAGE){
      CT::string XMLDocument;
      /*if(srvParam->enableDocumentCache==true){
        CSimpleStore simpleStore;
        CT::string documentName;
        bool storeNeedsUpdate=false;
        int status = getDocumentCacheName(&documentName,srvParam);if(status!=0)return 1;
        //Try to get the getcapabilities document from the store:
        status = getDocFromDocCache(&simpleStore,&documentName,&XMLDocument);if(status==1)return 1;
        //if(status==2, the store is ok, but not up to date
        if(status==2)storeNeedsUpdate=true;
        if(storeNeedsUpdate){
          CDBDebug("Generating a new document with name %s",documentName.c_str());
          status = generateOGCDescribeCoverage(&XMLDocument);if(status!=0)return 1;
          //Store this document  
          simpleStore.setStringAttribute(documentName.c_str(),XMLDocument.c_str());
          if(storeDocumentCache(&simpleStore)!=0)return 1;
        }else{
          CDBDebug("Providing document from store with name %s",documentName.c_str());
        }
      }else{
        //Do not use cache
        status = generateOGCDescribeCoverage(&XMLDocument);if(status!=0)return 1;
    }*/
      status = generateOGCDescribeCoverage(&XMLDocument);if(status==CXMLGEN_FATAL_ERROR_OCCURED)return 1;
      printf("%s%c%c\n","Content-Type:text/xml",13,10);
      printf("%s",XMLDocument.c_str());
      return 0;
    }
  }

  for(size_t j=0;j<dataSources.size();j++){
    
    
    if(dataSources[j]->dLayerType==CConfigReaderLayerTypeUnknown){
      CDBError("Unknow layer type");
      return 0;
    }

    /***************************/
    /* Type = Database layer   */
    /***************************/
    if(dataSources[j]->dLayerType==CConfigReaderLayerTypeDataBase||
      dataSources[j]->dLayerType==CConfigReaderLayerTypeStyled)
    {

      //When this layer has no dimensions, we do not need to query 
      // When there are no dims, we can get the filename from the config
      if(dataSources[j]->cfgLayer->Dimension.size()==0){
       
        CDataReader::autoConfigureDimensions(dataSources[j]);
        
      }
      if(dataSources[j]->cfgLayer->Dimension.size()!=0){
        if(getDimValuesForDataSource(dataSources[j],srvParam)!=0){
          CDBError("Unable to fill in dimensions");
          return 1;
        }
        /*delete[] values_path;
        delete[] values_dim;
        delete[] date_time;*/
      }else{
        //This layer has no dimensions, but we need to add one timestep with data in order to make the next code work.        
        CDataSource::TimeStep * timeStep = new CDataSource::TimeStep();
        dataSources[j]->timeSteps.push_back(timeStep);
        timeStep->fileName.copy(dataSources[j]->cfgLayer->FilePath[0]->value.c_str());
        timeStep->timeString.copy("0");
        timeStep->dims.addDimension("time","0",0);
      }
    }
    
    if(dataSources[j]->dLayerType==CConfigReaderLayerTypeCascaded){
        //This layer has no dimensions, but we need to add one timestep with data in order to make the next code work.
        CDataSource::TimeStep * timeStep = new CDataSource::TimeStep();
        dataSources[j]->timeSteps.push_back(timeStep);
        timeStep->fileName.copy("");
        timeStep->timeString.copy("0");
        timeStep->dims.addDimension("time","0",0);
    }
  }
  
  int j=0;
  
    /**************************************/
    /* Handle WMS Getmap database request */
    /**************************************/
    if(dataSources[j]->dLayerType==CConfigReaderLayerTypeDataBase||
      dataSources[j]->dLayerType==CConfigReaderLayerTypeStyled||
      dataSources[j]->dLayerType==CConfigReaderLayerTypeCascaded)
    {
      try{
        for(size_t d=0;d<dataSources.size();d++){
          dataSources[d]->setTimeStep(0);
        }
        if(srvParam->requestType==REQUEST_WMS_GETMAP){
          CImageDataWriter imageDataWriter;

          /**
            We want like give priority to our own internal layers, instead to external cascaded layers. This is because
            our internal layers have an exact customized legend, and we would like to use this always.
          */
       
          bool imageDataWriterIsInitialized = false;
          int dataSourceToUse=0;
          for(size_t d=0;d<dataSources.size()&&imageDataWriterIsInitialized==false;d++){
            if(dataSources[d]->dLayerType!=CConfigReaderLayerTypeCascaded){
              //CDBDebug("INIT");
              status = imageDataWriter.init(srvParam,dataSources[d],dataSources[d]->getNumTimeSteps());if(status != 0)throw(__LINE__);
              imageDataWriterIsInitialized=true;
              dataSourceToUse=d;
            }
          }
          
          //There are only cascaded layers, so we intialize the imagedatawriter with this the first layer.
          if(imageDataWriterIsInitialized==false){
            status = imageDataWriter.init(srvParam,dataSources[0],dataSources[0]->getNumTimeSteps());if(status != 0)throw(__LINE__);
            dataSourceToUse=0;
            imageDataWriterIsInitialized=true;
          }
          
          //When we have multiple timesteps, we will create an animation.
          if(dataSources[dataSourceToUse]->getNumTimeSteps()>1)imageDataWriter.createAnimation();
    
          for(size_t k=0;k<(size_t)dataSources[dataSourceToUse]->getNumTimeSteps();k++){
            for(size_t d=0;d<dataSources.size();d++){
              dataSources[d]->setTimeStep(k);
            }
            if(dataSources[j]->dLayerType==CConfigReaderLayerTypeDataBase||
              dataSources[j]->dLayerType==CConfigReaderLayerTypeCascaded){
              status = imageDataWriter.addData(dataSources);
              if(status != 0){
                /**
                 * Adding data failed:
                 * Do not ruin an animation if one timestep fails to load.
                 * If there is a single time step then throw an exception otherwise continue.
                 */
                if(dataSources[dataSourceToUse]->getNumTimeSteps()==1){
                  //Not an animation, so throw an exception
                  CDBError("Unable to convert datasource %s to image",dataSources[j]->layerName.c_str());
                  throw(__LINE__);
                }else{
                  //This is an animation, report an error and continue with adding images.
                  CDBError("Unable to load datasource %s at line %d",dataSources[dataSourceToUse]->dataObject[0]->variableName.c_str(),__LINE__);
                }
              }
            }
            if(dataSources[j]->dLayerType==CConfigReaderLayerTypeStyled){
              //Special styled layer for GEOMON project
              status = imageDataWriter.calculateData(dataSources);if(status != 0)throw(__LINE__);
            }
            if(dataSources[dataSourceToUse]->getNumTimeSteps()>1){
              //Print the animation data into the image
              char szTemp[1024];
              snprintf(szTemp,1023,"%s UTC",dataSources[dataSourceToUse]->timeSteps[k]->timeString.c_str());
              imageDataWriter.setDate(szTemp);
            }
          }
          
          int textY=5;
          //int prevTextY=0;
          if(srvParam->mapTitle.length()>0){
            if(srvParam->cfg->WMS[0]->TitleFont.size()==1){
              float fontSize=parseFloat(srvParam->cfg->WMS[0]->TitleFont[0]->attr.size.c_str());
              textY+=int(fontSize);
              imageDataWriter.drawImage.drawText(5,textY,srvParam->cfg->WMS[0]->TitleFont[0]->attr.location.c_str(),fontSize,0,srvParam->mapTitle.c_str(),CColor(0,0,0,255),CColor(255,255,255,100));
              textY+=12;
            }
          }
          if(srvParam->mapSubTitle.length()>0){
            if(srvParam->cfg->WMS[0]->SubTitleFont.size()==1){
              float fontSize=parseFloat(srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.size.c_str());
              textY+=int(fontSize);
              imageDataWriter.drawImage.drawText(6,textY,srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.location.c_str(),fontSize,0,srvParam->mapSubTitle.c_str(),CColor(0,0,0,255),CColor(255,255,255,100));
              textY+=8;
            }
          }

          if(srvParam->showDimensionsInImage){
            textY+=4;
            for(size_t d=0;d<srvParam->requestDims.size();d++){
              CT::string message;
              float fontSize=parseFloat(srvParam->cfg->WMS[0]->DimensionFont[0]->attr.size.c_str());
              textY+=int(fontSize*1.2);
              message.print("%s: %s",srvParam->requestDims[d]->name.c_str(),srvParam->requestDims[d]->value.c_str());
              imageDataWriter.drawImage.drawText(6,textY,srvParam->cfg->WMS[0]->DimensionFont[0]->attr.location.c_str(),fontSize,0,message.c_str(),CColor(0,0,0,255),CColor(255,255,255,100));
              textY+=4;
            }
          }
          
          if(srvParam->showLegendInImage){
            //Draw legend
            bool legendDrawn = false;
            for(size_t d=0;d<dataSources.size()&&legendDrawn==false;d++){
              if(dataSources[d]->dLayerType!=CConfigReaderLayerTypeCascaded){
                if(1==0){
                  CDrawImage legendImage;
                  legendImage.createImage(&imageDataWriter.drawImage,LEGEND_WIDTH,imageDataWriter.drawImage.Geo->dHeight-8);
                  status = imageDataWriter.createLegend(dataSources[d],&legendImage);if(status != 0)throw(__LINE__);
                  int posX=4;
                  int posY=imageDataWriter.drawImage.Geo->dHeight-(legendImage.Geo->dHeight+4);
                  imageDataWriter.drawImage.rectangle(posX,posY,legendImage.Geo->dWidth+posX+1,legendImage.Geo->dHeight+posY+1,CColor(255,255,255,100),CColor(255,255,255,0));
                  imageDataWriter.drawImage.draw(posX,posY,0,0,&legendImage);
                  legendDrawn=true;
                }
                if(1==1){
                  int padding=1;
                  int minimumLegendWidth=80;
                  CDrawImage legendImage;
                  int legendWidth = imageDataWriter.drawImage.Geo->dWidth/10;
                  if(legendWidth<minimumLegendWidth)legendWidth=minimumLegendWidth;
                  legendImage.createImage(&imageDataWriter.drawImage,legendWidth,imageDataWriter.drawImage.Geo->dHeight-padding*2+2);
                  status = imageDataWriter.createLegend(dataSources[d],&legendImage);if(status != 0)throw(__LINE__);
                  int posX=imageDataWriter.drawImage.Geo->dWidth-(legendImage.Geo->dWidth+padding);
                  int posY=imageDataWriter.drawImage.Geo->dHeight-(legendImage.Geo->dHeight+padding);
                  imageDataWriter.drawImage.rectangle(posX,posY,legendImage.Geo->dWidth+posX+1,legendImage.Geo->dHeight+posY+1,CColor(255,255,255,180),CColor(255,255,255,0));
                  imageDataWriter.drawImage.draw(posX+1,posY+1,0,0,&legendImage);
                  legendDrawn=true;
                  
                }
              }
            }
          }
          if(srvParam->showNorthArrow){

          }
          status = imageDataWriter.end();if(status != 0)throw(__LINE__);
          fclose(stdout);
        }
    
        if(srvParam->requestType==REQUEST_WCS_GETCOVERAGE){
    #ifdef ADAGUC_USE_GDAL
          CGDALDataWriter GDALDataWriter;
          status = GDALDataWriter.init(srvParam,dataSources[j],dataSources[j]->getNumTimeSteps());if(status != 0)throw(__LINE__);
          for(int k=0;k<dataSources[j]->getNumTimeSteps();k++){
            dataSources[j]->setTimeStep(k);
            status = GDALDataWriter.addData(dataSources);if(status != 0)throw(__LINE__);
          }
          status = GDALDataWriter.end();if(status != 0)throw(__LINE__);
    #endif
        }
    
        if(srvParam->requestType==REQUEST_WMS_GETFEATUREINFO){
          CImageDataWriter imageDataWriter;
          
          status = imageDataWriter.init(srvParam,dataSources[j],dataSources[j]->getNumTimeSteps());if(status != 0)throw(__LINE__);
         
//             for(int k=0;k<dataSources[j]->getNumTimeSteps();k++){
//               for(size_t d=0;d<dataSources.size();d++){
//                 dataSources[d]->setTimeStep(k);
//               }
              status = imageDataWriter.getFeatureInfo(dataSources,0,int(srvParam->dX),int(srvParam->dY));if(status != 0)throw(__LINE__);
//             }
          
          
          status = imageDataWriter.end();if(status != 0)throw(__LINE__);
        }
        
        // WMS Getlegendgraphic
        if(srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
          CImageDataWriter imageDataWriter;
          
          
          status = imageDataWriter.init(srvParam,dataSources[j],1);if(status != 0)throw(__LINE__);
         //imageDataWriter.drawImage.crop(10,10);
          status = imageDataWriter.createLegend(dataSources[j],&imageDataWriter.drawImage);if(status != 0)throw(__LINE__);
          status = imageDataWriter.end();if(status != 0)throw(__LINE__);
        }
        // WMS GetMetaData
        if(srvParam->requestType==REQUEST_WMS_GETMETADATA){
          printf("%s%c%c\n","Content-Type:text/plain",13,10);
          //printf("%s",dataSources[j]->getFileName());
          CDataReader reader;
          status = reader.open(dataSources[j],CNETCDFREADER_MODE_OPEN_HEADER);
          if(status!=0){
            CDBError("Could not open file: %s",dataSources[j]->getFileName());
            throw(__LINE__);
          }
          
          CT::string dumpString=CDF::dump(dataSources[j]->dataObject[0]->cdfObject);
          
          printf("%s",dumpString.c_str());
          reader.close();

        }
      }
      catch(int i){
        //Exception thrown: Cleanup and return;
        CDBError("Returning from line %d",i);
        return 1;
      }
    }else{
      CDBError("Unknown layer type");
    }
  //}

return 0;
}

int CRequest::process_querystring(){
  #ifdef MEASURETIME
  StopWatch_Stop("Start processing query string");
  #endif
//  StopWatch_Time("render()");
  //First try to find all possible dimensions
  //std::vector
 /* for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
    for(size_t d=0;d<srvParam->cfg->Layer[j]->Dimension.size();d++){
      CT::string *dim = new CT::string(srvParam->cfg->Layer[j]->Dimension[d]->value.c_str());
      
      dim->toUpperCaseSelf();
      
      bool foundDim=false;
      for(size_t i=0;i<queryDims.size();i++){
        if(dim->equals(queryDims[i])){foundDim=true;break;}
      }
      if(foundDim==false){
        queryDims.push_back(dim);
      }else delete dim;
    }
  }
  */
  if(srvParam->cfg->WMS.size()!=1){
    CDBError("WMS element has not been configured");
    return 1;
  }
  if(srvParam->cfg->WCS.size()!=1){
    CDBError("WCS element has not been configured");
    return 1;
  }
 
  seterrormode(EXCEPTIONS_PLAINTEXT);
  CT::string SERVICE,REQUEST;

  int dFound_Width=0;
  int dFound_Height=0;
  int dFound_X=0;
  int dFound_Y=0;
  int dFound_I=0;
  int dFound_J=0;
  int dFound_RESX=0;
  int dFound_RESY=0;
  int dFound_BBOX=0;
  int dFound_SRS=0;
  int dFound_CRS=0;
  //int dFound_Debug=0;
  int dFound_Request=0;
  int dFound_Service=0;
  int dFound_Format=0;
  int dFound_InfoFormat=0;
  int dFound_Transparent=0;
  int dFound_BGColor=0;
  int dErrorOccured=0;
  int dFound_WMSLAYERS=0;
  int dFound_WMSLAYER=0;
  int dFound_WCSCOVERAGE=0;
  int dFound_Version=0;
  int dFound_Exceptions=0;
  int dFound_Styles=0;
  int dFound_Style=0;
  int dFound_JSONP=0;
  int dFound_Dataset=0;
  

  
  int dFound_autoResourceLocation=0;
  //int dFound_OpenDAPVariable=0;
  
  const char * pszQueryString=getenv("QUERY_STRING");
  
  
  
  if(pszQueryString==NULL){
    pszQueryString=strdup("SERVICE=WMS&request=getcapabilities");
    CGI=0;
  }else CGI=1;

  CT::string queryString(pszQueryString);
  queryString.decodeURLSelf();
  //CDBDebug("QueryString: \"%s\"",queryString.c_str());
  CT::string * parameters=queryString.splitToArray("&");
  CT::string value0Cap;
  for(size_t j=0;j<parameters->count;j++){
    CT::string values[2];
   
    int equalPos = parameters[j].indexOf("=");//splitToArray("=");

    if(equalPos!=-1){
      
      values[0] = parameters[j].substringr(0,equalPos);
      values[1] = parameters[j].c_str()+equalPos+1;
      values[0].count = 2;
    }else{
      
      values[0] = parameters[j].c_str();
      values[1] = "";
      values[0].count = 1;
    }

  
    //values[1] = value
    //=parameters[j].splitToArray("=");

    // Styles parameter
    value0Cap.copy(&values[0]);
    value0Cap.toUpperCaseSelf();
    
      
    if(value0Cap.equals("STYLES")){
      if(dFound_Styles==0){
        if(values->count==2&&values[1].length()>0){
          srvParam->Styles.copy(&values[1]);
        }else srvParam->Styles.copy("");
        dFound_Styles=1;
      }else{
        CDBWarning("Styles already defined");
        dErrorOccured=1;
      }
    }
    // Style parameter
    if(value0Cap.equals("STYLE")){
      if(dFound_Style==0){
        if(values->count==2&&values[1].length()>0){
          srvParam->Style.copy(&values[1]);
        }else srvParam->Style.copy("");
        dFound_Style=1;
      }else{
        CDBWarning("Style already defined");
        dErrorOccured=1;
      }
    }
    if(values->count>=2){
      // BBOX Parameters
      if(value0Cap.equals("BBOX")){
        CT::string * bboxvalues=values[1].splitToArray(",");
        if(bboxvalues->count==4){
          for(int j=0;j<4;j++){
            srvParam->Geo->dfBBOX[j]=atof(bboxvalues[j].c_str());
          }
        }else{
          CDBWarning("Invalid BBOX values");
          dErrorOccured=1;
        }
        delete[] bboxvalues;
        dFound_BBOX=1;
      }
      
      if(value0Cap.equals("FIGWIDTH")){
        srvParam->figWidth=atoi(values[1].c_str());
        if(srvParam->figWidth<1)srvParam->figWidth=-1;
      }
      if(value0Cap.equals("FIGHEIGHT")){
        srvParam->figHeight=atoi(values[1].c_str());
        if(srvParam->figHeight<1)srvParam->figHeight=-1;
      }
      
      // Width Parameters
      if(value0Cap.equals("WIDTH")){
        srvParam->Geo->dWidth=atoi(values[1].c_str());
        if(srvParam->Geo->dWidth<1){
          CDBWarning("Parameter Width should be at least 1");
          dErrorOccured=1;
        }
        dFound_Width=1;
      }
      // Height Parameters
      if(value0Cap.equals("HEIGHT")){
        srvParam->Geo->dHeight=atoi(values[1].c_str());
        if(srvParam->Geo->dHeight<1){
          CDBWarning("Parameter Height should be at least 1");
          dErrorOccured=1;
        }

        dFound_Height=1;
      }
      // RESX Parameters
      if(value0Cap.equals("RESX")){
        srvParam->dfResX=atof(values[1].c_str());
        if(srvParam->dfResX==0){
          CDBWarning("Parameter RESX should not be zero");
          dErrorOccured=1;
        }
        dFound_RESX=1;
      }
      // RESY Parameters
      if(value0Cap.equals("RESY")){
        srvParam->dfResY=atof(values[1].c_str());
        if(srvParam->dfResY==0){
          CDBWarning("Parameter RESY should not be zero");
          dErrorOccured=1;
        }
        dFound_RESY=1;
      }

      // X/I Parameters
      if(strncmp(value0Cap.c_str(),"X",1)==0&&value0Cap.length()==1){
        srvParam->dX=atof(values[1].c_str());
        dFound_X=1;
      }
      if(strncmp(value0Cap.c_str(),"I",1)==0&&value0Cap.length()==1){
        srvParam->dX=atof(values[1].c_str());
        dFound_I=1;
      }
      // Y/J Parameter
      if(strncmp(value0Cap.c_str(),"Y",1)==0&&value0Cap.length()==1){
        srvParam->dY=atof(values[1].c_str());
        dFound_Y=1;
      }
      if(strncmp(value0Cap.c_str(),"J",1)==0&&value0Cap.length()==1){
        srvParam->dY=atof(values[1].c_str());
        dFound_J=1;
      }
      // SRS / CRS Parameters
      if(value0Cap.equals("SRS")){
        if(parameters[j].length()>5){
          srvParam->Geo->CRS.copy(parameters[j].c_str()+4);
          srvParam->Geo->CRS.decodeURLSelf();
          dFound_SRS=1;
        }
      }
      if(value0Cap.equals("CRS")){
        if(parameters[j].length()>5){
          srvParam->Geo->CRS.copy(parameters[j].c_str()+4);
          srvParam->Geo->CRS.decodeURLSelf();
          dFound_CRS=1;
        }
      }
      //DIM Params
      int foundDim=-1;
      if(value0Cap.equals("TIME")||value0Cap.equals("ELEVATION")){
        foundDim=0;
      }else if(value0Cap.indexOf("DIM_")==0){
        //We store the OGCdim without the DIM_ prefix
        foundDim=4;
      }
      if(foundDim!=-1){
        COGCDims *ogcDim = NULL;
        const char *ogcDimName=value0Cap.c_str()+foundDim;
        for(size_t j=0;j<srvParam->requestDims.size();j++){if(srvParam->requestDims[j]->name.equals(ogcDimName)){ogcDim = srvParam->requestDims[j];break;}}
        if(ogcDim==NULL){ogcDim = new COGCDims();srvParam->requestDims.push_back(ogcDim);}else {CDBDebug("OGC Dim %s reused",ogcDimName);}
        ogcDim->name.copy(ogcDimName);
        ogcDim->value.copy(&values[1]);
        
      }

      // FORMAT parameter
      if(value0Cap.equals("FORMAT")){
        if(dFound_Format==0){
          if(values[1].length()>1){
            srvParam->Format.copy(&values[1]);
            dFound_Format=1;
          }
        }else{
          CDBWarning("FORMAT already defined");
          dErrorOccured=1;
        }
      }
      
      // INFO_FORMAT parameter
      if(value0Cap.equals("INFO_FORMAT")){
        if(dFound_InfoFormat==0){
          if(values[1].length()>1){
            srvParam->InfoFormat.copy(&values[1]);
            dFound_InfoFormat=1;
          }
        }else{
          CDBWarning("INFO_FORMAT already defined");
          dErrorOccured=1;
        }
      }

      // TRANSPARENT parameter
      if(value0Cap.equals("TRANSPARENT")){
        if(dFound_Transparent==0){
          if(values[1].length()>1){
            values[1].toUpperCaseSelf();
            if(values[1].equals("TRUE")){
              srvParam->Transparent=true;
            }
            dFound_Transparent=1;
          }
        }else{
          CDBWarning("TRANSPARENT already defined");
          dErrorOccured=1;
        }
      }
      // BGCOLOR parameter
      if(value0Cap.equals("BGCOLOR")){
        if(dFound_BGColor==0){
          if(values[1].length()>1){
            srvParam->BGColor.copy(&values[1]);
            dFound_BGColor=1;
          }
        }else{
          CDBWarning("FORMAT already defined");
          dErrorOccured=1;
        }
      }
      
      
      // Version parameter
      if(value0Cap.equals("VERSION")){
        if(dFound_Version==0){
          if(values[1].length()>1){
            Version.copy(&values[1]);
            dFound_Version=1;
          }
        }
        //ARCGIS user Friendliness, version can be defined multiple times.
        /*else{
          CDBWarning("Version already defined");
          dErrorOccured=1;
        }*/
      }

      // Exceptions parameter
      if(value0Cap.equals("EXCEPTIONS")){
        if(dFound_Exceptions==0){
          if(values[1].length()>1){
            Exceptions.copy(&values[1]);
            dFound_Exceptions=1;
          }
        }else{
          CDBWarning("Exceptions already defined");
          dErrorOccured=1;
        }
      }


      //Opendap source parameter
      if(dFound_autoResourceLocation==0){
        if(value0Cap.equals("SOURCE")){
          if(srvParam->autoResourceLocation.empty()){
            srvParam->autoResourceLocation.copy(values[1].c_str());
          }
          dFound_autoResourceLocation=1;
        }
      }
      
      if(dFound_Dataset==0){
        if(value0Cap.equals("DATASET")){
          if(srvParam->datasetLocation.empty()){
            srvParam->datasetLocation.copy(values[1].c_str());
          }
          dFound_Dataset=1;
        }
      }
     /* //Opendap variable parameter
       if(dFound_OpenDAPVariable==0){
        if(value0Cap.equals("VARIABLE")){
          if(srvParam->autoResourceVariable.empty()){
            srvParam->autoResourceVariable.copy(values[1].c_str());
          }
          dFound_OpenDAPVariable=1;
        }
      }*/
      
      
      //WMS Layers parameter
      if(value0Cap.equals("LAYERS")){
        if(srvParam->WMSLayers!=NULL)
          delete[] srvParam->WMSLayers;
          srvParam->WMSLayers = values[1].splitToArray(",");
        dFound_WMSLAYERS=1;
      }
      //WMS Layer parameter
      if(value0Cap.equals("LAYER")){
        if(srvParam->WMSLayers!=NULL)
          delete[] srvParam->WMSLayers;
          srvParam->WMSLayers = values[1].splitToArray(",");
        dFound_WMSLAYER=1;
      }

    //WMS Layer parameter
      if(value0Cap.equals("QUERY_LAYERS")){
        if(srvParam->WMSLayers!=NULL)
          delete[] srvParam->WMSLayers;
          srvParam->WMSLayers = values[1].splitToArray(",");
          /*if(srvParam->WMSLayers->count>1){
            CDBError("Too many layers in request");
            dErrorOccured=1;
          }*/
        dFound_WMSLAYER=1;
    }
      //WCS Coverage parameter
    if(value0Cap.equals("COVERAGE")){
        if(srvParam->WMSLayers!=NULL){
          CDBWarning("COVERAGE already defined");
          dErrorOccured=1;
        }else{
          srvParam->WMSLayers = values[1].splitToArray(",");
        }
        dFound_WCSCOVERAGE=1;
      }

      // Service parameters
      if(value0Cap.equals("SERVICE")){
        values[1].toUpperCaseSelf();
        SERVICE.copy(values[1].c_str(),values[1].length());
        dFound_Service=1;
      }
      // Request parameters
      if(value0Cap.equals("REQUEST")){
        values[1].toUpperCaseSelf();
        REQUEST.copy(values[1].c_str(),values[1].length());
        dFound_Request=1;
      }


      // debug Parameters
      if(value0Cap.equals("DEBUG")){
        if(values[1].equals("ON")){
          printf("%s%c%c\n","Content-Type:text/plain",13,10);
          printf("Debug mode:ON\nDebug messages:<br>\r\n\r\n");
          //dFound_Debug=1;
        }
      }
      
      if(value0Cap.equals("TITLE")){
        if(values[1].length()>0){
          srvParam->mapTitle = values[1].c_str();
        }
      }
      if(value0Cap.equals("SUBTITLE")){
        if(values[1].length()>0){
          srvParam->mapSubTitle = values[1].c_str();
        }
      }
      if(value0Cap.equals("SHOWDIMS")){
        values[1].toLowerCaseSelf();
        if(values[1].equals("true")){
          srvParam->showDimensionsInImage = true;
        }
      }
      if(value0Cap.equals("SHOWLEGEND")){
        values[1].toLowerCaseSelf();
        if(values[1].equals("true")){
          srvParam->showLegendInImage = true;
        }
      }
      if(value0Cap.equals("SHOWNORTHARROW")){
        values[1].toLowerCaseSelf();
        if(values[1].equals("true")){
          srvParam->showNorthArrow = true;
        }
      }
      
      //http://www.resc.rdg.ac.uk/trac/ncWMS/wiki/WmsExtensions
      if(value0Cap.equals("OPACITY")){
        srvParam->wmsExtensions.opacity = values[1].toDouble();
      }  
      if(value0Cap.equals("COLORSCALERANGE")){
        CT::string *valuesC=values[1].splitToArray(",");
        if(valuesC->count==2){
          srvParam->wmsExtensions.colorScaleRangeMin = valuesC[0].toDouble();
          srvParam->wmsExtensions.colorScaleRangeMax = valuesC[1].toDouble();
          srvParam->wmsExtensions.colorScaleRangeSet=true;
          
        }
        delete[] valuesC;
      }  
      if(value0Cap.equals("NUMCOLORBANDS")){
        srvParam->wmsExtensions.numColorBands = values[1].toInt();
      }  
      if(value0Cap.equals("LOGSCALE")){
        values[1].toLowerCaseSelf();
        if(values[1].equals("true")){
          srvParam->wmsExtensions.logScale = true;
        }
      } 
      // JSONP parameter
      if(value0Cap.equals("JSONP")){
        if(dFound_JSONP==0){
          if(values[1].length()>1){
            srvParam->JSONP.copy(&values[1]);
            dFound_JSONP=1;
          }
        }else{
          CDBWarning("JSONP already defined");
          dErrorOccured=1;
        }
      }

      
    }

  }
  delete[] parameters;

  #ifdef MEASURETIME
  StopWatch_Stop("query string processed");
  #endif

  if(dFound_Service==0){
    CDBWarning("Parameter SERVICE missing");
    dErrorOccured=1;
  }
  if(dFound_Styles==0){
    srvParam->Styles.copy("");
  }
  if(SERVICE.equals("WMS"))srvParam->serviceType=SERVICE_WMS;
  if(SERVICE.equals("WCS"))srvParam->serviceType=SERVICE_WCS;
  
  if(dErrorOccured==0&&srvParam->serviceType==SERVICE_WMS){
    
    
    
    //Default is 1.3.0
    
    srvParam->OGCVersion=WMS_VERSION_1_3_0;
    
    if(dFound_Request==0){
      CDBWarning("Parameter REQUEST missing");
      dErrorOccured=1;
    }else{
      if(REQUEST.equals("GETCAPABILITIES"))srvParam->requestType=REQUEST_WMS_GETCAPABILITIES;
      if(REQUEST.equals("GETMAP"))srvParam->requestType=REQUEST_WMS_GETMAP;
      if(REQUEST.equals("GETFEATUREINFO"))srvParam->requestType=REQUEST_WMS_GETFEATUREINFO;
      if(REQUEST.equals("GETPOINTVALUE"))srvParam->requestType=REQUEST_WMS_GETPOINTVALUE;
      if(REQUEST.equals("GETLEGENDGRAPHIC"))srvParam->requestType=REQUEST_WMS_GETLEGENDGRAPHIC;
      if(REQUEST.equals("GETMETADATA"))srvParam->requestType=REQUEST_WMS_GETMETADATA;
      if(REQUEST.equals("GETSTYLES"))srvParam->requestType=REQUEST_WMS_GETSTYLES;
      if(REQUEST.equals("GETREFERENCETIMES"))srvParam->requestType=REQUEST_WMS_GETREFERENCETIMES;
    }
    
    //For getlegend graphic the parameter is style, not styles
    if(dFound_Style==0){
      srvParam->Style.copy("");
    }else{
      //For getlegend graphic the parameter is style, not styles
      if(srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
        srvParam->Styles.copy(&srvParam->Style);
      }
    }
    
    seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
    // Check the version
    if(dFound_Version!=0){
      srvParam->OGCVersion=-1;//WMS_VERSION_1_1_1;
      if(Version.equals("1.0.0"))srvParam->OGCVersion=WMS_VERSION_1_0_0;
      if(Version.equals("1.1.1"))srvParam->OGCVersion=WMS_VERSION_1_1_1;
      if(Version.equals("1.3.0"))srvParam->OGCVersion=WMS_VERSION_1_3_0;
      if(srvParam->OGCVersion==-1){
        CDBError("Invalid version ('%s'): WMS 1.0.0, WMS 1.1.1 and WMS 1.3.0 are supported",Version.c_str());
        dErrorOccured=1;
      }
    }
    // Set the exception response
    if(srvParam->OGCVersion==WMS_VERSION_1_0_0){
      seterrormode(EXCEPTIONS_PLAINTEXT);
      if(srvParam->requestType==REQUEST_WMS_GETMAP)seterrormode(WMS_EXCEPTIONS_IMAGE);
      if(srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC)seterrormode(WMS_EXCEPTIONS_IMAGE);
    }
    
    if(srvParam->OGCVersion==WMS_VERSION_1_1_1){
      seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
    }
    
    if(srvParam->OGCVersion==WMS_VERSION_1_3_0){
      seterrormode(WMS_EXCEPTIONS_XML_1_3_0);
      
      if(srvParam->checkBBOXXYOrder(NULL)==true){
        //BBOX swap
        double dfBBOX[4];
        for(int j=0;j<4;j++){
          dfBBOX[j] = srvParam->Geo->dfBBOX[j];
        }
        srvParam->Geo->dfBBOX[0] = dfBBOX[1];
        srvParam->Geo->dfBBOX[1] = dfBBOX[0];
        srvParam->Geo->dfBBOX[2] = dfBBOX[3];
        srvParam->Geo->dfBBOX[3] = dfBBOX[2];
      }
      
    }
    
    if(dFound_Exceptions!=0){
      if(Exceptions.equals("application/vnd.ogc.se_xml")){
        if(srvParam->OGCVersion==WMS_VERSION_1_1_1)seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
      }
      if(Exceptions.equals("application/vnd.ogc.se_inimage")){
        seterrormode(WMS_EXCEPTIONS_IMAGE);
      }
      if(Exceptions.equals("application/vnd.ogc.se_blank")){
        seterrormode(WMS_EXCEPTIONS_BLANKIMAGE);
      }
      if(Exceptions.equals("INIMAGE")){
        seterrormode(WMS_EXCEPTIONS_IMAGE);
      }
      if(Exceptions.equals("BLANK")){
        seterrormode(WMS_EXCEPTIONS_BLANKIMAGE);
      }
    }
  }
  
  
    //Configure the server based an an available dataset
    if(srvParam->datasetLocation.empty()==false&&dErrorOccured==0){
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
      int status = srvParam->configObj->parseFile(datasetConfigFile.c_str());
      if(status!=0){
        CDBError("Invalid dataset configuration file. ");
        return 1;
      }
      
      //Adjust online resource in order to pass on dataset parameters
      CT::string onlineResource=srvParam->cfg->OnlineResource[0]->attr.value.c_str();
      CT::string stringToAdd;
      stringToAdd.concat("dataset=");stringToAdd.concat(srvParam->datasetLocation.c_str());
      stringToAdd.concat("&amp;");
      onlineResource.concat(stringToAdd.c_str());
      srvParam->cfg->OnlineResource[0]->attr.value.copy(onlineResource.c_str());
     
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
      
      //Error messages should be the unique for different 'dir' attempts, otherwise someone can find out directory structures
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
        CDFObject * cdfObject =  CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(srvParam,srvParam->internalAutoResourceLocation.c_str());
        //int status=cdfObject->open(srvParam->internalAutoResourceLocation.c_str());
        if(cdfObject!=NULL){
          for(size_t j=0;j<cdfObject->variables.size();j++){
            if(cdfObject->variables[j]->dimensionlinks.size()>=2){
              if(cdfObject->variables[j]->getAttributeNE("ADAGUC_SKIP")==NULL){
                if(!cdfObject->variables[j]->name.equals("lon")&&
                  !cdfObject->variables[j]->name.equals("lat")&&
                  !cdfObject->variables[j]->name.equals("lon_bounds")&&
                  !cdfObject->variables[j]->name.equals("lat_bounds")&&
                  !cdfObject->variables[j]->name.equals("time_bounds")&&
                  !cdfObject->variables[j]->name.equals("lon_bnds")&&
                  !cdfObject->variables[j]->name.equals("lat_bnds")&&
                  !cdfObject->variables[j]->name.equals("time_bnds")&&
                  !cdfObject->variables[j]->name.equals("time")){
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
      CDFObject * cdfObject =  CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(srvParam,srvParam->internalAutoResourceLocation.c_str());
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
      
      CT::string serverAbstract="";
      if(serverInstitution.length()>0){serverAbstract.printconcat("Institution: %s.\n",serverInstitution.c_str());}
      if(serverSummary.length()>0){serverAbstract.printconcat("Summary: %s.\n",serverSummary.c_str());}
      if(serverDescription.length()>0){serverAbstract.printconcat("Description: '%s'.\n",serverDescription.c_str());}
      if(serverSource.length()>0){serverAbstract.printconcat("Source: %s.\n",serverSource.c_str());}
      if(serverReferences.length()>0){serverAbstract.printconcat("References: %s.\n",serverReferences.c_str());}
      if(serverHistory.length()>0){serverAbstract.printconcat("History: %s.\n",serverHistory.c_str());}
      if(serverComments.length()>0){serverAbstract.printconcat("Comments: %s.\n",serverComments.c_str());}
      if(serverDisclaimer.length()>0){serverAbstract.printconcat("Disclaimer: %s.\n",serverDisclaimer.c_str());}

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
     
    
      //Generate layers based on the OpenDAP variables
      CT::StackList<CT::string> variables=srvParam->autoResourceVariable.splitToStack(",");
      for(size_t j=0;j<variables.size();j++){
        std::vector<CT::string> variableNames;
        variableNames.push_back(variables[j].c_str());
        addXMLLayerToConfig(srvParam,&variableNames,NULL,srvParam->internalAutoResourceLocation.c_str());
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
          addXMLLayerToConfig(srvParam,&variableNames,"derived",srvParam->internalAutoResourceLocation.c_str());
        }
      }
      
      //Detect dd and ff for wind direction and wind speed
      if(1==1){
        CDF::Variable *varSpeed = cdfObject->getVariableNE("ff");
        CDF::Variable *varDirection = cdfObject->getVariableNE("dd");
        if(varSpeed!=NULL&&varDirection!=NULL){
          std::vector<CT::string> variableNames;
          variableNames.push_back(varSpeed->name.c_str());
          variableNames.push_back(varDirection->name.c_str());
          addXMLLayerToConfig(srvParam,&variableNames,"derived",srvParam->internalAutoResourceLocation.c_str());
        }
      }
      
      //Adjust online resource in order to pass on variable and source parameters
      CT::string onlineResource=srvParam->cfg->OnlineResource[0]->attr.value.c_str();
      CT::string stringToAdd;
      stringToAdd.concat("&source=");stringToAdd.concat(srvParam->autoResourceLocation.c_str());
      //stringToAdd.concat("&variable=");stringToAdd.concat(srvParam->autoResourceVariable.c_str());
      stringToAdd.concat("&");
      stringToAdd.encodeURLSelf();
      onlineResource.concat(stringToAdd.c_str());
      srvParam->cfg->OnlineResource[0]->attr.value.copy(onlineResource.c_str());
      //CDBDebug("OGC REQUEST RESOURCE %s",srvParam->internalAutoResourceLocation.c_str());//,srvParam->autoResourceLocation.c_str(),);
      
      
      
   
      #ifdef MEASURETIME
      StopWatch_Stop("Auto opendap configured");
      #endif
      
    }
  
  // WMS Service
  if(dErrorOccured==0&&srvParam->serviceType==SERVICE_WMS){

    
    
    //Default is 1.1.1
/*
    srvParam->OGCVersion=WMS_VERSION_1_1_1;

    if(dFound_Request==0){
      CDBWarning("Parameter REQUEST missing");
      dErrorOccured=1;
    }else{
      if(REQUEST.equals("GETCAPABILITIES"))srvParam->requestType=REQUEST_WMS_GETCAPABILITIES;
      if(REQUEST.equals("GETMAP"))srvParam->requestType=REQUEST_WMS_GETMAP;
      if(REQUEST.equals("GETFEATUREINFO"))srvParam->requestType=REQUEST_WMS_GETFEATUREINFO;
      if(REQUEST.equals("GETPOINTVALUE"))srvParam->requestType=REQUEST_WMS_GETPOINTVALUE;
      if(REQUEST.equals("GETLEGENDGRAPHIC"))srvParam->requestType=REQUEST_WMS_GETLEGENDGRAPHIC;
      if(REQUEST.equals("GETMETADATA"))srvParam->requestType=REQUEST_WMS_GETMETADATA;
      if(REQUEST.equals("GETSTYLES"))srvParam->requestType=REQUEST_WMS_GETSTYLES;
    }
    
    //For getlegend graphic the parameter is style, not styles
    if(dFound_Style==0){
      srvParam->Style.copy("");
    }else{
    //For getlegend graphic the parameter is style, not styles
      if(srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
        srvParam->Styles.copy(&srvParam->Style);
      }
    }

    // Check the version
    if(dFound_Version!=0){
      srvParam->OGCVersion=WMS_VERSION_1_1_1;
      if(Version.equals("1.0.0"))srvParam->OGCVersion=WMS_VERSION_1_0_0;
      if(Version.equals("1.1.1"))srvParam->OGCVersion=WMS_VERSION_1_1_1;
      if(srvParam->OGCVersion==-1){
        CDBError("Invalid version ('%s'): only WMS 1.0.0 and WMS 1.1.1 supported",Version.c_str());
        dErrorOccured=1;
      }
    }
    // Set the exception response
    if(srvParam->OGCVersion==WMS_VERSION_1_0_0){
      seterrormode(EXCEPTIONS_PLAINTEXT);
      if(srvParam->requestType==REQUEST_WMS_GETMAP)seterrormode(WMS_EXCEPTIONS_IMAGE);
      if(srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC)seterrormode(WMS_EXCEPTIONS_IMAGE);
    }
    if(srvParam->OGCVersion==WMS_VERSION_1_1_1)seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
    if(dFound_Exceptions!=0){
      if(Exceptions.equals("application/vnd.ogc.se_xml")){
        if(srvParam->OGCVersion==WMS_VERSION_1_1_1)seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
      }
      if(Exceptions.equals("application/vnd.ogc.se_inimage")){
        seterrormode(WMS_EXCEPTIONS_IMAGE);
      }
      if(Exceptions.equals("application/vnd.ogc.se_blank")){
        seterrormode(WMS_EXCEPTIONS_BLANKIMAGE);
      }
    }*/
    
    if (srvParam->requestType==REQUEST_WMS_GETREFERENCETIMES) {
      int status =  process_wms_getreferencetimes_request();
      if(status != 0) {
	CDBError("WMS GetReferenceTimes Request failed");
	return 1;
      }
      return 0;
    }
    if(srvParam->requestType==REQUEST_WMS_GETMAP||srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
        if(dFound_Format==0){
          CDBWarning("Parameter FORMAT missing");
          dErrorOccured=1;
        }else{
          
          //Mapping
          for(size_t j=0;j<srvParam->cfg->WMS[0]->WMSFormat.size();j++){
            if(srvParam->Format.equals(srvParam->cfg->WMS[0]->WMSFormat[j]->attr.name.c_str())){
              if(srvParam->cfg->WMS[0]->WMSFormat[j]->attr.format.empty()==false){
                srvParam->Format.copy(srvParam->cfg->WMS[0]->WMSFormat[j]->attr.format.c_str());
              }
              break;
            }
          }
    
          // Set format
          //CDBDebug("FORMAT: %s",srvParam->Format.c_str());
          if(srvParam->Format.indexOf("24")>0){srvParam->imageFormat=IMAGEFORMAT_IMAGEPNG32;srvParam->imageMode=SERVERIMAGEMODE_RGBA;}
          else if(srvParam->Format.indexOf("32")>0){srvParam->imageFormat=IMAGEFORMAT_IMAGEPNG32;srvParam->imageMode=SERVERIMAGEMODE_RGBA;}
          else if(srvParam->Format.indexOf("8")>0){srvParam->imageFormat=IMAGEFORMAT_IMAGEPNG8;srvParam->imageMode=SERVERIMAGEMODE_8BIT;}
          else if(srvParam->Format.indexOf("gif")>0){srvParam->imageFormat=IMAGEFORMAT_IMAGEGIF;srvParam->imageMode=SERVERIMAGEMODE_8BIT;}
          else if(srvParam->Format.indexOf("GIF")>0){srvParam->imageFormat=IMAGEFORMAT_IMAGEGIF;srvParam->imageMode=SERVERIMAGEMODE_8BIT;}
       
        }
      }
       
    
    if(dErrorOccured==0&&
      (
        srvParam->requestType==REQUEST_WMS_GETMAP||
        srvParam->requestType==REQUEST_WMS_GETFEATUREINFO||
        srvParam->requestType==REQUEST_WMS_GETPOINTVALUE
      )){
      
      if(srvParam->requestType==REQUEST_WMS_GETFEATUREINFO||srvParam->requestType==REQUEST_WMS_GETPOINTVALUE){
        int status = checkDataRestriction();
        if((status&ALLOW_GFI)==false){
          CDBWarning("This layer is not queryable.");
          return 1;
        }
      }
      // Check if styles is defined for WMS 1.1.1
      if(dFound_Styles==0&&srvParam->requestType==REQUEST_WMS_GETMAP){
        if(srvParam->OGCVersion==WMS_VERSION_1_1_1){
          //CDBWarning("Parameter STYLES missing");TODO Google Earth does not provide this!
        }
      }
      
      if(srvParam->requestType==REQUEST_WMS_GETPOINTVALUE){
        //Maps REQUEST_WMS_GETPOINTVALUE to REQUEST_WMS_GETFEATUREINFO
        //http://bhw222.knmi.nl:8080/cgi-bin/model.cgi?&SERVICE=WMS&REQUEST=GetPointValue&VERSION=1.1.1&SRS=EPSG%3A4326&QUERY_LAYERS=PMSL_sfc_0&X=3.74&Y=52.34&INFO_FORMAT=text/html&time=2011-08-18T09:00:00Z/2011-08-18T18:00:00Z&DIM_sfc_snow=0
        dFound_BBOX=1;
        dFound_WMSLAYERS=1;
        dFound_Width=1;
        dFound_Height=1;
        srvParam->Geo->dfBBOX[0]=srvParam->dX;
        srvParam->Geo->dfBBOX[1]=srvParam->dY;
        srvParam->Geo->dfBBOX[2]=srvParam->Geo->dfBBOX[0];
        srvParam->Geo->dfBBOX[3]=srvParam->Geo->dfBBOX[1];
        srvParam->Geo->dWidth=1;
        srvParam->Geo->dHeight=1;
        srvParam->dX=0;
        srvParam->dY=0;
        srvParam->requestType=REQUEST_WMS_GETFEATUREINFO;
      }
   
      
    
      
      if(dFound_BBOX==0){
        CDBWarning("Parameter BBOX missing");
        dErrorOccured=1;
      }
      
      
      if(dFound_Width==0&&dFound_Height==0){
        CDBWarning("Parameter WIDTH or HEIGHT missing");
        dErrorOccured=1;
      }
      
      if(dFound_Width==0){
        float r=fabs(srvParam->Geo->dfBBOX[3]-srvParam->Geo->dfBBOX[1])/fabs(srvParam->Geo->dfBBOX[2]-srvParam->Geo->dfBBOX[0]);
        srvParam->Geo->dWidth=int(float(srvParam->Geo->dHeight)/r);
      }
        
      if(dFound_Height==0){
        float r=fabs(srvParam->Geo->dfBBOX[3]-srvParam->Geo->dfBBOX[1])/fabs(srvParam->Geo->dfBBOX[2]-srvParam->Geo->dfBBOX[0]);
        srvParam->Geo->dHeight=int(float(srvParam->Geo->dWidth)*r);
      }
        
      
      // When error is image, utilize full image size
      setErrorImageSize(srvParam->Geo->dWidth,srvParam->Geo->dHeight,srvParam->imageFormat);
      
      if(srvParam->OGCVersion==WMS_VERSION_1_0_0 || srvParam->OGCVersion==WMS_VERSION_1_1_1){
        if(dFound_SRS==0){
          CDBWarning("Parameter SRS missing");
          dErrorOccured=1;
        }
      }
      
      if(srvParam->OGCVersion==WMS_VERSION_1_3_0 ){
        if(dFound_CRS==0){
          CDBWarning("Parameter CRS missing");
          dErrorOccured=1;
        }
      }
      
      
      if(dFound_WMSLAYERS==0){
        CDBWarning("Parameter LAYERS missing");
        dErrorOccured=1;
      }
      if(dErrorOccured==0){
        if(srvParam->requestType==REQUEST_WMS_GETMAP){
          int status =  process_wms_getmap_request();
          if(status != 0) {
            CDBError("WMS GetMap Request failed");
            return 1;
          }
          return 0;
        }
      
        if(srvParam->requestType==REQUEST_WMS_GETFEATUREINFO){
          if(srvParam->OGCVersion == WMS_VERSION_1_0_0 || srvParam->OGCVersion == WMS_VERSION_1_1_1){
            if(dFound_X==0){
              CDBWarning("Parameter X missing");
              dErrorOccured=1;
            }
            if(dFound_Y==0){
              CDBWarning("Parameter Y missing");
              dErrorOccured=1;
            }
          }
          
          if(srvParam->OGCVersion == WMS_VERSION_1_3_0){
            if(dFound_I==0){
              CDBWarning("Parameter I missing");
              dErrorOccured=1;
            }
            if(dFound_J==0){
              CDBWarning("Parameter J missing");
              dErrorOccured=1;
            }
          }
          
          int status =  process_wms_getfeatureinfo_request();
          if(status != 0) {
            if(status!=2){
              CDBError("WMS GetFeatureInfo Request failed");
            }
            return 1;
          }
          return 0;
        }
      }
    }
    if(dErrorOccured==0&&srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
      if(dFound_WMSLAYER==0){
        CDBWarning("Parameter LAYER missing");
        dErrorOccured=1;
      }
      if(dErrorOccured==0){
        int status =  process_wms_getlegendgraphic_request();
        if(status != 0) {
          CDBError("WMS GetLegendGraphic Request failed");
          return 1;
        }
        return 0;
      }
    }

    if(dErrorOccured==0&&srvParam->requestType==REQUEST_WMS_GETCAPABILITIES){
      int status = process_wms_getcap_request();
      if(status!=0){
        CDBError("GetCapabilities failed");
      }
      return status;
    }
    if(dErrorOccured==0&&srvParam->requestType==REQUEST_WMS_GETMETADATA){
      int status = checkDataRestriction();
      if((status&ALLOW_METADATA)==false){
        CDBWarning("GetMetaData is restricted");
        return 1;
      }
      if(dFound_WMSLAYER==0){
        CDBWarning("Parameter LAYER missing");
        dErrorOccured=1;
      }
      if(dFound_Format==0){
        CDBWarning("Parameter FORMAT missing");
        dErrorOccured=1;
      }
      if(dErrorOccured==0){
        int status =  process_wms_getmetadata_request();
        if(status != 0) {
          CDBError("WMS GetMetaData Request failed");
          return 1;
        }
      }
      return 0;
    }
  }

  if(dErrorOccured==0&&srvParam->serviceType==SERVICE_WCS){
    srvParam->OGCVersion=WCS_VERSION_1_0;
    int status = checkDataRestriction();
    if((status&ALLOW_WCS)==false){
      CDBWarning("WCS Service is disabled.");
      return 1;
    }
    if(dFound_Request==0){
      CDBWarning("Parameter REQUEST missing");
      dErrorOccured=1;
    }else{
      if(REQUEST.equals("GETCAPABILITIES"))srvParam->requestType=REQUEST_WCS_GETCAPABILITIES;
      if(REQUEST.equals("DESCRIBECOVERAGE"))srvParam->requestType=REQUEST_WCS_DESCRIBECOVERAGE;
      if(REQUEST.equals("GETCOVERAGE"))srvParam->requestType=REQUEST_WCS_GETCOVERAGE;
    }

    if(dErrorOccured==0&&srvParam->requestType==REQUEST_WCS_DESCRIBECOVERAGE){
      if(dErrorOccured==0){
        int status =  process_wcs_describecov_request();
        if(status != 0) {
          CDBError("WCS DescribeCoverage Request failed");
        }
      }
      return 0;
    }
    if(dErrorOccured==0&&srvParam->requestType==REQUEST_WCS_GETCOVERAGE){

      if(dFound_Width==0&&dFound_Height==0&&dFound_RESX==0&&dFound_RESY==0&&dFound_BBOX==0&&dFound_CRS==0)srvParam->WCS_GoNative=1;else{
        srvParam->WCS_GoNative = 0;
        if(dFound_RESX==0||dFound_RESY==0){
          if(dFound_Width==0){
            CDBWarning("Parameter WIDTH/RESX missing");
            dErrorOccured=1;
          }
          if(dFound_Height==0){
            CDBWarning("Parameter HEIGHT/RESY missing");
            dErrorOccured=1;
          }
          srvParam->dWCS_RES_OR_WH = 0;
        }else if(dFound_Width==0||dFound_Height==0){
          if(dFound_RESX==0){
            CDBWarning("Parameter RESX missing");
            dErrorOccured=1;
          }
          if(dFound_RESY==0){
            CDBWarning("Parameter RESY missing");
            dErrorOccured=1;
          }
          srvParam->dWCS_RES_OR_WH = 1;
        }
        if(dFound_BBOX==0){
          CDBWarning("Parameter BBOX missing");
          dErrorOccured=1;
        }
        if(dFound_CRS==0){
          CDBWarning("Parameter CRS missing");
          dErrorOccured=1;
        }
        if(dFound_Format==0){
          CDBWarning("Parameter FORMAT missing");
          dErrorOccured=1;
        }
        if(dFound_WCSCOVERAGE==0){
          CDBWarning("Parameter COVERAGE missing");
          dErrorOccured=1;
        }
      }


      if(dErrorOccured==0){
        int status =  process_wcs_getcoverage_request();
        if(status != 0) {
          CDBError("WCS GetCoverage Request failed");
          return 1;
        }
      }
      return 0;
    }
    if(dErrorOccured==0&&srvParam->requestType==REQUEST_WCS_GETCAPABILITIES){
      return process_wcs_getcap_request();
    }
  }
  //An error occured, stopping..
  if(dErrorOccured==1){
    return 0;
  }
  if(srvParam->serviceType==SERVICE_WCS){
    CDBWarning("Invalid value for request. Supported requests are: getcapabilities, describecoverage and getcoverage");
  }else if(srvParam->serviceType==SERVICE_WMS){
    CDBWarning("Invalid value for request. Supported requests are: getcapabilities, getmap, getfeatureinfo, getpointvalue, getmetadata and getlegendgraphic");
  }else{
    CDBWarning("Unknown service");
  }
  

  return 0;
}


int CRequest::updatedb(CT::string *tailPath,CT::string *layerPathToScan){
  int errorHasOccured = 0;
  int status;
  //Fill in all data sources from the configuration object
  size_t numberOfLayers = srvParam->cfg->Layer.size();

  for(size_t layerNo=0;layerNo<numberOfLayers;layerNo++){
    CDataSource *dataSource = new CDataSource ();
    dataSources.push_back(dataSource);
    if(dataSource->setCFGLayer(srvParam,srvParam->configObj->Configuration[0],srvParam->cfg->Layer[layerNo],NULL,layerNo)!=0){
      return 1;
    }
  }

  srvParam->requestType=REQUEST_UPDATEDB;

  for(size_t j=0;j<numberOfLayers;j++){
    if(dataSources[j]->dLayerType==CConfigReaderLayerTypeDataBase){
        status = CDBFileScanner::updatedb(srvParam->cfg->DataBase[0]->attr.parameters.c_str(),dataSources[j],tailPath,layerPathToScan);
        if(status !=0){CDBError("Could not update db for: %s",dataSources[j]->cfgLayer->Name[0]->value.c_str());errorHasOccured++;}
    }
  }

  //invalidate cache
  CT::string cacheFileName;
  srvParam->getCacheFileName(&cacheFileName);
  
  //Remove the cache file, but check first wether it exists or not.
  struct stat stFileInfo;
  int intStat;
  intStat = stat(cacheFileName.c_str(),&stFileInfo);
  CT::string cacheBuffer;
  //The file exists, so remove it.
  if(intStat == 0) {
    CDBDebug("Removing cachefile %s ",cacheFileName.c_str());
    if(cacheFileName.length()>0){
      if(remove(cacheFileName.c_str())!=0){
        CDBError("Unable to remove cachefile %s, please do it manually.",cacheFileName.c_str());
      }
    }
  }
  /*
  CSimpleStore simpleStore;
  status = getDocFromDocCache(&simpleStore,NULL,NULL);  
  simpleStore.setStringAttribute("configModificationDate","needsupdate!");
  if(storeDocumentCache(&simpleStore)!=0)return 1;*/
  if(errorHasOccured){
    CDBDebug("***** Finished DB Update with %d errors *****",errorHasOccured);
  }else{
    CDBDebug("***** Finished DB Update *****");
  }
  
  CDFObjectStore::getCDFObjectStore()->clear();
  return errorHasOccured;
}



int CRequest::getDocumentCacheName(CT::string *documentName,CServerParams *srvParam){
  documentName->copy("none");
  if(srvParam->requestType==REQUEST_WMS_GETCAPABILITIES){
    if(srvParam->OGCVersion==WMS_VERSION_1_0_0){
      documentName->copy("WMS_1_0_0_GetCapabilities");
    }
    if(srvParam->OGCVersion==WMS_VERSION_1_1_1){
      documentName->copy("WMS_1_1_1_GetCapabilities");
    }
    if(srvParam->OGCVersion==WMS_VERSION_1_3_0){
      documentName->copy("WMS_1_3_0_GetCapabilities");
    }
  }
  if(srvParam->requestType==REQUEST_WMS_GETREFERENCETIMES){
    documentName->copy("WMS_1_3_0_GetReferenceTimes");
  }
  if(srvParam->requestType==REQUEST_WCS_GETCAPABILITIES){
    if(srvParam->OGCVersion==WCS_VERSION_1_0){
      documentName->copy("WCS_1_0_GetCapabilities");
    }
  }
  if(srvParam->requestType==REQUEST_WCS_DESCRIBECOVERAGE){
    if(srvParam->OGCVersion==WCS_VERSION_1_0){
      documentName->copy("WCS_1_0_DescribeCoverage");
    }
  }
  if(documentName->equals("none")){
    CDBError("Unknown cache request...");
    return 1;
  }
  return 0;
}

