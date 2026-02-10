#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_H
#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_H
/**
 * CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_H algorithm
 */

#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_U_COMPONENT "org_dpp_u_component"
#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_V_COMPONENT "org_dpp_v_component"
#define CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ID "convert_uv_components"

#define U_COMPONENT_GRID_ABSOLUTE "eastward_component" // TODO PREFIX
#define V_COMPONENT_GRID_ABSOLUTE "northward_component"
#define SPEED_COMPONENT "speed_component"
#define DIRECTION_COMPONENT "direction_component"

class CDDPUVComponents : public CDPPInterface {
public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t);
};

#endif