#include "CDataPostProcessor.h"


const char *CDPPExecutor::className="CDPPExecutor";
const char *CDPPMSGCPPMask::className="CDPPMSGCPPMask";
const char *CDPPAXplusB::className="CDPPAXplusB";

const char *CDPPAXplusB::getId(){
  return "AX+B";
}
int CDPPAXplusB::isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource){
  if(proc->attr.algorithm.equals("ax+b")){
    return CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}
int CDPPAXplusB::execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource){
  if(isApplicable(proc,dataSource)!=CDATAPOSTPROCESSOR_RUNBEFOREREADING){
    return -1;
  }
  CDBDebug("Applying ax+b");
  for(size_t varNr=0;varNr<dataSource->getNumDataObjects();varNr++){
    dataSource->getDataObject(varNr)->hasScaleOffset=true;
    dataSource->getDataObject(varNr)->cdfVariable->setType(CDF_DOUBLE);
    CDF::Attribute *scale_factor = dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("scale_factor");
    CDF::Attribute *add_offset =  dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("add_offset");
    //Apply offset
    if(proc->attr.b.empty()==false){
      CT::string b;
      b.copy(proc->attr.b.c_str());
      if(add_offset==NULL){
        dataSource->getDataObject(varNr)->dfadd_offset=b.toDouble();
      }else{
        dataSource->getDataObject(varNr)->dfadd_offset+=b.toDouble();
      }
    }
    //Apply scale
    if(proc->attr.a.empty()==false){
      CT::string a;
      a.copy(proc->attr.a.c_str());
      if(scale_factor==NULL){
        dataSource->getDataObject(varNr)->dfscale_factor=a.toDouble();
      }else{
        dataSource->getDataObject(varNr)->dfscale_factor*=a.toDouble();
      }
    }
    if(proc->attr.units.empty()==false){
      dataSource->getDataObject(varNr)->units=proc->attr.units.c_str();
    }
  }
}


const char *CDPPMSGCPPMask::getId(){
  return "MSGCPP_MASK";
}
int CDPPMSGCPPMask::isApplicable(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource){
  if(proc->attr.algorithm.equals("msgcppmask")){
    if(dataSource->getNumDataObjects()!=2){
      CDBError("3 variables are needed for msgcppmask");
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPMSGCPPMask::execute(CServerConfig::XMLE_DataPostProc* proc, CDataSource* dataSource){
  if(isApplicable(proc,dataSource)!=CDATAPOSTPROCESSOR_RUNAFTERREADING){
    return -1;
  }
  CDBDebug("Applying msgcppmask");
  float *data1=(float*)dataSource->getDataObject(0)->cdfVariable->data;//sunz
  float *data2=(float*)dataSource->getDataObject(1)->cdfVariable->data;
  float fNodata1=(float)dataSource->getDataObject(0)->dfNodataValue;
  size_t l=(size_t)dataSource->dHeight*(size_t)dataSource->dWidth;
  float fa=72,fb=75; 
  if(proc->attr.b.empty()==false){CT::string b;b.copy(proc->attr.b.c_str());fb=b.toDouble();}
  if(proc->attr.a.empty()==false){CT::string a;a.copy(proc->attr.a.c_str());fa=a.toDouble();}
  for(size_t j=0;j<l;j++){
    if((data2[j]<fa&&data1[j]<fa)||(data2[j]>fb))data1[j]=fNodata1; else data1[j]=1;
  }
  dataSource->eraseDataObject(1);
  return 0;
}


CDPPExecutor::CDPPExecutor(){
  //CDBDebug("CDPPExecutor");
  dataPostProcessorList = new  CT::PointerList<CDPPInterface*>();
  dataPostProcessorList->push_back(new CDPPAXplusB());
  dataPostProcessorList->push_back(new CDPPMSGCPPMask());
}

CDPPExecutor::~CDPPExecutor(){
  //CDBDebug("~CDPPExecutor");
  delete dataPostProcessorList;
}

const CT::PointerList<CDPPInterface*> *CDPPExecutor::getPossibleProcessors(){
  return dataPostProcessorList;
}

int CDPPExecutor::executeProcessors( CDataSource *dataSource,int mode){
  //const CT::PointerList<CDPPInterface*> *availableProcs = getPossibleProcessors();
  //CDBDebug("executeProcessors, found %d",dataSource->cfgLayer->DataPostProc.size());
  for(size_t dpi=0;dpi<dataSource->cfgLayer->DataPostProc.size();dpi++){
    CServerConfig::XMLE_DataPostProc * proc = dataSource->cfgLayer->DataPostProc[dpi];
    for(size_t procId = 0;procId<dataPostProcessorList->size();procId++){
      int code = dataPostProcessorList->get(procId)->isApplicable(proc,dataSource);
      
      if(code == CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET){
        CDBError("Constraints for DPP %s are note met",dataPostProcessorList->get(procId)->getId());
      }
      
      /*Will be runned when datasource metadata been loaded */
      if(mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING){
        if(code&CDATAPOSTPROCESSOR_RUNBEFOREREADING){
          int status = dataPostProcessorList->get(procId)->execute(proc,dataSource);
          if(status != 0){
            CDBError("Processor %s failed execution, statuscode %d",dataPostProcessorList->get(procId)->getId(),status);
          }
        }
      }
      /*Will be runned when datasource data been loaded */
      if(mode == CDATAPOSTPROCESSOR_RUNAFTERREADING){
        if(code&CDATAPOSTPROCESSOR_RUNAFTERREADING){
          int status = dataPostProcessorList->get(procId)->execute(proc,dataSource);
          if(status != 0){
            CDBError("Processor %s failed execution, statuscode %d",dataPostProcessorList->get(procId)->getId(),status);
          }
        }
      }
    }
  }
  return 0;
}
