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
#include "CDPPGoes16Metadata.h"
/**
 * AX + B algorithm
 */
class CDPPAXplusB : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};

/**
 * MASK algorithm
 */

class CDPPDATAMASK : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  template <typename TT, typename SS> class DOIT {
  public:
    static void doIt(void *newData, void *OriginalData, void *maskData, double newDataNoDataValue, CDFType maskType, double a, double b, double c, int mode, size_t size);
    static void reallyDoIt(void *newData, void *OriginalData, void *maskData, double newDataNoDataValue, CDFType maskType, double a, double b, double c, int mode, size_t size);
  };

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
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
class CDPPMSGCPPVisibleMask : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
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
class CDPPMSGCPPHIWCMask : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};

class CDPPExecutor {
private:
  DEF_ERRORFUNCTION();

public:
  CT::PointerList<CDPPInterface *> *dataPostProcessorList;
  CDPPExecutor();
  ~CDPPExecutor();
  const CT::PointerList<CDPPInterface *> *getPossibleProcessors();
  int executeProcessors(CDataSource *dataSource, int mode);
  int executeProcessors(CDataSource *dataSource, int mode, double *data, size_t numItems);
  int executeProcessors(CDataSource *dataSource, int mode, double *data, size_t numItems, double timestamp);
};

/**
 * Calculate Beaufort algorithm
 */
class CDPPBeaufort : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  float getBeaufort(float speed);

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};

/**
 * Calculate speed in knots algorithm
 */
class CDPPToKnots : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};

/**
 * Radar dbZ to Rain intensity algorithm
 */
class CDPDBZtoRR : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  float getRR(float dbZ);

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems);
};

/**
 * AddFeature from a GEOJSON shape provider
 */
class CDPPAddFeatures : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  //     float addFeature(float speed);
public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};

class CDataPostProcessor {
public:
  static CDPPExecutor *getCDPPExecutor();
};

#endif
