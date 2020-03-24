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

#ifndef CRequest_H
#define CRequest_H

#include <sys/stat.h>
#include <set>
#include "CImageDataWriter.h"
#include "CServerParams.h"
#include "CDataSource.h"
#include "CStopWatch.h"
#include "CXMLGen.h"
#ifdef ADAGUC_USE_GDAL
#include "CGDALDataWriter.h"
#endif
#include "CCreateHistogram.h"
#include "CDFObjectStore.h"
#include "CCDFStore.h"
#include "CDebugger.h"


class CSimpleStore{
  private:
    DEF_ERRORFUNCTION();
    CT::string cacheBuffer;
    class Attribute{
      public:
        CT::string name;
        size_t start,end;
        CT::string data;
    };
    std::vector<Attribute *>attributeList;
    size_t headerSize;
  public:
    CSimpleStore(){
      headerSize=0;
    }
    ~CSimpleStore(){
      for(size_t j=0;j<attributeList.size();j++){
        delete attributeList[j];
        attributeList[j]=NULL;
      }
    }
    int parse(const char *_cacheBuffer){
      cacheBuffer.copy(_cacheBuffer);
      if(cacheBuffer.length()==0){
        CDBError("no contents");
        return 1;
      }
      int a=cacheBuffer.indexOf("[/header]");
      if(a>0){
        headerSize=a+10;
        CT::string header;
        header.copy(cacheBuffer.c_str(),a-1);
        CT::string *lines = header.splitToArray("\n");
        for(size_t j=0;j<(size_t)lines->count;j++){
          CT::string *params=lines[j].splitToArray(",");
          if(params->count==4){
            if(params[0].equals("string")){
              
              
              Attribute * attr = new Attribute();
              attributeList.push_back(attr);
              attr->name.copy(&params[1]);
              attr->start = atoi(params[2].c_str());
              attr->end = attr->start+atoi(params[3].c_str());
            }
          }
          delete[] params;
        }
        
        delete[] lines;

      }else {
        CDBError("no [/header] found");
        return 2;
      }
      
      return 0;
    }
    int getBuffer(CT::string *_cacheBuffer){
      CT::string *c=_cacheBuffer;
      c->copy("");
      size_t start=0;
      for(size_t j=0;j<attributeList.size();j++){
        c->concat("string,");
        c->concat(&attributeList[j]->name);
        c->concat(",");
        if(attributeList[j]->data.length()==0){
          attributeList[j]->data.substringSelf(&cacheBuffer,attributeList[j]->start+headerSize,attributeList[j]->end+headerSize);
        }
        size_t length=attributeList[j]->data.length();
        //One byte longer because of carriage returns
        //if(j<attributeList.size()-1)length++;
        c->printconcat("%d,%d\n",start,length);
        start+=length;
      }
      c->concat("[/header]\n");
      for(size_t j=0;j<attributeList.size();j++){
        c->concat(&attributeList[j]->data);
        //c->concat("\n");
      }

      return 0;
    }
    int getCTStringAttribute(const char *name,CT::string* dest){
      for(size_t j=0;j<attributeList.size();j++){
        if(attributeList[j]->name.equals(name)){
          if(attributeList[j]->data.length()==0){
            dest->substringSelf(&cacheBuffer,attributeList[j]->start+headerSize,attributeList[j]->end+headerSize);
            attributeList[j]->data.copy(dest);
          }else{
            dest->copy(&attributeList[j]->data);
          }
          return 0;
        }
      }
      dest->copy("");
      return 1;
      
    }
    int setStringAttribute(const char *name,const char *data){
      Attribute *attr=NULL;
      for(size_t j=0;j<attributeList.size();j++){
        if(attributeList[j]->data.length()==0){
          attributeList[j]->data.substringSelf(&cacheBuffer,attributeList[j]->start+headerSize,attributeList[j]->end+headerSize);
        }
        if(attributeList[j]->name.equals(name)){
          attr=attributeList[j];
        }
      }
      if(attr==NULL){
        attr = new Attribute;
        attr->name.copy(name);
        attributeList.push_back(attr);
      }
      attr->data.copy(data);
      return 0;
    }
    
};


class CRequest{
private:
    std::vector <CDataSource*>dataSources;
    CT::string Version;
    CT::string Exceptions;
    CServerParams *srvParam;
    std::vector <CT::string*> queryDims;
    DEF_ERRORFUNCTION();
    int getDocFromDocCache(CSimpleStore *simpleStore,CT::string *docName,CT::string *document);
    int getDocumentCacheName(CT::string *documentName,CServerParams *srvParam);
    int storeDocumentCache(CSimpleStore *simpleStore);
    int generateOGCGetCapabilities(CT::string *XMLdocument);
    int generateGetReferenceTimes(CDataSource *dataSource);
    int generateGetReferenceTimesDoc(CT::string *result,CDataSource *dataSource);
    int generateOGCDescribeCoverage(CT::string *XMLdocument);
    
    
    
    
  public:
    static int setDimValuesForDataSource(CDataSource *dataSource,CServerParams *srvParam);
    static int queryDimValuesForDataSource(CDataSource *dataSource,CServerParams *srvParam);
    static int fillDimValuesForDataSource(CDataSource *dataSource,CServerParams *srvParam);
    
    CRequest(){
      srvParam=new CServerParams();
    }
    ~CRequest(){
      for(size_t j=0;j<dataSources.size();j++){
        if(dataSources[j]!=NULL){
          delete dataSources[j];
          dataSources[j]=NULL;
        }
      }
      for(size_t j=0;j<queryDims.size();j++){
        delete queryDims[j];
        queryDims[j]=NULL;
      }
      delete srvParam;
    }
    static int CGI;
    int process_querystring();
    int setConfigFile(const char *pszConfigFile);
    int process_wms_getcap_request();
    int process_wms_getmap_request();
    int process_wms_getmetadata_request();
    int process_wms_getfeatureinfo_request();
    int process_wms_getstyles_request();
    int process_wms_getlegendgraphic_request();
    int process_wcs_getcap_request();
    int process_wcs_describecov_request();
    int process_wcs_getcoverage_request();
    int process_all_layers();
    int process_wms_getreferencetimes_request();
    int process_wms_gethistogram_request();
    int updatedb(CT::string *tailPath,CT::string *layerPathToScan,int scanFlags, CT::string layerName);
    
    int runRequest();

    static void getCacheFileName(CT::string *cacheFileName,CServerParams *srvParam);
 

};

class CImageDataWriter_addData_args{
public:
  CImageDataWriter *imageDataWriter;
  std::vector <CDataSource*>dataSources;
  int status;
  bool running;
  bool finished;
  bool used;
};

void *CImageDataWriter_addData(void *arg);


#endif

