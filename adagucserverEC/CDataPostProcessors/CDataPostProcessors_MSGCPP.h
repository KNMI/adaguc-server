#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_MSGCPP_H
#define CDATAPOSTPROCESSOR_MSGCPP_H

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
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("%s: CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED", this->className);
    return CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED;
  } // TODO: Still need to implement for timeseries
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
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("%s: CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED", this->className);
    return CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED;
  } // TODO: Still need to implement for timeseries
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
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("%s: CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED", this->className);
    return CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED;
  } // TODO: Still need to implement for timeseries
};

#endif