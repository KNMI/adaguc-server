#include "CCDFDataModel.h"
const char *CDF::Variable::className="Variable";
const char *CDFObject::className="CDFObject";
extern Tracer NewTrace;
DEF_ERRORMAIN()
int CDF::getTypeSize(CDFType type){
  if(type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE)return 1;
  if(type == CDF_SHORT || type == CDF_USHORT)return 2;
  if(type == CDF_INT || type == CDF_UINT)return 4;
  if(type == CDF_FLOAT)return 4;
  if(type == CDF_DOUBLE)return 8;
  return 0;
}

int CDF::freeData(void **p){
  if (Tracer::Ready)
    NewTrace.Remove (*p);
  free(*p);
  *p=NULL;
  return 0;
}


//Data must be freed with freeData()
int CDF::allocateData(CDFType type,void **p,size_t length){

  if((*p)!=NULL)freeData(p);(*p)=NULL;

  size_t typeSize= getTypeSize(type);
  if(typeSize==0){
    CDBError("In CDF::allocateData: Unknown type");
    return 1;
  }
  *p = malloc(length*typeSize);
  if (Tracer::Ready)NewTrace.Add (*p, __FILE__, __LINE__);

  return 0;
}

void CDF::getCDFDataTypeName(char *name,const size_t maxlen,const int type){
  snprintf(name,maxlen,"CDF_UNDEFINED");
  if(type==CDF_NONE  )snprintf(name,maxlen,"CDF_NONE");
  if(type==CDF_BYTE  )snprintf(name,maxlen,"CDF_BYTE");
  if(type==CDF_CHAR  )snprintf(name,maxlen,"CDF_CHAR");
  if(type==CDF_SHORT )snprintf(name,maxlen,"CDF_SHORT");
  if(type==CDF_INT   )snprintf(name,maxlen,"CDF_INT");
  if(type==CDF_FLOAT )snprintf(name,maxlen,"CDF_FLOAT");
  if(type==CDF_DOUBLE)snprintf(name,maxlen,"CDF_DOUBLE");
  if(type==CDF_UBYTE )snprintf(name,maxlen,"CDF_UBYTE");
  if(type==CDF_USHORT)snprintf(name,maxlen,"CDF_USHORT");
  if(type==CDF_UINT  )snprintf(name,maxlen,"CDF_UINT");
}

void CDF::getCDataTypeName(char *name,const size_t maxlen,const int type){
  snprintf(name,maxlen,"CDF_UNDEFINED");
  if(type==CDF_NONE  )snprintf(name,maxlen,"none");
  if(type==CDF_BYTE  )snprintf(name,maxlen,"uchar");
  if(type==CDF_CHAR  )snprintf(name,maxlen,"char");
  if(type==CDF_SHORT )snprintf(name,maxlen,"short");
  if(type==CDF_INT   )snprintf(name,maxlen,"int");
  if(type==CDF_FLOAT )snprintf(name,maxlen,"float");
  if(type==CDF_DOUBLE)snprintf(name,maxlen,"double");
  if(type==CDF_UBYTE )snprintf(name,maxlen,"ubyte");
  if(type==CDF_USHORT)snprintf(name,maxlen,"ushort");
  if(type==CDF_UINT  )snprintf(name,maxlen,"uint");
}

void CDF::getErrorMessage(char *errorMessage,const size_t maxlen,const int errorCode){
   snprintf(errorMessage,maxlen,"CDF_E_UNDEFINED");
   if(errorCode==CDF_E_NONE)snprintf(errorMessage,maxlen,"CDF_E_NONE");
   if(errorCode==CDF_E_DIMNOTFOUND)snprintf(errorMessage,maxlen,"CDF_E_DIMNOTFOUND");
   if(errorCode==CDF_E_ATTNOTFOUND)snprintf(errorMessage,maxlen,"CDF_E_ATTNOTFOUND");
   if(errorCode==CDF_E_VARNOTFOUND)snprintf(errorMessage,maxlen,"CDF_E_VARNOTFOUND");
   if(errorCode==CDF_E_NRDIMSNOTEQUAL)snprintf(errorMessage,maxlen,"CDF_E_NRDIMSNOTEQUAL");
   if(errorCode==CDF_E_VARHASNOPARENT)snprintf(errorMessage,maxlen,"CDF_E_VARHASNOPARENT");
   if(errorCode==CDF_E_VARHASNODATA)snprintf(errorMessage,maxlen,"CDF_E_VARHASNODATA");
   
}

void CDF::getErrorMessage(CT::string *errorMessage,const int errorCode){
  char msg[1024];
  getErrorMessage(msg,1023,errorCode);
  errorMessage->copy(msg);    
}
  


//const char *getAttributeAsString(CDF::Attribute *attr){
//}

void CDF::_dumpPrintAttributes(const char *variableName, std::vector<CDF::Attribute *>attributes,CT::string *dumpString){
  //print attributes:
  for(size_t a=0;a<attributes.size();a++){
    CDF::Attribute *attr=attributes[a];
    dumpString->printconcat("\t\t%s:%s =",variableName,attr->name.c_str());
    
    //print data
    char *data = new char[attr->length+1];
    memcpy(data,attr->data,attr->length);
    data[attr->length]='\0';
    if(attr->type==CDF_CHAR)dumpString->printconcat(" \"%s\"",data);
    //if(attr->type==CDF_UBYTE)dumpString->printconcat(" \"%s\"",data);
    //if(attr->type==CDF_BYTE)dumpString->printconcat(" \"%s\"",data);
    delete[] data;
    
    if(attr->type==CDF_BYTE||attr->type==CDF_UBYTE)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %db",((char*)attr->data)[n]);
    if(attr->type==CDF_INT||attr->type==CDF_UINT)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %d",((int*)attr->data)[n]);
    if(attr->type==CDF_SHORT||attr->type==CDF_USHORT)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %ds",((short*)attr->data)[n]);
    if(attr->type==CDF_FLOAT)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %ff",((float*)attr->data)[n]);
    if(attr->type==CDF_DOUBLE)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %fdf",((double*)attr->data)[n]);
    dumpString->printconcat(" ;\n");
  }
}
void CDF::dump(CDFObject* cdfObject,CT::string* dumpString){
  //print dimensions:
  char temp[1024];
  char dataTypeName[20];
  
  dumpString->copy("CCDFDataModel {\ndimensions:\n");
  
  for(size_t j=0;j<cdfObject->dimensions.size();j++){
    snprintf(temp,1023,"%s",cdfObject->dimensions[j]->name.c_str());
    dumpString->printconcat("\t%s = %d ;\n",temp,int(cdfObject->dimensions[j]->length));
    
  }
  dumpString->printconcat("variables:\n");
  for(size_t j=0;j<cdfObject->variables.size();j++){
    {
      CDF::getCDataTypeName(dataTypeName,19,cdfObject->variables[j]->type);
      snprintf(temp,1023,"\t%s %s",dataTypeName,cdfObject->variables[j]->name.c_str());
      dumpString->printconcat("%s",temp);
      if(cdfObject->variables[j]->dimensionlinks.size()>0){
        dumpString->printconcat("(");
        for(size_t i=0;i<cdfObject->variables[j]->dimensionlinks.size();i++){
          if(i>0&&i<cdfObject->variables[j]->dimensionlinks.size())dumpString->printconcat(", ");
          dumpString->printconcat("%s",cdfObject->variables[j]->dimensionlinks[i]->name.c_str());
        }
        dumpString->printconcat(")");
      }
      dumpString->printconcat(" ;\n");
      //print attributes:
      _dumpPrintAttributes(cdfObject->variables[j]->name.c_str(),
                           cdfObject->variables[j]->attributes,
                           dumpString);
    }
  }
  //print GLOBAL attributes:
  dumpString->concat("\n// global attributes:\n");
  _dumpPrintAttributes("",cdfObject->attributes,dumpString);
  dumpString->concat("}\n");
}

int CDF::Variable::readData(CDFType type){
  return readData(type,NULL,NULL,NULL);
}

int CDF::Variable::readData(CDFType type,size_t *_start,size_t *_count,ptrdiff_t *_stride){
#ifdef CCDFDATAMODEL_DEBUG          
  CDBDebug("reading variable %s",name.c_str());
#endif  
  //TODO needs to cope correctly with cdfReader.
  if(data!=NULL){
#ifdef CCDFDATAMODEL_DEBUG            
     CDBDebug("Data is already defined");
#endif     
    return 0;
  }
  //TODO NEEDS BETTER CHECKS
  if(cdfReaderPointer==NULL){
    CDBError("No CDFReader defined for variable %s",name.c_str());
    return 1;
  }
  
  
  
  //Check for iterative dimension
  bool needsDimIteration=false;
  int iterativeDimIndex=getIterativeDimIndex();
  if(iterativeDimIndex!=-1&&isDimension==false){
    needsDimIteration=true;
  }
  
  if(needsDimIteration==true){
    CDF::Dimension * iterativeDim;
    bool useStartCountStride=false;if(_start!=NULL&&_count!=NULL){useStartCountStride=true;}
    iterativeDim=dimensionlinks[iterativeDimIndex];
    //Make start and count params.
    size_t *start = new size_t[dimensionlinks.size()];
    size_t *count = new size_t[dimensionlinks.size()];
    ptrdiff_t *stride = new ptrdiff_t[dimensionlinks.size()];
    
    for(size_t j=0;j<dimensionlinks.size();j++){
      start[j]=0;
      count[j]=dimensionlinks[j]->getSize();
      stride[j]=1;
      
          if(useStartCountStride){
            start[j]=_start[j];//TODO in case of multiple readers for the same variable, this offset will change!
            count[j]=_count[j];
            stride[j]=_stride[j];
          }
    }
    
    //Allocate data for this chunk.
    size_t totalVariableSize = 1;
    for(size_t i=0;i<dimensionlinks.size();i++){
      totalVariableSize*=count[i];
    }
    setSize(totalVariableSize);
    int status=CDF::allocateData(type,&data,getSize());
    if(data==NULL||status!=0){
      CDBError("Variable data allocation failed, unable to allocate %d elements",totalVariableSize);
      return 1;
    }
    //Now make the iterative dim of length zero
    size_t iterDimStart=start[iterativeDimIndex];
    size_t iterDimCount=count[iterativeDimIndex];
#ifdef CCDFDATAMODEL_DEBUG        
    for(size_t i=0;i<dimensionlinks.size();i++){
      CDBDebug("%d\t%d",start[i],count[i]);
    }
#endif
   
    size_t dataReadOffset=0;
    for(size_t j=iterDimStart;j<iterDimCount+iterDimStart;j++){
      try{
        //Get the right CDF reader for this dimension set
        start[iterativeDimIndex]=j;count[iterativeDimIndex]=1;
        CDFObject *tCDFObject= (CDFObject *)getCDFObjectPointer(start,count);
        if(tCDFObject==NULL){CDBError("Unable to read variable %s because tCDFObject==NULL",name.c_str());throw(CDF_E_ERROR);}
        //Get the variable from this reader
        Variable *tVar=tCDFObject->getVariable(name.c_str());
        //
        for(size_t d=0;d<dimensionlinks.size();d++){
          if(useStartCountStride){
            start[d]=0;//TODO in case of multiple readers for the same variable, this offset will change!
            count[d]=_count[d];
            stride[d]=_stride[d];
          }else{
            start[d]=0;
            count[d]=dimensionlinks[d]->getSize();
            stride[d]=1;
          }      
        }
        count[iterativeDimIndex]=1;
        //Read the data!
        if(tVar->readData(type,start,count,stride)!=0)throw(__LINE__);
        //Put the read data chunk in our destination variable
#ifdef CCDFDATAMODEL_DEBUG                
        CDBDebug("Copying %d elements to variable %s",tVar->getSize(),name.c_str());
#endif        
        dataCopier.copy(data,tVar->data,type,dataReadOffset,0,tVar->getSize());dataReadOffset+=tVar->getSize();
        //Free the read data
#ifdef CCDFDATAMODEL_DEBUG                
        CDBDebug("Free tVar %s",tVar->name.c_str());
#endif        
        tVar->freeData();
#ifdef CCDFDATAMODEL_DEBUG                
        CDBDebug("Variable->data==NULL: %d",data==NULL);
#endif        
      }catch(int e){
        
        CDBError("Exception at line %d",e);
        delete[] start;delete[] count;delete[] stride;
        return 1;
      }
    }
    delete[] start;delete[] count;delete[] stride;
  }
  
  if(needsDimIteration==false){
    CDFReader *cdfReader = (CDFReader *)cdfReaderPointer;
    int status =0;
    bool useStartCountStride=false;if(_start!=NULL&&_count!=NULL){
      useStartCountStride=true;
      //When start and count are exactly the same as its dimension size, we do not need to use start,count and stride.
      bool dimSizesAreSameAsRequested=true;
      for(size_t j=0;j<dimensionlinks.size();j++){
        if(_start[j]==0&&dimensionlinks[j]->getSize()==_count[j]&&_stride[j]==1);else {
          dimSizesAreSameAsRequested=false;
          break;
        }
      }
      if(dimSizesAreSameAsRequested)useStartCountStride=false;
    }
    if(useStartCountStride==true){
      status = cdfReader->_readVariableData(this, type,_start,_count,_stride);
    }else{
      status = cdfReader->_readVariableData(this, type);
    }
    if(status!=0){
      CDBError("Unable to read data for variable %s",name.c_str());
      return 1;
    }
  }
  return 0;
}

 void CDF::Variable::setCDFObjectDim(CDF::Variable *sourceVar,const char *dimName){
  //if(sourceVar->isDimension)return;
  CDFObject *sourceCDFObject=(CDFObject*)sourceVar->getParentCDFObject();
  std::vector<Dimension *> &srcDims=sourceVar->dimensionlinks;
  std::vector<Dimension *> &dstDims=dimensionlinks;
  //Concerning dimensions need to have data in order to get this to work
  if(dstDims.size()!=srcDims.size()){throw(CDF_E_NRDIMSNOTEQUAL);}

  //Check which dims are iterative
  for(size_t j=0;j<dstDims.size();j++){
    //Test if the dimensions are the same
    if(!dstDims[j]->name.equals((srcDims)[j]->name.c_str())){
      CDBError("setCDFReaderForDim: Dimension names are unequal: %s !=%s ",dstDims[j]->name.c_str(),srcDims[j]->name.c_str());
      throw(CDF_E_ERROR);
    }
    //TODO Also check dimension units
    //Check which dimension is not yet iterative.
    if(dstDims[j]->isIterative==false){
      if(srcDims[j]->name.equals(dimName)){
        dstDims[j]->isIterative=true;
      }
    }
  }
  
  Dimension *iterativeDim;
  Variable *iterativeVar;
  try{    iterativeDim=getIterativeDim();  }catch(int e){    return;  }
  try{    iterativeVar=((CDFObject*)getParentCDFObject())->getVariable(iterativeDim->name.c_str());  }catch(int e){    return;  }
  
  //Read data from the source dim
  Variable *srcDimVar;
  try{
    srcDimVar = sourceCDFObject->getVariable(iterativeDim->name.c_str());
  }catch(int e){CDBError("Variable [%s] not found in source CDFObject",iterativeDim->name.c_str());throw(e);}
  

  
  
  if(srcDimVar->data==NULL){
    srcDimVar->readData(type);
  }
  if(srcDimVar->getSize()!=1){
    CDBError("srcDimVar->getSize()==%d",srcDimVar->getSize());
    throw("__LINE__");
  }
  double srcDimValue=srcDimVar->getDataAt<double>(0);
  
  //Check wether we already have this cdfobject dimension combo in our list
  int foundCDFObject = -1;
  for(size_t j=0;j<cdfObjectList.size();j++){
    //CDBDebug("%f==%f",cdfObjectList[j]->dimValue,srcDimValue);
    if(cdfObjectList[j]->dimValue==srcDimValue){foundCDFObject=j;break;}
  }
  if(foundCDFObject!=-1){
    CDBDebug("Found existing cdfObject %d",foundCDFObject);
  }else{
//    CDBDebug("cdfObjectList.push_back(new CDFObjectClass()) for variable %s size= %d",name.c_str(),cdfObjectList.size());
    CDFObjectClass *c=new CDFObjectClass();
    c->dimValue=srcDimValue;
    c->dimIndex=0;
    c->cdfObjectPointer=sourceVar->getParentCDFObject();
    cdfObjectList.push_back(c);
  }
  int foundDimValue = -1;
  if(iterativeVar->data==NULL){
    //Dit gaat van het verkeerde object!
    if(iterativeVar->readData(type)!=0){
      throw(0);
    }
  }

  for(size_t j=0;j<iterativeDim->getSize();j++){
    //CDBDebug("%f==%f",cdfObjectList[j]->dimValue,srcDimValue);
     //CDBDebug("3 %d ------------------ %f == %f",j,((double*)iterativeVar->data)[j],srcDimValue);
    if(((double*)iterativeVar->data)[j]==srcDimValue){foundDimValue=j;break;}
  }
  
  if(foundDimValue == -1){
    //Extend the concerning dimension
    size_t currentDimSize=iterativeDim->getSize();
    //if(currentDimSize!=cdfObjectList.size()){
//      CDBError("currentDimSize!=cdfObjectList.getSize()-1: %d %d",currentDimSize,cdfObjectList.size());throw(__LINE__);
  //  }
    void *dstData = NULL;
    void *srcData = NULL;
    int status = 0;
    status = allocateData(type,&dstData,currentDimSize+1);
    if(status!=0){
      CDBError("Unable to allocate data");
      throw("__LINE__");
    }
    status = allocateData(type,&srcData,1);
    if(status!=0){
      CDBError("Unable to allocate data");  
      throw("__LINE__");
    }
    
     dataCopier.copy(srcData,srcDimVar->data,type,1);
    //srcDimVar->getData(srcData,1);
#ifdef CCDFDATAMODEL_DEBUG             
    CDBDebug("Read data for %s = %f",srcDimVar->name.c_str(),((double*)srcData)[0]);
#endif    
  // for(int j=0;j<iterativeDim->getSize();j++){
    //  CDBDebug("1 %d------------------ %f",((double*)iterativeVar->data)[j]);
    //}

    dataCopier.copy(dstData,iterativeVar->data,type,0,0,currentDimSize);
    dataCopier.copy(dstData,srcData,type,currentDimSize,0,1);
    
    iterativeVar->freeData();
    iterativeVar->data=dstData;
    iterativeDim->setSize(currentDimSize+1);
    iterativeVar->setSize(currentDimSize+1);
    CDF::freeData(&srcData);
#ifdef CCDFDATAMODEL_DEBUG            
    CDBDebug("iterativeDim %d",iterativeDim->getSize());
#endif    
  }
  
  
  /*for(size_t j=0;j<dimensions->size();j++){
    if(dimensionlinks[j]->isIterative){
      Variable *srcVar,*dstVar ;
      try{srcVar = ((CDFObject *)sourceCDFObject)->getVariable(dimensionlinks[j]->name.c_str());}catch(int e){
        CDBError("Variable [%s] not found in source CDFObject",dimensionlinks[j]->name.c_str());throw(e);
      }
      try{dstVar = ((CDFObject *)getParentCDFObject())->getVariable(dimensionlinks[j]->name.c_str());}catch(int e){
        CDBError("Variable [%s] not found in destination CDFObject",dimensionlinks[j]->name.c_str());throw(e);
      }
      if(srcVar->data==NULL){CDBError("source dimension data not available");throw(CDF_E_ERROR);}
      if(dstVar->data==NULL){CDBError("destination dimension data not available");throw(CDF_E_ERROR);}
      size_t srcSize=(*dimensions)[j]->getSize();
      size_t dstSize=dimensionlinks[j]->getSize();
      double *srcData = new double[srcSize];
      double *dstData = new double[dstSize];
      int *foundSrcData = new int[srcSize];
      dataCopier.copy(srcData,srcVar->data,srcVar->type,srcSize);
      dataCopier.copy(dstData,dstVar->data,dstVar->type,dstSize);
      
      //Create Lookup 
      for(size_t h=0;h<srcSize;h++)foundSrcData[h]=0;
      try{
        for(size_t i=0;i<dstSize;i++){        
          for(size_t h=0;h<srcSize;h++){
            if(dstData[i]==srcData[h]){
              if(foundSrcData[h]==1){
                CDBError("CDFReader is already set for this dimension value!");
                throw(CDF_E_ERROR);
              }
              //printf("%f - %f\n",dstData[i],srcData[h]);
              //TODO Currently is i the correct CDFreader, but this will not be the case when using aggregation along multiple dims.
              //printf("%d - %d\n",i,h);
              CDBDebug("FOUND! %d == %d",i,cdfObjectList.size());
              //cdfObjectList[i].dimIndex=i;
              //cdfObjectList[i].cdfObjectPointer=sourceCDFObject;
              foundSrcData[h]=1;
            }
          }
        }
        //Check if all input dims were found...
        for(size_t h=0;h<srcSize;h++)if(foundSrcData[h]==0){
          CDBError("Source Dimension index not found in any of the destination dims");
          throw(CDF_E_ERROR);
        }
        throw(0);
      }
      catch(int e){
        delete[] foundSrcData;
        delete[] srcData;
        delete[] dstData;
        if(e!=0)throw(e);
      }
    }*/
    
    
    
  //  printf("%s %d - %s %d\n",(*dimensions)[j]->name.c_str(),(*dimensions)[j]->getSize(),dimensionlinks[j]->name.c_str(),dimensionlinks[j]->getSize());
  //}

  
}

CDFObject::~CDFObject(){
  //CDBDebug("********** Destroying cdfobject");
  clear();
  /*CDFReader *r=(CDFReader*)reader;
  delete r;r=NULL;
  reader=NULL;*/
  //for(size_t j=0;j<attributes.size();j++){delete attributes[j];attributes[j]=NULL;}
}
int CDFObject::attachCDFReader(void *reader){
  CDFReader *r=(CDFReader*)reader;
  r->cdfObject=this;
  this->reader=r;
  return 0;
}
void CDFObject::clear(){
  for(size_t j=0;j<dimensions.size();j++){delete dimensions[j];dimensions[j]=NULL;}
  for(size_t j=0;j<variables.size();j++){delete variables[j];variables[j]=NULL;}
}
int CDFObject::open(const char *fileName){
  //CDBDebug("Opening file %s (current =%s)",fileName,currentFile.c_str());
  if(currentFile.equals(fileName)){
    //CDBDebug("OK: Current file is already open");
    return 0;
  }
  CDFReader *r=(CDFReader*)reader;
   if(r==NULL){
    CDBError("No reader attached");return 1;
  }
  clear();
  currentFile.copy(fileName);
  return r->open(fileName);
}
int CDFObject::close(){
  if(reader==NULL){
    CDBError("No reader attached");return 1;
  }
  CDBDebug("Closing reader");
  CDFReader *r=(CDFReader*)reader;
  return r->close();
}
