#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_H
#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_H
/**
 * CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_H algorithm
 */

#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_U_COMPONENT "org_dpp_u_component"
#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_V_COMPONENT "org_dpp_v_component"
#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ID "UVCOMPONENTS"
#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_SPEED_COMPONENT "speed_component_object"
#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_DIRECTION_COMPONENT "direction_component_object"

class CDDPUVComponents : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t);
};

#endif