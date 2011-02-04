#include "CCDFDataModel.h"
const char *CDF::Variable::className="Variable";
const char *CDFObject::className="CDFObject";

int CDF::getTypeSize(CDFType type){
  if(type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE)return 1;
  if(type == CDF_SHORT || type == CDF_USHORT)return 2;
  if(type == CDF_INT || type == CDF_UINT)return 4;
  if(type == CDF_FLOAT)return 4;
  if(type == CDF_DOUBLE)return 8;
  return 0;
}

//Data must be freed with free()
int CDF::allocateData(CDFType type,void **p,size_t length){
  if((*p)!=NULL)free(*p);(*p)=NULL;
  size_t typeSize=1;
  if(type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE){
    typeSize=1;
    *p = malloc(length*typeSize);
    return 0;
  }
  if(type == CDF_SHORT || type == CDF_USHORT){
    typeSize=2;
    *p = malloc(length*typeSize);
    return 0;
  }
  if(type == CDF_INT || type == CDF_UINT){
    typeSize=4;
    *p = malloc(length*typeSize);
    return 0;
  }
  if(type == CDF_FLOAT){
    typeSize=4;
    *p = malloc(length*typeSize);
    return 0;
  }
  if(type == CDF_DOUBLE){
    typeSize=8;
    *p = malloc(length*typeSize);
    return 0;
  }
  return 1;
}
void CDF::getCDFDataTypeName(char *name,size_t maxlen,int type){
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

void CDF::getCDataTypeName(char *name,size_t maxlen,int type){
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
  if(data!=NULL)return 0;
  if(cdfReaderPointer==NULL){
    CDBError("No CDFReader defined for variable %s",name.c_str());
    return 1;
  }
  CDFReader *cdfReader = (CDFReader *)cdfReaderPointer;
  int status = cdfReader->readVariableData(this, type);
  if(status!=0){
    CDBError("Unable to read data for variable %s",name.c_str());
    return 1;
  }
  return 0;
}
