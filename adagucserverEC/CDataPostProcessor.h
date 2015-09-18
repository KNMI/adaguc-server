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
    * Executes the data postprocessor
    */
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode) = 0;
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
  };

  /**
  * MSGCPP VISIBLE-mask, based on sunz and satz. a is sunz+satz threshold and b is satz threshold
  */
  class CDPPMSGCPPVisibleMask : public CDPPInterface{
   private:
    DEF_ERRORFUNCTION();
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
  };
  
  /**
    * MSGCPP HIWC-mask
    * thresholds for detection of “High Ice Water Content” according to HAIC case
    * -          The cloud phase is ice
    * -          Cloud water path is > 0.1 kg/m2
    * -          Cloud top temperature < 270 K
    * -          Cloud optical thickness > 20
  */
  class CDPPMSGCPPHIWCMask : public CDPPInterface{
   private:
    DEF_ERRORFUNCTION();
  public:
    virtual const char *getId();
    virtual int isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource);
    virtual int execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource,int mode);
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
  };
#endif