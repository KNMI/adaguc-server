#include "CDataSource.h"

#ifndef CDPPInterface_H
#define CDPPInterface_H

#define CDATAPOSTPROCESSOR_NOTAPPLICABLE 1
#define CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET 2
#define CDATAPOSTPROCESSOR_RUNBEFOREREADING 4
#define CDATAPOSTPROCESSOR_RUNAFTERREADING 8
#define CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED 128

class CDPPInterface {
public:
  virtual ~CDPPInterface() {}
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
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) = 0;

  /**
   * Executes the data postprocessor on a datasource, on the full grid
   */
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) = 0;

  /**
   * Executes the data postprocessor for a given array
   */
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems) = 0;
};

#endif