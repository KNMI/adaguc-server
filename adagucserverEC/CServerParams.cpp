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

#include "CServerParams.h"
#include "CStopWatch.h"
const char *CServerParams::className="CServerParams";



CServerParams::CServerParams(){
  
  WMSLayers=NULL;
  serviceType=-1;
  requestType=-1;
  OGCVersion=-1;

  Transparent=false;
  enableDocumentCache=false;
  configObj = new CServerConfig();
  Geo = new CGeoParams;
  imageFormat=IMAGEFORMAT_IMAGEPNG8;
  imageMode=SERVERIMAGEMODE_8BIT;
  autoOpenDAPEnabled=-1;
  autoLocalFileResourceEnabled = -1;
  showDimensionsInImage = false;
  showLegendInImage = false;
  showScaleBarInImage = false;
  figWidth=-1;
  figHeight=-1;

}

CServerParams::~CServerParams(){
  if(WMSLayers!=NULL){delete[] WMSLayers;WMSLayers=NULL;}
  if(configObj!=NULL){delete configObj;configObj=NULL;}
  if(Geo!=NULL){delete Geo;Geo=NULL;}
  for(size_t j=0;j<requestDims.size();j++){
    delete requestDims[j];
    requestDims[j]= NULL;
  }
  requestDims.clear();
  
}


void CServerParams::getCacheFileName(CT::string *cacheFileName){
  CT::string cacheName("WMSCACHE");
  bool useProvidedCacheFileName=false;
  //Check wether a cachefile has been provided in the config
  if(cfg->CacheDocs.size()>0){
    if(cfg->CacheDocs[0]->attr.cachefile.empty()==false){
      useProvidedCacheFileName=true;
      cacheName.concat("_");
      cacheName.concat(cfg->CacheDocs[0]->attr.cachefile.c_str());
    }
  }
  if(useProvidedCacheFileName==false){
    //If no cache filename is provided, we will create a standard one
    
    if(autoResourceLocation.empty()==false){
       cacheName.concat(&configFileName);
       cacheName.concat(&autoResourceLocation);
    } else {
      //Based on the name of the configuration file
      cacheName.concat(&configFileName);
    }
    for(size_t j=0;j<cacheName.length();j++){
      char c=cacheName.charAt(j);
      if(c=='/')c='_';
      if(c=='\\')c='_';
      if(c=='.')c='_';
      cacheName.setChar(j,c);
    }
  }
  

  //Insert the tmp dir in the beginning
  cacheFileName->copy(cfg->TempDir[0]->attr.value.c_str());
  cacheFileName->concat("/");
  cacheFileName->concat(&cacheName);
//  CDBDebug("Make pub dir %s",cacheFileName->c_str());
  CDirReader::makePublicDirectory(cacheFileName->c_str());
  cacheFileName->concat("/simplecachestore.dat");
}


void CServerParams::_getCacheDirectory(CT::string *_cacheFileName){
  CT::string cacheFileName;
  //CDBDebug("getCacheDirectory");
  CT::string cacheName("WMSCACHE");
  bool useProvidedCacheFileName=false;
  //Check wether a cachefile has been provided in the config
  if(cfg->CacheDocs.size()>0){
    if(cfg->CacheDocs[0]->attr.cachefile.empty()==false){
      useProvidedCacheFileName=true;
      cacheName.concat("_");
      cacheName.concat(cfg->CacheDocs[0]->attr.cachefile.c_str());
    }
  }
  if(useProvidedCacheFileName==false){
    //If no cache filename is provided, we will create a standard one
    //Based on the name of the configuration file
    cacheName.concat(&configFileName);
    for(size_t j=0;j<cacheName.length();j++){
      char c=cacheName.charAt(j);
      if(c=='/')c='_';
      if(c=='\\')c='_';
      if(c=='.')c='_';
      cacheName.setChar(j,c);
    }
  }
  //Insert the tmp dir in the beginning
  cacheFileName.copy(cfg->TempDir[0]->attr.value.c_str());
  cacheFileName.concat("/");
  cacheFileName.concat(&cacheName);
  
  //Split up

   int maxlength = 200;
  size_t nrStrings = (cacheFileName.length()/maxlength)+1;
  CT::string myidinparts = "";
  for(size_t j=0;j<nrStrings;j++){
    myidinparts+="/";
    myidinparts+= cacheFileName.substring(j*maxlength,j*maxlength+maxlength);
  }
  _cacheFileName->copy(myidinparts.c_str());
}




#include <stdio.h>
#define USE_DEVRANDOM


#include <ctime>
#include <sys/time.h>


const CT::string CServerParams::randomString(const int len) {
  #ifdef MEASURETIME
  StopWatch_Stop(">CServerParams::randomString");
  #endif

    char s[len+1];
    
    timeval curTime;
    gettimeofday(&curTime, NULL);
    
#ifdef USE_DEVRANDOM
    FILE* randomData = fopen("/dev/urandom", "r");
    int myRandomInteger;
    fread(&myRandomInteger, sizeof myRandomInteger,1,randomData);
    srand(myRandomInteger);
    fclose(randomData);
#else
    srand((curTime.tv_sec * 1000) + (curTime.tv_usec / 1000));
#endif
    
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
    CT::string r=s;
    
    #ifdef MEASURETIME
    StopWatch_Stop("<CServerParams::randomString");
    #endif
    return r;
}







//Table names need to be different between dims like time and height.
// Therefore create unique tablenames like tablename_time and tablename_height
void CServerParams::makeCorrectTableName(CT::string *tableName,CT::string *dimName){
  tableName->concat("_");tableName->concat(dimName);
  tableName->replaceSelf("-","_m_");
  tableName->replaceSelf("+","_p_");
  tableName->replaceSelf(".","_");
  tableName->toLowerCaseSelf();
}

void CServerParams::showWCSNotEnabledErrorMessage(){
  CDBError("WCS is not enabled because GDAL was not compiled into the server. ");
}


int CServerParams::makeUniqueLayerName(CT::string *layerName,CServerConfig::XMLE_Layer *cfgLayer){
  /*
  if(cfgLayer->Variable.size()!=0){
  _layerName=cfgLayer->Variable[0]->value.c_str();
}*/
  
  layerName->copy("");
  if(cfgLayer->Group.size()==1){
    if(cfgLayer->Group[0]->attr.value.empty()==false){
      layerName->copy(cfgLayer->Group[0]->attr.value.c_str());
      layerName->concat("/");
    }
  }
  if(cfgLayer->Name.size()==0){
    CServerConfig::XMLE_Name *name=new CServerConfig::XMLE_Name();
    cfgLayer->Name.push_back(name);
    if(cfgLayer->Variable.size()==0){
      name->value.copy("undefined_variable");
    }else{
      name->value.copy(cfgLayer->Variable[0]->value.c_str());
    }
  }
  
  if(!cfgLayer->Name[0]->attr.force.equals("true")){
    layerName->concat(cfgLayer->Name[0]->value.c_str());
    //layerName->replaceSelf(".","_");//TODO CHECK WHY?
    layerName->replaceSelf(" ", "_");
  }else{
    layerName->copy(cfgLayer->Name[0]->value.c_str());
  }
  
  
  /*if(cfgLayer->Title.size()==0){
    CServerConfig::XMLE_Title *title=new CServerConfig::XMLE_Title();
    cfgLayer->Title.push_back(title);
    title->value.copy(cfgLayer->Name[0]->value.c_str());
  } */ 
  
  
  

  return 0;
}



int CServerParams::makeLayerGroupName(CT::string *groupName,CServerConfig::XMLE_Layer *cfgLayer){
  /*
  if(cfgLayer->Variable.size()!=0){
  _layerName=cfgLayer->Variable[0]->value.c_str();
}*/
  
  CT::string layerName;
  groupName->copy("");
  if(cfgLayer->Group.size()==1){
    if(cfgLayer->Group[0]->attr.value.c_str()!=NULL){
      CT::string layerName(cfgLayer->Group[0]->attr.value.c_str());
      CT::string *groupElements=layerName.splitToArray("/");
      groupName->copy(groupElements[0].c_str());
      delete[] groupElements;
    }
  }

  return 0;
}

bool CServerParams::isAutoResourceCacheEnabled() const {

  if (cfg->AutoResource.size() > 0)
    return cfg->AutoResource[0]->attr.enablecache.equals("true");
  return false;
}


bool CServerParams::isAutoOpenDAPResourceEnabled(){
  if (this->datasetLocation.empty() == false){
    return false;
  }
  if(autoOpenDAPEnabled==-1){
    autoOpenDAPEnabled = 0;
    if(cfg->AutoResource.size()>0){
      if(cfg->AutoResource[0]->attr.enableautoopendap.equals("true"))autoOpenDAPEnabled = 1;
    }
  }
  if(autoOpenDAPEnabled==1)return true;
  return false;
}

bool CServerParams::isAutoLocalFileResourceEnabled(){
  /* When a dataset is configured, autoscan should be disabled */
  if (this->datasetLocation.empty() == false){
    return false;
  }
  if(autoLocalFileResourceEnabled==-1){
    autoLocalFileResourceEnabled = 0;
    if(cfg->AutoResource.size()>0){
      if(cfg->AutoResource[0]->attr.enablelocalfile.equals("true"))autoLocalFileResourceEnabled = 1;
    }
  }
  if(autoLocalFileResourceEnabled==1)return true;
  return false;
}

bool CServerParams::isAutoResourceEnabled(){
  if(isAutoOpenDAPResourceEnabled()||isAutoLocalFileResourceEnabled())return true;
  return false;
}


bool CServerParams::checkIfPathHasValidTokens(const char *path){
  return checkForValidTokens(path,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/_-+:. ,[]");
}

bool CServerParams::checkForValidTokens(const char *path,const char *validPATHTokens){
  //Check for valid tokens
  size_t pathLength=strlen(path);
  size_t allowedTokenLength=strlen(validPATHTokens);
  for(size_t j=0;j<pathLength;j++){
    bool isInvalid = true;
    for(size_t i=0;i<allowedTokenLength;i++){
      if(path[j]==validPATHTokens[i]){isInvalid=false;break;}
    }
    if(isInvalid){
      CDBDebug("Invalid token '%c' in '%s'",path[j],path);
      return false;
    }
    
    //Check for sequences
    if(j>0){
      if(path[j-1]=='.'&&path[j]=='.'){
        CDBDebug("Invalid sequence in '%s'",path);
        return false;
      }
    }
  }
  return true;
}


bool CServerParams::checkResolvePath(const char *path,CT::string *resolvedPath){
  if(cfg->AutoResource.size()>0){
    //Needs to be configured otherwise it will be denied.
    if(cfg->AutoResource[0]->Dir.size()==0){
      CDBDebug("No Dir elements defined");
      return false;
    }
    
    if(checkIfPathHasValidTokens(path)==false)return false;
    
    for(size_t d=0;d<cfg->AutoResource[0]->Dir.size();d++){
      const char *_baseDir =cfg->AutoResource[0]->Dir[d]->attr.basedir.c_str();
      const char *dirPrefix=cfg->AutoResource[0]->Dir[d]->attr.prefix.c_str();

      char baseDir[PATH_MAX];
      if(realpath(_baseDir,baseDir)==NULL){
        CDBError("Skipping AutoResource[0]->Dir[%d]->basedir: Configured value is not a valid realpath", d);
        continue;
      }
      
      if(baseDir!=NULL&&dirPrefix!=NULL){
        //Prepend the prefix to make the absolute path
        CT::string pathToCheck;
        pathToCheck.print("%s/%s",dirPrefix,path);
        //Make a realpath
        char szResolvedPath[PATH_MAX];
        if(realpath(pathToCheck.c_str(),szResolvedPath)==NULL){
          //CDBDebug("basedir='%s', prefix='%s', inputpath='%s', absolutepath='%s'",baseDir,dirPrefix,path,pathToCheck.c_str());
          CDBDebug("LOCALFILEACCESS: Invalid path '%s'",pathToCheck.c_str());
        }else{
          //Check if the resolved path is within the basedir
          //CDBDebug("basedir='%s', prefix='%s', inputpath='%s', absolutepath='%s'",baseDir,dirPrefix,path,pathToCheck.c_str());
          CDBDebug("szResolvedPath: %s", szResolvedPath);
          CDBDebug("baseDir       : %s", baseDir);
          CT::string resolvedPathStr=szResolvedPath;
          if(resolvedPathStr.indexOf(baseDir)==0){
            resolvedPath->copy(szResolvedPath);
            return true;
          }
        }
      }else{
        if(baseDir==NULL){CDBDebug("basedir not defined");}
        if(dirPrefix==NULL){CDBDebug("prefix not defined");}
      }
    }
  }else{
    CDBDebug("No AutoResource enabled");
  }
  return false;
}

void CServerParams::encodeTableName(CT::string *tableName){
  tableName->replaceSelf("/","_");
  tableName->toLowerCaseSelf();
}

void CServerParams::setOnlineResource(CT::string r){
  _onlineResource = r;
};

CT::string CServerParams::getOnlineResource(){
  if(_onlineResource.length()>0){
    return _onlineResource;
  }
  if(cfg->OnlineResource.size() == 0){
    //No Online resource is given.
     const char *pszADAGUCOnlineResource=getenv("ADAGUC_ONLINERESOURCE");
    if(pszADAGUCOnlineResource==NULL){
      CDBDebug("Warning: No OnlineResources configured. Unable to get from config OnlineResource or from environment ADAGUC_ONLINERESOURCE");
      _onlineResource = "";
      return "";
    }
    CT::string onlineResource=pszADAGUCOnlineResource;
    _onlineResource = onlineResource;
    return onlineResource;
  }
  
  CT::string onlineResource=cfg->OnlineResource[0]->attr.value.c_str();
  
  
  //A full path is given in the configuration
  if(onlineResource.indexOf("http",4)==0){
    _onlineResource = onlineResource;
    return onlineResource;
  }
  
  //Only the last part is given, we need to prepend the HTTP_HOST environment variable.
  const char *pszHTTPHost=getenv("HTTP_HOST");
  if(pszHTTPHost==NULL){
    CDBError("Unable to determine HTTP_HOST");
    _onlineResource = "";
    return "";
  }
  CT::string httpHost="http://";
  httpHost.concat(pszHTTPHost);
  httpHost.concat(&onlineResource);
  _onlineResource = httpHost;
  return httpHost;
}


bool CServerParams::checkBBOXXYOrder(const char *projName){
  if(OGCVersion == WMS_VERSION_1_3_0){
    CT::string projNameString;
    if(projName == NULL){
      projNameString = Geo->CRS.c_str();
    }else{
      projNameString = projName;
    }
    if(projNameString.equals("EPSG:4326"))return true;
    else if(projNameString.equals("EPSG:4258"))return true;
    else if(projNameString.equals("CRS:84"))return true;
    
  }
  return false;
}








/**
* Returns a stringlist with all possible legends available for this Legend config object.
* This is usually a configured legend element in a layer, or a configured legend element in a style.
* @param Legend a XMLE_Legend object configured in a style or in a layer
* @return Pointer to a new stringlist with all possible legend names, must be deleted with delete. Is NULL on failure.
*/
CT::PointerList<CT::string*> *CServerParams::getLegendNames(std::vector <CServerConfig::XMLE_Legend*> Legend){
  if(Legend.size()==0){CDBError("No legends defined");return NULL;}
  CT::PointerList<CT::string*> *stringList = new CT::PointerList<CT::string*>();
  
  for(size_t j=0;j<Legend.size();j++){
    CT::string legendValue=Legend[j]->value.c_str();
    CT::StackList<CT::string> l1=legendValue.splitToStack(",");
    for(size_t i=0;i<l1.size();i++){
      if(l1[i].length()>0){
        CT::string * val = new CT::string();
        stringList->push_back(val);
        val->copy(&l1[i]);
      }
    }
  }
  return stringList;
}

int CServerParams::dataRestriction = -1;
int CServerParams::checkDataRestriction(){
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

const char *timeFormatAllowedChars="0123456789:TZ-/. _ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz()*";
bool CServerParams::checkTimeFormat(CT::string& timeToCheck){
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

int CServerParams::parseConfigFile(CT::string &pszConfigFile){
  CT::string configFileData;
  
  configFileData = "";
  try{
    try {
      configFileData = CReadFile::open(pszConfigFile.c_str());
    } catch(int e){
      CDBError("Unable to open configuration file [%s], error %d", pszConfigFile.c_str(), e);
      return 1;
    }
    const char *pszADAGUC_PATH=getenv("ADAGUC_PATH");
    if(pszADAGUC_PATH!=NULL){
      CT::string adagucPath = CDirReader::makeCleanPath(pszADAGUC_PATH);
      adagucPath = adagucPath + "/";
      configFileData.replaceSelf("{ADAGUC_PATH}",adagucPath.c_str());
    }
    const char *pszADAGUC_TMP=getenv("ADAGUC_TMP");
    if(pszADAGUC_TMP!=NULL)configFileData.replaceSelf("{ADAGUC_TMP}",pszADAGUC_TMP);
    const char *pszADAGUC_DB=getenv("ADAGUC_DB");
    if(pszADAGUC_DB!=NULL)configFileData.replaceSelf("{ADAGUC_DB}",pszADAGUC_DB);
  }catch(int e){
    CDBError("Exception %d in substituting", e);
  }

  int status = configObj->parse(configFileData.c_str(),configFileData.length());
  
  if(status == 0 && configObj->Configuration.size()==1){
    return 0;
  }else{
    //cfg=NULL;
    CDBError("Invalid XML file %s",pszConfigFile.c_str());
    return 1;
  } 
}

