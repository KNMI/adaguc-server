#include "CRequest.h"
const char *CRequest::className="CRequest";
int CRequest::CGI=0;

int CRequest::runRequest(){
  int status=process_querystring();
  CDFObjectStore::getCDFObjectStore()->clear();
  return status;
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
    temp.toUpperCase();
    if(temp.equals("TRUE")){
      dr = ALLOW_NONE;
    }
    if(temp.equals("FALSE")){
      dr = ALLOW_WCS|ALLOW_GFI|ALLOW_METADATA;
    }
    //Decompose into stringlist and check each item
    CT::stringlist *items = temp.splitN("|");
    for(size_t j=0;j<items->size();j++){
      if(items->get(j)->equals("ALLOW_GFI"))dr|=ALLOW_GFI;
      if(items->get(j)->equals("ALLOW_WCS"))dr|=ALLOW_WCS;
      if(items->get(j)->equals("ALLOW_METADATA"))dr|=ALLOW_METADATA;
      if(items->get(j)->equals("SHOW_QUERYINFO"))dr|=SHOW_QUERYINFO;
    }
    delete items;
  }
  
  dataRestriction = dr;
  return dataRestriction;
}


int CRequest::setConfigFile(const char *pszConfigFile){
  int status = srvParam->configObj->parseFile(pszConfigFile);
  if(status == 0){
    srvParam->configFileName.copy(pszConfigFile);
    srvParam->cfg=srvParam->configObj->Configuration[0];
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
      CDBDebug("Unable to get document %s from cache",docName->c_str());
      cacheNeedsRefresh=true;
    }
  }
  if(cacheNeedsRefresh==true){
    simpleStore->setStringAttribute("configModificationDate",configModificationDate.c_str());
    CDBDebug("cacheNeedsRefresh==true");
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
      CDBDebug("Generating a new document with name %s",documentName.c_str());
      int status = generateOGCGetCapabilities(&XMLdocument);if(status!=0)return 1;
      //Store this document  
      simpleStore.setStringAttribute(documentName.c_str(),XMLdocument.c_str());
      if(storeDocumentCache(&simpleStore)!=0)return 1;
    }else{
      CDBDebug("Providing document from store with name %s",documentName.c_str());
    }
  }else{
    //Do not use cache
    int status = generateOGCGetCapabilities(&XMLdocument);if(status!=0)return 1;
  }
  printf("%s%c%c\n","Content-Type:text/xml",13,10);
  printf("%s",XMLdocument.c_str());
  

  
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
  if(srvParam->WMSLayers!=NULL)
    for(size_t j=0;j<srvParam->WMSLayers->count;j++){
      CDBDebug("WMS GETMAP for layers (%d) %s",j,srvParam->WMSLayers[j].c_str());
    }
  return process_all_layers();
}



const char *timeFormatAllowedChars="0123456789:TZ-/. _ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
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
    if(srvParam->Geo->dWidth>8000){
      CDBError("Parameter WIDTH must be smaller than 8000");
      return 1;
    }
    if(srvParam->Geo->dHeight>8000){
    CDBError("Parameter HEIGHT must be smaller than 8000");
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
      status = generateOGCDescribeCoverage(&XMLDocument);if(status!=0)return 1;
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
       
        CDataReader::autoConfigureDimensions(dataSources[j],true);
        
      }
      if(dataSources[j]->cfgLayer->Dimension.size()!=0){
        CPGSQLDB DB;
        CT::string Query;
        CT::string *values_path=NULL;
        CT::string *values_dim=NULL;
        CT::string *date_time=NULL;
        //char szPartialQuery[MAX_STR_LEN+1];
      
        // Connect do DB
        status = DB.connect(srvParam->cfg->DataBase[0]->attr.parameters.c_str());if(status!=0){CDBError("Unable to connect to DB");return 1;}
        //Get the number of required dims from the given dims
        //Check if all dimensions are given
        for(int k=0;k<srvParam->NumOGCDims;k++)srvParam->OGCDims[k].Name.toLowerCase();
        int dimsfound[NC_MAX_DIMS];
        for(size_t i=0;i<dataSources[j]->cfgLayer->Dimension.size();i++){
          CT::string dimName(dataSources[j]->cfgLayer->Dimension[i]->value.c_str());
          dimName.toLowerCase();
          dimsfound[i]=0;
          for(int k=0;k<srvParam->NumOGCDims;k++){
            //CDBDebug("DIM COMPARE: %s==%s",srvParam->OGCDims[k].Name.c_str(),dimName.c_str());
            if(srvParam->OGCDims[k].Name.equals(&dimName)){
              //This dimension has been specified in the request, so the dimension has been found:
              dimsfound[i]=1;
              COGCDims *ogcDim = new COGCDims;
              dataSources[j]->requiredDims.push_back(ogcDim);
              ogcDim->Name.copy(&dimName);
              ogcDim->Value.copy(&srvParam->OGCDims[k].Value);
              ogcDim->netCDFDimName.copy(dataSources[j]->cfgLayer->Dimension[i]->attr.name.c_str());
              //If we have a special styled layer, the time does not come from TIME, but from style:
              if(dataSources[0]->dLayerType==CConfigReaderLayerTypeStyled){
                CT::string *splittedStyles=srvParam->Styles.split(",");
                if(splittedStyles->count!=dataSources.size()){
                  status = DB.close();
                  CDBError("Number of provided layers (%d) does not match the number of provided styles (%d)",dataSources.size(),splittedStyles->count);
                  //for(size_t j=0;j<requiredDims.size();j++)delete requiredDims[j];
                  return 1;
                }
                if(j==0){
                  //The first boolean layer needs to be loaded with an unspecified time 
                  //We will use 'current' to make sure something is loaded
                  ogcDim->Value.copy("current");
                }
                
                if(j>0){
                  //Try to find the time from the style:
                  CT::string * stripeString = splittedStyles[j].split("|");
                  if(stripeString->count==2){
                    if(stripeString[1].indexOf("time_")==0){
                      CT::string *valuePair=stripeString[1].split("_");
                      ogcDim->Value.copy(&valuePair[1]);
                      delete[] valuePair;
                    }
                  }
                  delete[] stripeString ;
                }
                delete[] splittedStyles;
                //CDBDebug("srvParam->OGCDims[k].Value %d==%s",j,srvParam->OGCDims[k].Value.c_str());
              }
              // If we have value 'current', give the dim a special status
              if(ogcDim->Value.equals("current")){
                dimsfound[i]=2;
                CT::string tableName(dataSources[j]->cfgLayer->DataBaseTable[0]->value.c_str());
                CServerParams::makeCorrectTableName(&tableName,&ogcDim->netCDFDimName);
                
                Query.print("select max(%s) from %s",
                        ogcDim->netCDFDimName.c_str(),
                            tableName.c_str());
                CT::string *temp = DB.query_select(Query.c_str(),0);
                if(temp == NULL){CDBError("Query failed");status = DB.close(); return 1;}
                ogcDim->Value.copy(&temp[0]);
              }
            }
          }
        }
      

        // Fill in the undefined dims
        for(size_t i=0;i<dataSources[j]->cfgLayer->Dimension.size();i++){
          if(dimsfound[i]==0){
            CT::string netCDFDimName(dataSources[j]->cfgLayer->Dimension[i]->attr.name.c_str());
            CT::string tableName(dataSources[j]->cfgLayer->DataBaseTable[0]->value.c_str());
            CServerParams::makeCorrectTableName(&tableName,&netCDFDimName);
            //Add the undefined dims to the srvParams as additional dims
            COGCDims *ogcDim = new COGCDims;
            dataSources[j]->requiredDims.push_back(ogcDim);
            ogcDim->Name.copy(dataSources[j]->cfgLayer->Dimension[i]->value.c_str());
            ogcDim->netCDFDimName.copy(dataSources[j]->cfgLayer->Dimension[i]->attr.name.c_str());
            //Try to find the max value for this dim name from the database
            Query.print("select max(%s) from %s",
                      dataSources[j]->cfgLayer->Dimension[i]->attr.name.c_str(),
                      tableName.c_str());
            //Execute the query
            CT::string *temp = DB.query_select(Query.c_str(),0);
            if(temp == NULL){CDBError("Query failed");status = DB.close(); return 1;}
            //Copy the value corresponding to this dim name to srvparams
            ogcDim->Value.copy(&temp[0]);
            delete[] temp;
          }
        }
        
        
      

        Query.print("select a0.path");
        for(size_t i=0;i<dataSources[j]->requiredDims.size();i++){
          Query.printconcat(",%s,dim%s",dataSources[j]->requiredDims[i]->netCDFDimName.c_str(),dataSources[j]->requiredDims[i]->netCDFDimName.c_str());
        }
        
        Query.concat(" from ");
        bool timeValidationError = false;
        for(size_t i=0;i<dataSources[j]->requiredDims.size();i++){
          CT::string netCDFDimName(&dataSources[j]->requiredDims[i]->netCDFDimName);
          CT::string tableName(dataSources[j]->cfgLayer->DataBaseTable[0]->value.c_str());
          CServerParams::makeCorrectTableName(&tableName,&netCDFDimName);
          CT::string subQuery;
          subQuery.print("(select path,dim%s,%s from %s where ",netCDFDimName.c_str(),
                      netCDFDimName.c_str(),
                      tableName.c_str());
          CT::string queryParams(&dataSources[j]->requiredDims[i]->Value);
          CT::string *cDims =queryParams.split(",");// Split up by commas (and put into cDims)
          for(size_t k=0;k<cDims->count;k++){
            CT::string *sDims =cDims[k].split("/");// Split up by slashes (and put into sDims)
            if(sDims->count>0&&k>0)subQuery.concat("or ");
            for(size_t  l=0;l<sDims->count&&l<2;l++){
              if(sDims[l].length()>0){
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
          subQuery.printconcat("ORDER BY %s)a%d ",netCDFDimName.c_str(),i);
          if(i<dataSources[j]->requiredDims.size()-1)subQuery.concat(",");
          Query.concat(&subQuery);
        }
        //Join by path
        if(dataSources[j]->requiredDims.size()>1){
          Query.concat(" where a0.path=a1.path");
          for(size_t i=2;i<dataSources[j]->requiredDims.size();i++){
            Query.printconcat(" and a0.path=a%d.path",i);
          }
        }
        Query.concat(" limit 32");
        //CDBDebug(Query.c_str());
        if(timeValidationError==true){
          if((checkDataRestriction()&SHOW_QUERYINFO)==false)Query.copy("hidden");
          CDBError("Query fails regular expression: '%s'",Query.c_str());
          status = DB.close();
          return 1;
        }
        

        //Execute the query
        values_path = DB.query_select(Query.c_str(),0);
        if(values_path==NULL&&srvParam->isAutoOpenDAPEnabled()){
          //TODO disable automatic update for certain cases!
          CDBDebug("No results for query: Trying to update the database automatically.");
        
          status = CDBFileScanner::updatedb(srvParam->cfg->DataBase[0]->attr.parameters.c_str(),dataSources[j],NULL,NULL);
          if(status !=0){CDBError("Could not update db for: %s",dataSources[j]->cfgLayer->Name[0]->value.c_str());DB.close();return 2;}
          values_path = DB.query_select(Query.c_str(),0);
          if(values_path==NULL){
            if((checkDataRestriction()&SHOW_QUERYINFO)==false)Query.copy("hidden");
            CDBError("No results for query: '%s'",Query.c_str());
            return 2;
          }
        }
        if(values_path==NULL){
          if((checkDataRestriction()&SHOW_QUERYINFO)==false)Query.copy("hidden");
          CDBError("No results for query: '%s'",Query.c_str());
          return 2;
        }
        if(values_path->count==0){
          if((checkDataRestriction()&SHOW_QUERYINFO)==false)Query.copy("hidden");
          CDBError("No results for query: '%s'",Query.c_str());
          delete[] values_path;
          status = DB.close();
          return 2;
        }
        //Get timestring
        date_time = DB.query_select(Query.c_str(),1);
        if(date_time == NULL){CDBError("Query failed");status = DB.close(); return 1;}
        
        
        //Now get the dimensions
        std::vector <CT::string*> dimValues;
        for(size_t i=0;i<dataSources[j]->requiredDims.size();i++){
          CT::string *t=DB.query_select(Query.c_str(),2+i*2);
          dimValues.push_back(t);
        }
              
        for(size_t k=0;k<values_path->count;k++){
          CDataSource::TimeStep * timeStep = new CDataSource::TimeStep();
          dataSources[j]->timeSteps.push_back(timeStep);
          timeStep->fileName.copy(values_path[k].c_str());
          //CDBDebug("%s",timeStep->fileName.c_str());
          timeStep->timeString.copy(date_time[k].c_str());
          //For each timesteps a new set of dimensions is added with corresponding dim array indices.
          for(size_t i=0;i<dataSources[j]->requiredDims.size();i++){
            timeStep->dims.addDimension(dataSources[j]->requiredDims[i]->netCDFDimName.c_str(),atoi(dimValues[i][k].c_str()));
          }
        }

        status = DB.close();  if(status!=0)return 1;
      
        for(size_t j=0;j<dimValues.size();j++){
          delete[] dimValues[j]; 
        }
        delete[] values_path;
        delete[] values_dim;
        delete[] date_time;
      }else{
        
        //This layer has no dimensions
        //We need to add one timestep with data.
        CDataSource::TimeStep * timeStep = new CDataSource::TimeStep();
        dataSources[j]->timeSteps.push_back(timeStep);
         //if(dataSources[j]->cfgLayer->Dimension.size()!=0){
        timeStep->fileName.copy(dataSources[j]->cfgLayer->FilePath[0]->value.c_str());
        
        CDirReader dirReader;
        CDBFileScanner::searchFileNames(&dirReader,dataSources[j]->cfgLayer->FilePath[0]->value.c_str(),dataSources[j]->cfgLayer->FilePath[0]->attr.filter.c_str(),NULL);
        if(dirReader.fileList.size()==1){
          timeStep->fileName.copy(dirReader.fileList[0]->fullName.c_str());
        }else{
          timeStep->fileName.copy(dataSources[j]->cfgLayer->FilePath[0]->value.c_str());
        }
        
        timeStep->timeString.copy("0");
        timeStep->dims.addDimension("time",0);
        
      }
    }
    if(dataSources[j]->dLayerType==CConfigReaderLayerTypeCascaded){
       
        //This layer has no dimensions
        //We need to add one timestep with data.
        CDataSource::TimeStep * timeStep = new CDataSource::TimeStep();
        dataSources[j]->timeSteps.push_back(timeStep);
         //if(dataSources[j]->cfgLayer->Dimension.size()!=0){
        timeStep->fileName.copy("");
        timeStep->timeString.copy("0");
        timeStep->dims.addDimension("time",0);
        
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
        if(srvParam->requestType==REQUEST_WMS_GETMAP){
          CImageDataWriter imageDataWriter;

          /*
            We want like give priority to our own internal layers, instead to external cascaded layers. This is because
            our internal layers have an exact customized legend, and we would like to use this always.
          */
          bool imageDataWriterIsInitialized = false;
          for(size_t d=0;d<dataSources.size()&&imageDataWriterIsInitialized==false;d++){
            if(dataSources[d]->dLayerType!=CConfigReaderLayerTypeCascaded){
              status = imageDataWriter.init(srvParam,dataSources[d],dataSources[d]->getNumTimeSteps());if(status != 0)throw(__LINE__);
              imageDataWriterIsInitialized=true;
            }
          }
          //There are only cascaded layers, so we intiialize the imagedatawriter with this the first layer.
          if(imageDataWriterIsInitialized==false){
            status = imageDataWriter.init(srvParam,dataSources[0],dataSources[0]->getNumTimeSteps());if(status != 0)throw(__LINE__);
            imageDataWriterIsInitialized=true;
          }
          
          //When we have multiple timesteps, we will create an animation.
          if(dataSources[0]->getNumTimeSteps()>1)imageDataWriter.createAnimation();
    
          for(size_t k=0;k<(size_t)dataSources[0]->getNumTimeSteps();k++){
            for(size_t d=0;d<dataSources.size();d++){
              dataSources[d]->setTimeStep(k);
            }
            if(dataSources[j]->dLayerType==CConfigReaderLayerTypeDataBase||
              dataSources[j]->dLayerType==CConfigReaderLayerTypeCascaded){
              //CDBDebug("!");
              status = imageDataWriter.addData(dataSources);
              //CDBDebug("!");
              if(status != 0){
                //Adding data failed:
                //Do not ruin an animation if one timestep fails to load.
                //If there is a single time step then throw an exception otherwise continue.
                if(dataSources[0]->getNumTimeSteps()==1){
                  //Not an animation, so throw an exception
                  CDBError("Unable to convert datasource %s to image",dataSources[j]->layerName.c_str());
                  throw(__LINE__);
                }else{
                  //This is an animation, report an error and continue with adding images.
                  CDBError("Unable to load datasource %s at line %d",dataSources[0]->dataObject[0]->variableName.c_str(),__LINE__);
                }
              }
            }
            if(dataSources[j]->dLayerType==CConfigReaderLayerTypeStyled){
              //Special styled layer for GEOMON project
              status = imageDataWriter.calculateData(dataSources);if(status != 0)throw(__LINE__);
            }
            if(dataSources[j]->getNumTimeSteps()>1){
              //Print the animation data into the image
              char szTemp[1024];
              snprintf(szTemp,1023,"%s UTC",dataSources[0]->timeSteps[k]->timeString.c_str());
              imageDataWriter.setDate(szTemp);
            }
          }
          
          //CDBDebug("drawing titles");
          
          int textY=5;
          //int prevTextY=0;
          if(srvParam->mapTitle.length()>0){
            if(srvParam->cfg->WMS[0]->TitleFont.size()==1){
              float fontSize=parseFloat(srvParam->cfg->WMS[0]->TitleFont[0]->attr.size.c_str());
              textY+=fontSize;
              //imageDataWriter.drawImage.rectangle(0,0,srvParam->Geo->dWidth,textY+8,CColor(255,255,255,0),CColor(255,255,255,80));
              //imageDataWriter.drawImage.setFillC
              imageDataWriter.drawImage.drawText(5,textY,srvParam->cfg->WMS[0]->TitleFont[0]->attr.location.c_str(),fontSize,0,srvParam->mapTitle.c_str(),CColor(0,0,0,255),CColor(180,180,200,100));
              textY+=12;
              
              //prevTextY=textY;
            }
          }
          if(srvParam->mapSubTitle.length()>0){
            if(srvParam->cfg->WMS[0]->SubTitleFont.size()==1){
              float fontSize=parseFloat(srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.size.c_str());
              textY+=fontSize;
              
              //imageDataWriter.drawImage.rectangle(0,prevTextY,srvParam->Geo->dWidth,textY+4,CColor(255,255,255,0),CColor(255,255,255,80));
              imageDataWriter.drawImage.drawText(6,textY,srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.location.c_str(),fontSize,0,srvParam->mapSubTitle.c_str(),CColor(0,0,0,255),CColor(180,180,200,100));
              textY+=8;
              //prevTextY=textY;
            }
          }
          //imageDataWriter.drawText(5,5+30+25+2+12+2,"/home/visadm/software/fonts/verdana.ttf",12,0,dataSources[0]->timeSteps[0]->timeString.c_str(),240);
          if(srvParam->showDimensionsInImage){
            textY+=4;
            for(int d=0;d<srvParam->NumOGCDims;d++){
              CT::string message;
              float fontSize=parseFloat(srvParam->cfg->WMS[0]->DimensionFont[0]->attr.size.c_str());
              textY+=fontSize*1.2;
              message.print("%s: %s",srvParam->OGCDims[d].Name.c_str(),srvParam->OGCDims[d].Value.c_str());
              imageDataWriter.drawText(6,textY,srvParam->cfg->WMS[0]->DimensionFont[0]->attr.location.c_str(),fontSize,0,message.c_str(),240);
              textY+=4;
            }
          }
          
          if(srvParam->showLegendInImage){
            //Draw legend
            bool legendDrawn = false;
            for(size_t d=0;d<dataSources.size()&&legendDrawn==false;d++){
              if(dataSources[d]->dLayerType!=CConfigReaderLayerTypeCascaded){
                CDrawImage legendImage;
                legendImage.createImage(&imageDataWriter.drawImage,LEGEND_WIDTH,LEGEND_HEIGHT);
                
                status = imageDataWriter.createLegend(dataSources[d],&legendImage);if(status != 0)throw(__LINE__);
                int posX=5;
                int posY=imageDataWriter.drawImage.Geo->dHeight-(legendImage.Geo->dHeight+5);
                imageDataWriter.drawImage.rectangle(posX,posY,legendImage.Geo->dWidth+posX,legendImage.Geo->dHeight+posY,CColor(255,255,255,0),CColor(255,255,255,200));
                imageDataWriter.drawImage.draw(posX,posY,0,0,&legendImage);
                legendDrawn=true;
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
          CImageDataWriter ImageDataWriter;
          status = ImageDataWriter.init(srvParam,dataSources[j],dataSources[j]->getNumTimeSteps());if(status != 0)throw(__LINE__);
          for(int k=0;k<dataSources[j]->getNumTimeSteps();k++){
            dataSources[j]->setTimeStep(k);
            status = ImageDataWriter.getFeatureInfo(dataSources[j],
                int(srvParam->dX),
                int(srvParam->dY));
            if(status != 0)throw(__LINE__);
          }
          status = ImageDataWriter.end();if(status != 0)throw(__LINE__);
        }
        
        // WMS Getlegendgraphic
        if(srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
          CImageDataWriter ImageDataWriter;
          status = ImageDataWriter.init(srvParam,dataSources[j],1);if(status != 0)throw(__LINE__);
          status = ImageDataWriter.createLegend(dataSources[j],&ImageDataWriter.drawImage);if(status != 0)throw(__LINE__);
          status = ImageDataWriter.end();if(status != 0)throw(__LINE__);
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
          
          CT::string dumpString;
          
          reader.dump(&dumpString);
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
//  StopWatch_Time("render()");
  //First try to find all possible dimensions
  //std::vector
 /* for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
    for(size_t d=0;d<srvParam->cfg->Layer[j]->Dimension.size();d++){
      CT::string *dim = new CT::string(srvParam->cfg->Layer[j]->Dimension[d]->value.c_str());
      
      dim->toUpperCase();
      
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
  int dFound_RESX=0;
  int dFound_RESY=0;
  int dFound_BBOX=0;
  int dFound_SRS=0;
  int dFound_CRS=0;
  int dFound_Debug=0;
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
  
  int dFound_OpenDAPSource=0;
  int dFound_OpenDAPVariable=0;
  
  char * data;
  data=getenv("QUERY_STRING");
  
  if(data==NULL){
    data=strdup("SERVICE=WMS&request=getcapabilities");
    //data=strdup("SERVICE=WMS&request=getmap&width=10&height=10&styles=&BBOX=0,50,10,60&srs=EPSG:4326&format=image/png&layers=temperature");
    CGI=0;
  }else CGI=1;
//  CDBDebug("QUERY_STRING: %s",data);
  //  StopWatch_Time("getenv(\"QUERY_STRING\")");
  CT::string queryString(data);
  queryString.decodeURL();

  CT::string * parameters=queryString.split("&");
  CT::string value0Cap;
  for(size_t j=0;j<parameters->count;j++){
    CT::string * values=parameters[j].split("=");

    // Styles parameter
    value0Cap.copy(&values[0]);
    value0Cap.toUpperCase();
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
        CT::string * bboxvalues=values[1].split(",");
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

      // X Parameter
      if(strncmp(value0Cap.c_str(),"X",1)==0&&value0Cap.length()==1){
        srvParam->dX=atof(values[1].c_str());
        dFound_X=1;
      }
      // Y Parameter
      if(strncmp(value0Cap.c_str(),"Y",1)==0&&value0Cap.length()==1){
        srvParam->dY=atof(values[1].c_str());
        dFound_Y=1;
      }
      // SRS / CRS Parameters
      if(value0Cap.equals("SRS")){
        if(parameters[j].length()>5){
          srvParam->Geo->CRS.copy(parameters[j].c_str()+4);
          dFound_SRS=1;
        }
      }
      if(value0Cap.equals("CRS")){
        if(parameters[j].length()>5){
          srvParam->Geo->CRS.copy(parameters[j].c_str()+4);
          srvParam->Geo->CRS.decodeURL();
          dFound_CRS=1;
        }
      }
      //DIM Params
      /*for(size_t d=0;d<queryDims.size();d++){
        CT::string dimName("DIM_");
        dimName.concat(queryDims[d]);
        if(value0Cap.equals(queryDims[d])||value0Cap.equals(&dimName)){
          
          srvParam->OGCDims[srvParam->NumOGCDims].Name.copy(queryDims[d]->c_str());
          srvParam->OGCDims[srvParam->NumOGCDims].Value.copy(&values[1]);
          srvParam->NumOGCDims++;
        }
      }*/
      if(value0Cap.equals("TIME")||value0Cap.equals("ELEVATION")||value0Cap.indexOf("DIM_")==0){
        
        if(value0Cap.indexOf("DIM_")==0){
          srvParam->OGCDims[srvParam->NumOGCDims].Name.copy(value0Cap.c_str()+4);
        }else{
          srvParam->OGCDims[srvParam->NumOGCDims].Name.copy(value0Cap.c_str());
        }
         srvParam->OGCDims[srvParam->NumOGCDims].Value.copy(&values[1]);
        //CDBDebug("DIM_ %s==%s",value0Cap.c_str(),srvParam->OGCDims[srvParam->NumOGCDims].Value.c_str());
        srvParam->NumOGCDims++;
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
            values[1].toUpperCase();
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
      if(dFound_OpenDAPSource==0){
        if(value0Cap.equals("SOURCE")){
          if(srvParam->OpenDAPSource.c_str()==NULL){
            srvParam->OpenDAPSource.copy(values[1].c_str());
          }
          dFound_OpenDAPSource=1;
        }
      }
      //Opendap variable parameter
       if(dFound_OpenDAPVariable==0){
        if(value0Cap.equals("VARIABLE")){
          if(srvParam->OpenDapVariable.c_str()==NULL){
            srvParam->OpenDapVariable.copy(values[1].c_str());
          }
          dFound_OpenDAPVariable=1;
        }
      }
      
      
      //WMS Layers parameter
      if(value0Cap.equals("LAYERS")){
        if(srvParam->WMSLayers!=NULL)
          delete[] srvParam->WMSLayers;
          srvParam->WMSLayers = values[1].split(",");
        dFound_WMSLAYERS=1;
      }
      //WMS Layer parameter
      if(value0Cap.equals("LAYER")){
        if(srvParam->WMSLayers!=NULL)
          delete[] srvParam->WMSLayers;
          srvParam->WMSLayers = values[1].split(",");
        dFound_WMSLAYER=1;
      }

    //WMS Layer parameter
      if(value0Cap.equals("QUERY_LAYERS")){
        if(srvParam->WMSLayers!=NULL)
          delete[] srvParam->WMSLayers;
          srvParam->WMSLayers = values[1].split(",");
          if(srvParam->WMSLayers->count>1){
            CDBError("Too many layers in request");
            dErrorOccured=1;
          }
        dFound_WMSLAYER=1;
    }
      //WCS Coverage parameter
    if(value0Cap.equals("COVERAGE")){
        if(srvParam->WMSLayers!=NULL){
          CDBWarning("COVERAGE already defined");
          dErrorOccured=1;
        }else{
          srvParam->WMSLayers = values[1].split(",");
        }
        dFound_WCSCOVERAGE=1;
      }

      // Service parameters
      if(value0Cap.equals("SERVICE")){
        values[1].toUpperCase();
        SERVICE.copy(values[1].c_str(),values[1].length());
        dFound_Service=1;
      }
      // Request parameters
      if(value0Cap.equals("REQUEST")){
        values[1].toUpperCase();
        REQUEST.copy(values[1].c_str(),values[1].length());
        dFound_Request=1;
      }


      // debug Parameters
      if(value0Cap.equals("DEBUG")){
        if(values[1].equals("ON")){
          printf("%s%c%c\n","Content-Type:text/plain",13,10);
          printf("Debug mode:ON\nDebug messages:<br>\r\n\r\n");
          dFound_Debug=1;
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
        values[1].toLowerCase();
        if(values[1].equals("true")){
          srvParam->showDimensionsInImage = true;
        }
      }
      if(value0Cap.equals("SHOWLEGEND")){
        values[1].toLowerCase();
        if(values[1].equals("true")){
          srvParam->showLegendInImage = true;
        }
      }
      if(value0Cap.equals("SHOWNORTHARROW")){
        values[1].toLowerCase();
        if(values[1].equals("true")){
          srvParam->showNorthArrow = true;
        }
      }
    }
    delete[] values;
  }
  delete[] parameters;


  if(dFound_Service==0){
    CDBWarning("Parameter SERVICE missing");
    dErrorOccured=1;
  }
  if(dFound_Styles==0){
    srvParam->Styles.copy("");
  }
  if(SERVICE.equals("WMS"))srvParam->serviceType=SERVICE_WMS;
  if(SERVICE.equals("WCS"))srvParam->serviceType=SERVICE_WCS;


  
  //srvParam->OpenDAPSource="/net/bhw262/nobackup/users/vreedede/ECMWF/U11_1313650800.nc";
  
    // If an Opendapsource is used, the LAYERS indicate which value should be visualised.
    // Add configuration layers to the XML object to make things work
    if(srvParam->OpenDAPSource.c_str()!=NULL){
      
    
      
      
      if(srvParam->isAutoOpenDAPEnabled()==false){
        CDBError("Automatic OpenDAP is not enabled");
        readyerror();exit(0);
      }
      
      bool isValidOpenDAPURL = false;
      if(srvParam->OpenDAPSource.indexOf("http://")==0)isValidOpenDAPURL=true;
      if(srvParam->OpenDAPSource.indexOf("https://")==0)isValidOpenDAPURL=true;
      if(srvParam->OpenDAPSource.indexOf("dodsc://")==0)isValidOpenDAPURL=true;
      if(srvParam->OpenDAPSource.indexOf("dods://")==0)isValidOpenDAPURL=true;
      if(srvParam->OpenDAPSource.indexOf("ncdods://")==0)isValidOpenDAPURL=true;

      if(isValidOpenDAPURL==false){
        CDBError("Invalid OpenDAP URL");
        readyerror();exit(0);
      }
      
      if(srvParam->OpenDapVariable.c_str()==NULL||srvParam->OpenDapVariable.equals("*")){
        //Try to retrieve a list of variables from the OpenDAPURL.
        srvParam->OpenDapVariable.copy("");
        CDFObject c;
        c.attachCDFReader(new CDFNetCDFReader());
        int status=c.open(srvParam->OpenDAPSource.c_str());
        if(status==0){
          
          for(size_t j=0;j<c.variables.size();j++){
            if(c.variables[j]->dimensionlinks.size()>=2){
              if(srvParam->OpenDapVariable.length()>0)srvParam->OpenDapVariable.concat(",");
              srvParam->OpenDapVariable.concat(c.variables[j]->name.c_str());
              CDBDebug("%s",c.variables[j]->name.c_str());
            }
          }
        }
        if(srvParam->OpenDapVariable.length()==0)status=1;
        if(status!=0){
          CDBError("A source parameter without a variable parameter is given");
          readyerror();exit(0);
        }
      }
    
      CT::stringlist *variables=srvParam->OpenDapVariable.splitN(",");
      for(size_t j=0;j<variables->size();j++){
        CServerConfig::XMLE_Layer *xmleLayer=new CServerConfig::XMLE_Layer();
        CServerConfig::XMLE_Variable* xmleVariable = new CServerConfig::XMLE_Variable();
        CServerConfig::XMLE_FilePath* xmleFilePath = new CServerConfig::XMLE_FilePath();
        CServerConfig::XMLE_Cache* xmleCache = new CServerConfig::XMLE_Cache();
        xmleCache->attr.enabled.copy("false");
        xmleLayer->attr.type.copy("database");
        xmleVariable->value.copy((*variables)[j]->c_str());
        xmleFilePath->value.copy(srvParam->OpenDAPSource.c_str());
        xmleFilePath->attr.filter.copy("*");
        xmleLayer->Variable.push_back(xmleVariable);
        xmleLayer->FilePath.push_back(xmleFilePath);
        xmleLayer->Cache.push_back(xmleCache);
        srvParam->cfg->Layer.push_back(xmleLayer);
      }
      
      //Adjust online resource in order to pass on variable and source parameters
      CT::string onlineResource=srvParam->cfg->OnlineResource[0]->attr.value.c_str();
      CT::string stringToAdd;
      stringToAdd.concat("&source=");stringToAdd.concat(srvParam->OpenDAPSource.c_str());
      stringToAdd.concat("&variable=");stringToAdd.concat(srvParam->OpenDapVariable.c_str());
      stringToAdd.concat("&");
      stringToAdd.encodeURL();
      onlineResource.concat(stringToAdd.c_str());
      srvParam->cfg->OnlineResource[0]->attr.value.copy(onlineResource.c_str());
      CDBDebug("%s -- %s",srvParam->OpenDapVariable.c_str(),srvParam->OpenDAPSource.c_str());
      
      //CDBError("A");readyerror();exit(0);
      delete variables;
      
      
      //When an opendap source is added, we should not cache the getcapabilities
      if(srvParam->cfg->CacheDocs.size()==0){
        srvParam->cfg->CacheDocs.push_back(new CServerConfig::XMLE_CacheDocs());
      }
      srvParam->cfg->CacheDocs[0]->attr.enabled.copy("false");
      srvParam->enableDocumentCache=false;
    }
  
  // WMS Service
  if(dErrorOccured==0&&srvParam->serviceType==SERVICE_WMS){

    
    
    //Default is 1.1.1

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
    }
    
    if(srvParam->requestType==REQUEST_WMS_GETMAP||srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
        if(dFound_Format==0){
          CDBWarning("Parameter FORMAT missing");
          dErrorOccured=1;
        }else{
          
          //Mapping
          for(size_t j=0;j<srvParam->cfg->WMS[0]->WMSFormat.size();j++){
            if(srvParam->Format.equals(srvParam->cfg->WMS[0]->WMSFormat[j]->attr.name.c_str())){
              if(srvParam->cfg->WMS[0]->WMSFormat[j]->attr.format.c_str()!=NULL){
                srvParam->Format.copy(srvParam->cfg->WMS[0]->WMSFormat[j]->attr.format.c_str());
              }
              break;
            }
          }
          /*if(dataSource->cfgLayer->WMSFormat.size()>0){
            if(dataSource->cfgLayer->WMSFormat[0]->attr.name.equals("image/png32")){
              drawImage.setTrueColor(true);
            }
            if(dataSource->cfgLayer->WMSFormat[0]->attr.format.equals("image/png32")){
              drawImage.setTrueColor(true);
            }
          }*/
          
          // Set format
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
      
      if(dFound_Width==0){
        CDBWarning("Parameter WIDTH missing");
        dErrorOccured=1;
      }
      if(dFound_Height==0){
        CDBWarning("Parameter HEIGHT missing");
        dErrorOccured=1;
      }
      
      // When error is image, utilize full image size
      setErrorImageSize(srvParam->Geo->dWidth,srvParam->Geo->dHeight,srvParam->imageFormat);
      
    
      
      if(dFound_BBOX==0){
        CDBWarning("Parameter BBOX missing");
        dErrorOccured=1;
      }
      if(dFound_SRS==0){
        CDBWarning("Parameter SRS missing");
        dErrorOccured=1;
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
          if(dFound_X==0){
            CDBWarning("Parameter X missing");
            dErrorOccured=1;
          }
          if(dFound_Y==0){
            CDBWarning("Parameter Y missing");
            dErrorOccured=1;
          }
          int status =  process_wms_getfeatureinfo_request();
          if(status != 0) {
            if(status!=2){
              CDBError("WMS GetFeatureInfo Request failed");
            }else {
              printf("%s%c%c\n","Content-Type:text/plain",13,10);
              printf("No results from query.");
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
        CDBError("getcapabilities has failed");
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
  if(dErrorOccured==1){
    //CDBError("An error occured, stopping...");
    return 0;
  }
  CDBWarning("Invalid value for request. Supported requests are: getcapabilities, getmap, getfeatureinfo and getlegendgraphic");
//  StopWatch_Time("QUERY_STRING parsed");

  return 0;
}


int CRequest::updatedb(CT::string *tailPath,CT::string *layerPathToScan){

  int status;
  //Fill in all data sources from the configuratin object
  size_t numberOfLayers = srvParam->cfg->Layer.size();

  for(size_t layerNo=0;layerNo<numberOfLayers;layerNo++){
    CDataSource *dataSource = new CDataSource ();
    dataSources.push_back(dataSource);
    if(dataSource->setCFGLayer(srvParam,srvParam->configObj->Configuration[0],srvParam->cfg->Layer[layerNo],NULL,layerNo)!=0){
      return 1;
    }
  }

  srvParam->requestType=REQUEST_UPDATEDB;

  //CDataReader reader;
  CT::string tablesdone[numberOfLayers];
  int nrtablesdone=0;
  for(size_t j=0;j<numberOfLayers;j++){
    if(dataSources[j]->dLayerType==CConfigReaderLayerTypeDataBase){

      int i,found=0;
      //Make sure we are not updating the table twice

      for(i=0;i<nrtablesdone;i++){
        if(tablesdone[i].equals(dataSources[j]->cfgLayer->DataBaseTable[0]->value.c_str())){found=1;break;}
      }
      if(found==0){
        status = CDBFileScanner::updatedb(srvParam->cfg->DataBase[0]->attr.parameters.c_str(),dataSources[j],tailPath,layerPathToScan);
        if(status !=0){CDBError("Could not update db for: %s",dataSources[j]->cfgLayer->Name[0]->value.c_str());return 1;}

        //Remember that we did this table allready
        tablesdone[nrtablesdone].copy(dataSources[j]->cfgLayer->DataBaseTable[0]->value.c_str());
        nrtablesdone++;
      }
    }
  }

  
  
  //invalidate cache
  
  CT::string cacheFileName;
  srvParam->getCacheFileName(&cacheFileName);
  //printf("\nInvalidating cachefile %s...\n",cacheFileName.c_str());
  if(cacheFileName.length()>0){
    remove(cacheFileName.c_str());
  }
  /*
  CSimpleStore simpleStore;
  status = getDocFromDocCache(&simpleStore,NULL,NULL);  
  simpleStore.setStringAttribute("configModificationDate","needsupdate!");
  if(storeDocumentCache(&simpleStore)!=0)return 1;*/
  
  CDBDebug("***** Finished DB Update *****");
  return 0;
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
