/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2015-01-26
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

#ifndef CDATAPOSTPROCESSOR_H
#define CDATAPOSTPROCESSOR_H
#include "CDataSource.h"


#define CDATAPOSTPROCESSOR_NOTAPPLICABLE 1
#define CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET 2
#define CDATAPOSTPROCESSOR_RUNBEFOREREADING 4
#define CDATAPOSTPROCESSOR_RUNAFTERREADING  8



  class CDPPInterface{
  public:
    virtual ~CDPPInterface(){}
    virtual const char *getId() = 0;
    /**
    * Checks the proc configuration element whether this processor should be executed
    * @params CServerConfig::XMLE_DataPostProc* the configuration object
    * @params CDataSource* the datasource to apply to
    * @returns |CDATAPOSTPROCESSOR_RUNBEFOREREADING if is applicable, execute can be called before reading data.
    * @returns |CDATAPOSTPROCESSOR_RUNAFTERREADING if is applicable, execute can be called after reading data.
    * @returns CDATAPOSTPROCESSOR_NOTAPPLICABLE if not applicable, e.g. the configuration does not match the processor
    * @returns CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET if applicable but constraints are not met, e.g. datasource properties do not match
    */
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource) = 0;

    /**
    * Executes the data postprocessor on a datasource, on the full grid
    */
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode) = 0;

    /**
    * Executes the data postprocessor for a given array
    */
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems) = 0;
  };

  /**
  * AX + B algorithm 
  */
  class CDPPAXplusB : public CDPPInterface{
  private:
    DEF_ERRORFUNCTION();
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems){return 1;} // TODO: Still need to implement
  };
  
  /**
  * IncludeLayer algorithm 
  */
  class CDPPIncludeLayer : public CDPPInterface{
  private:
    DEF_ERRORFUNCTION();
    class Settings{
    public:
      size_t width;
      size_t height;
      void *data;
    };
        
    template <class T>
    static void drawFunction(int x,int y,T val, void *_settings){
      Settings*settings = (Settings*)_settings;
      if(x>=0&&y>=0&&x<(int)settings->width&&y<(int)settings->height){
        ((T*)settings->data)[x+y*settings->width]=val;
      }
    };
    CDataSource *getDataSource(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    int setDimsForNewDataSource(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,CDataSource*dataSourceToInclude);
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems){return 1;} // TODO: Still need to implement
  };

  
 /**
  * MASK algorithm 
  */
  
  class CDPPDATAMASK : public CDPPInterface{
  private:
    DEF_ERRORFUNCTION();
    template <typename TT,typename SS>
    class DOIT{
    public:
    static void doIt      (void *newData,void* OriginalData,void* maskData,double newDataNoDataValue,CDFType maskType,double a,double b,double c,int mode,size_t size);
    static void reallyDoIt(void *newData,void* OriginalData,void* maskData,double newDataNoDataValue,CDFType maskType,double a,double b,double c,int mode,size_t size);
    };
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems){return 1;} // TODO: Still need to implement
  };

  /**
  * MSGCPP VISIBLE-mask, based on sunz and satz. a is sunz+satz threshold and b is satz threshold
  * 
  * < Layer type="database">
    < Group value="auxiliary" />
    < Name force="true">mask< /Name>
    < Title>Mask (-)< /Title>
    < DataBaseTable>msgcpp_0001< /DataBaseTable>
    < Variable>sunz< /Variable>
    < Variable>satz< /Variable>
    < RenderMethod>nearest< /RenderMethod>
    < FilePath filter="^SEVIR_OPER_R___MSGCPP__L2.*\ .nc$" >/data/ogcrt/data/temporary/< /FilePath>
    < Styles>mask,red,green,blue< /Styles>
    < Dimension name="time" interval="PT15M">time< /Dimension>
    < LatLonBox minx="-80" maxx="80" miny="-82" maxy="82" />
    < Cache enabled="false" />
    < DataPostProc algorithm="msgcppvisiblemask" a="78" b="80" />
    < ImageText>source: EUMETSAT/KNMI< /ImageText>
  < /Layer>

  * 
  */
  class CDPPMSGCPPVisibleMask : public CDPPInterface{
   private:
    DEF_ERRORFUNCTION();
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems){return 1;} // TODO: Still need to implement
  };
  
  /**
    * MSGCPP HIWC-mask
    * thresholds for detection of “High Ice Water Content” according to HAIC case
    * -          The cloud phase is ice
    * -          Cloud water path is > 0.1 kg/m2
    * -          Cloud top temperature < 270 K
    * -          Cloud optical thickness > 20
    *  < Layer type="database">
    *      < Group value="auxiliary" />
    *      < Name force="true">hiwc< /Name>
    *      < Title>High Ice Water Content (-)< /Title>
    *      < DataBaseTable>msgcpp_0001< /DataBaseTable>
    *      < Variable>cph< /Variable>
    *      < Variable>cwp< /Variable>
    *      < Variable>ctt< /Variable>
    *      < Variable>cot< /Variable>
    *      < RenderMethod>nearest< /RenderMethod>
    *      < FilePath filter="^SEVIR_OPER_R___MSGCPP__L2.*\.nc$">/data/ogcrt/data/temporary/</FilePath>
    *      < Styles>red,green,blue,mask,gray_red,gray_green,gray_blue</Styles>
    *      < Dimension name="time" interval="PT15M">time< /Dimension>
    *      < LatLonBox minx="-80" maxx="80" miny="-82" maxy="82" />
    *      < Cache enabled="false" />
    *      < DataPostProc algorithm="msgcpphiwcmask" a="78" b="80" />
    *      < ImageText>source: EUMETSAT/KNMI< /ImageText>
    *      < /Layer>
    * 
  */
  class CDPPMSGCPPHIWCMask : public CDPPInterface{
   private:
    DEF_ERRORFUNCTION();
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems){return 1;} // TODO: Still need to implement
  };

  class CDPPExecutor{
     private:
    DEF_ERRORFUNCTION();
  public:
    CT::PointerList<CDPPInterface*> *dataPostProcessorList;
    CDPPExecutor();
    ~CDPPExecutor();
    const CT::PointerList<CDPPInterface*> *getPossibleProcessors();
    int executeProcessors( CDataSource *dataSource,int mode);
    int executeProcessors( CDataSource *dataSource,int mode, double * data, size_t numItems);
  };
  
  /**
  * Calculate Beaufort algorithm 
  */
  class CDPPBeaufort : public CDPPInterface{
  private:
    DEF_ERRORFUNCTION();
    float getBeaufort(float speed);
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems){return 1;} // TODO: Still need to implement
  };

  /**
  * Calculate speed in knots algorithm 
  */
  class CDPPToKnots : public CDPPInterface{
  private:
    DEF_ERRORFUNCTION();
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems){return 1;} // TODO: Still need to implement
  };

  /**
  * Radar dbZ to Rain intensity algorithm 
  */
  class CDPDBZtoRR : public CDPPInterface{
  private:
    DEF_ERRORFUNCTION();
    float getRR(float dbZ);
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems);
  };
  
  /**
   * AddFeature from a GEOJSON shape provider
   */
  class CDPPAddFeatures : public CDPPInterface{
  private:
    DEF_ERRORFUNCTION();
//     float addFeature(float speed);
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode, double *data, size_t numItems){return 1;} // TODO: Still need to implement
  };
  
  
  class CDataPostProcessor{
  public:
    static CDPPExecutor *getCDPPExecutor();
  };

 
#endif
