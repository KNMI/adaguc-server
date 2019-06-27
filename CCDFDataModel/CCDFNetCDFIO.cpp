/******************************************************************************
 * 
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
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

#include "CCDFNetCDFIO.h"
#include "CStopWatch.h"
const char *CDFNetCDFReader::className="NetCDFReader";
const char *CDFNetCDFWriter::className="NetCDFWriter";

#define CDFNetCDFGroupSeparator "/"
//const char *CCDFWarper::className="CCDFWarper";

//  #define CCDFNETCDFIO_DEBUG
//  #define CCDFNETCDFIO_DEBUG_OPEN
// #define CCDFNETCDFWRITER_DEBUG

CDFNetCDFReader::CDFNetCDFReader():CDFReader(){
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("New CDFNetCDFReader");
  #endif
  root_id=-1;
  keepFileOpen=false;
  
}
CDFNetCDFReader::~CDFNetCDFReader(){
  close();
}

void CDFNetCDFReader::enableLonWarp(bool enableLonWarp){
  //warper.enableLonWarp=enableLonWarp;
}
void CDFNetCDFReader::ncError(int line, const char *className, const char * msg,int e){
  if(e==NC_NOERR)return;
  char szTemp[1024];
  snprintf(szTemp,1023,"[E: %s, %d in class %s] %s: %s\n",__FILE__,line,className,msg,nc_strerror(e));
  printErrorStream(szTemp);
  
}

int CDFNetCDFReader::_readVariableData(CDF::Variable *var, CDFType type){
  return _readVariableData(var,type,NULL,NULL,NULL);
}

int CDFNetCDFReader::_readVariableData(CDF::Variable *var, CDFType type,size_t *start,size_t *count,ptrdiff_t *stride){
  int nDims,nVars,nRootAttributes,unlimDimIdP;    
  
  if(cdfCache!=NULL){
    #ifdef CCDFNETCDFIO_DEBUG_OPEN        
    CDBDebug("Looking into cache %s of type %s",var->name.c_str(),CDF::getCDFDataTypeName(type).c_str());
    #endif
    int cacheStatus = cdfCache->readVariableData(var, type,start,count,stride,false);
    if(cacheStatus == 0) {return 0;}
  }
  
  if(root_id==-1){
    #ifdef CCDFNETCDFIO_DEBUG_OPEN        
    CDBDebug("NC_OPEN re-opening %s for %s",fileName.c_str(),var->name.c_str());
    #endif      
    
    status = nc_open(fileName.c_str(),NC_NOWRITE,&root_id);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_open: ",status);return 1;}
    status = nc_inq(root_id,&nDims,&nVars,&nRootAttributes,&unlimDimIdP);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq: ",status);return 1;}
    #ifdef CCDFNETCDFIO_DEBUG_OPEN        
    CDBDebug("root_id %d",root_id);
    var->id = -1;
    CDBDebug("VARNAME %s id: %d",var->name.c_str(),var->id);
    #endif
    
    /*Check if var id is still OK*/
    char name[NC_MAX_NAME+1];
    nc_type type;
    int ndims;
    int natt;
    int dimids[NC_MAX_VAR_DIMS];
    //bool isDimension;
    for(int j=0;j<nVars;j++){
      int groupId = _findNCGroupIdForCDFVariable(&var->name);
      if(groupId == -1){
        CDBError("_findNCGroupIdForCDFVariable for %s = -1",var->name.c_str());
        return 1;
      }
      status = nc_inq_var(groupId,j,name,&type,&ndims,dimids,&natt);
      //CDBDebug("NAME EQUALS %s  = %s %d = %d",var->name.c_str(),name,var->id,j);
      if(var->name.equals(name)){
        
        var->id = j;
        break;
      }
    }
  }
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("reading %s with id %d from file %s",var->name.c_str(), var->id,fileName.c_str());
  #endif
  int varGroupId = _findNCGroupIdForCDFVariable(&var->name);
  if(varGroupId == -1){
    CDBError("_findNCGroupIdForCDFVariable for %s = -1",var->name.c_str());
    return 1;
  }
  //CDBDebug("readVariableData");
  //It is essential that the variable nows which reader can be used to read the data
  //var->cdfReaderPointer=(void*)this;
  //var->setCDFReaderPointer(this);
  var->freeData();
  
  
  
  
  bool useStartCount = false;
  bool useStriding=false;
  
  if(start!=NULL&&count!=NULL&&stride!=NULL){
    useStartCount = true;
  }
  
  size_t totalVariableSize = 1;
  
  if(useStartCount == false){
    for(size_t i=0;i<var->dimensionlinks.size();i++){
      totalVariableSize*=var->dimensionlinks[i]->length;
    }
  }
  
  if(useStartCount == true){
    for(size_t i=0;i<var->dimensionlinks.size();i++){
      totalVariableSize*=count[i];//stride[i];
      if(stride[i]!=1){
        useStriding=true;
      }
      #ifdef CCDFNETCDFIO_DEBUG        
      CDBDebug("%s: [%d %d %d]",var->dimensionlinks[i]->name.c_str(),start[i],count[i],stride[i]);
      #endif
    }
  }
  //CDBDebug("totalVariableSize = %d",totalVariableSize);
  
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("Setting variable size to %d",totalVariableSize);
  #endif
  var->setSize(totalVariableSize);
  
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("Allocating data for variable %s, type: %s, size: %d",var->name.c_str(),CDF::getCDFDataTypeName(var->currentType).c_str(),var->getSize());
  #endif
  CDF::allocateData(type,&var->data,var->getSize());
  
  
  if(type==CDF_STRING){
    if(var->isString()){
      /*Mimic char(numString,maxStrlen64) behaviour to CDF_STRING */
      size_t stringSize = var->dimensionlinks[1]->getSize();//e.g. maxStrlen64;
      size_t numStrings = var->dimensionlinks[0]->getSize();//The number of strings
      int step = 1;
      int beginAt=0;
      if(count != NULL)numStrings=count[0];
      if(stride != NULL)step=stride[0];
      if(start!=NULL)beginAt=start[0];
      char * tempData = new char[numStrings*stringSize];
      status = nc_get_var(varGroupId,var->id,tempData);
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_var_char (for CDF_STRING): ",status);}
      for(size_t j=0;j<numStrings;j=j+step){
        char*stringToAdd = tempData+(j+beginAt)*stringSize;
        size_t length = strlen(stringToAdd);
        ((char**)var->data)[j]=(char*)malloc(length+1);
        snprintf(((char**)var->data)[j],length+1,"%s",stringToAdd);
      }
      delete[] tempData;
    }else{
      if(useStartCount == true){
        if(useStriding){
          #ifdef CCDFNETCDFIO_DEBUG_OPEN        
          CDBDebug("READ NSCS: [%s]",var->name.c_str());
          #endif
          status = nc_get_vars_string(varGroupId,var->id,start,count,stride,(char**)var->data);
          if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vars (typeconversion): ",status);}
        }else{
          #ifdef CCDFNETCDFIO_DEBUG_OPEN        
          CDBDebug("READ NSC: [%s]",var->name.c_str());
          #endif
          status = nc_get_vara_string(varGroupId,var->id,start,count,(char**)var->data);
          if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vara (typeconversion): ",status);}
        }
      }else{
        #ifdef CCDFNETCDFIO_DEBUG_OPEN        
        CDBDebug("READ N: [%s]",var->name.c_str());
        #endif
        status = nc_get_var_string(varGroupId,var->id,(char**)var->data);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_var_string (typeconversion): ",status);}
      }
    }
    if(status!=NC_NOERR){
      char typeName[254];
      CDF::getCDFDataTypeName(typeName,255,var->currentType);
      CDBError("Problem with variable %s of type %s (requested %s):",var->name.c_str(),typeName,CDF::getCDFDataTypeName(type).c_str());
      ncError(__LINE__,className,"nc_get_var: ",status);
      return 1;
    }
    
    if(cdfCache!=NULL){
      CDBDebug("Putting into cache %s",var->name.c_str());
      int cacheStatus = cdfCache->readVariableData(var, type,start,count,stride,true);
      if(cacheStatus == 0) return 0;
    }  
    return 0;
  }
  
  //Data is requested with another type than requested. We will perform type conversion in the following piece of code.
  if(type!=var->nativeType){
    void * voidData = NULL;
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Allocating data for temp data");
    #endif
    
    CDF::allocateData(var->nativeType,&voidData,var->getSize());
    var->setType(type);  
    
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Allocated %d elements",var->getSize());
    #endif
    
    
    if(useStartCount == true){
      if(useStriding){
        #ifdef CCDFNETCDFIO_DEBUG_OPEN        
        CDBDebug("READ SCS: [%s]",var->name.c_str());
        #endif
        status = nc_get_vars(varGroupId,var->id,start,count,stride,voidData);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vars (typeconversion): ",status);}
      }else{
        #ifdef CCDFNETCDFIO_DEBUG_OPEN        
        CDBDebug("READ SC: [%s]",var->name.c_str());
        #endif
        status = nc_get_vara(varGroupId,var->id,start,count,voidData);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vara (typeconversion): ",status);}
      }
    }else{
      #ifdef CCDFNETCDFIO_DEBUG_OPEN        
      CDBDebug("READ: [%s]",var->name.c_str());
      #endif
      
      status = nc_get_var(varGroupId,var->id,voidData);
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_var (typeconversion): ",status);}
    }
    
    if(status!=NC_NOERR){
      char typeName[254];
      CDF::getCDFDataTypeName(typeName,255,var->currentType);
      CDBError("Problem with variable %s of type %s (requested %s):",var->name.c_str(),typeName,CDF::getCDFDataTypeName(type).c_str());
      ncError(__LINE__,className,"nc_get_var: ",status);
      return 1;
    }
    
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Copying %d elements from type %s to %s",var->getSize(),CDF::getCDFDataTypeName(var->nativeType).c_str(),CDF::getCDFDataTypeName(type).c_str());
    #endif
    
    
    CDF::DataCopier::copy(var->data,type,voidData,var->nativeType,0,0,var->getSize());
    
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Freeing temporary data object");
    #endif
    CDF::freeData(&voidData);
    
    //End of reading data and performing type conversion
  }
  
  if(type == var->nativeType){
    if(useStartCount){
      if(useStriding){
        #ifdef CCDFNETCDFIO_DEBUG_OPEN                
        CT::string dims="";
        for(size_t j=0;j<var->dimensionlinks.size();j++){
          if(j>0)dims.concat(",");
          dims.printconcat("%s[%d:%d:%d]",var->dimensionlinks[j]->name.c_str(),start[j],count[j],start[j]);
        }
        CDBDebug("READ NSCS: [%s](%s)",var->name.c_str(),dims.c_str());
        #endif
        status = nc_get_vars(varGroupId,var->id,start,count,stride,var->data);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vars (native): ",status);}
      }else{
        #ifdef CCDFNETCDFIO_DEBUG_OPEN        
        CT::string dims="";
        for(size_t j=0;j<var->dimensionlinks.size();j++){
          if(j>0)dims.concat(",");
          dims.printconcat("%s[%d:%d]",var->dimensionlinks[j]->name.c_str(),start[j],count[j]);
        }
        CDBDebug("READ NSC: [%s](%s)",var->name.c_str(),dims.c_str());
        #endif
        status = nc_get_vara(varGroupId,var->id,start,count,var->data);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vara (native): ",status);}
      }
    }else{
      #ifdef CCDFNETCDFIO_DEBUG_OPEN        
      CDBDebug("READ N: [%s]",var->name.c_str());
      #endif
      status = nc_get_var(varGroupId,var->id,var->data);
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_var (native): ",status);}
    }
    if(status!=NC_NOERR){
      char typeName[254];
      CDF::getCDFDataTypeName(typeName,255,var->currentType);
      CDBError("Problem with variable %s of type %s (requested %s):",var->name.c_str(),typeName,CDF::getCDFDataTypeName(type).c_str());
      ncError(__LINE__,className,"nc_get_var: ",status);
      return 1;
    }
    //End of reading data natively.
  }
  
  //   if(var->currentType == CDF_FLOAT){
  //   
  //     float min=0,max=1;
  //     for(size_t j=0;j<var->getSize();j++){
  //       float v = ((float*)var->data)[j];
  //       if(j==0){
  //         min=v;max=v;
  //       }
  //       if(v>max)max=v;
  //       if(v<min)min=v;
  //     }
  //     CDBDebug("MAX/MIN=%f/%f",min,max);
  //   }
  
  
  //warper.warpLonData(var);
  
  if(cdfCache!=NULL){
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Putting into cache %s",var->name.c_str());
    #endif
    int cacheStatus = cdfCache->readVariableData(var, type,start,count,stride,true);
    if(cacheStatus == 0) return 0;
  }  
  
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("Ready.");
  #endif
  return 0;
}


int CDFNetCDFReader::readDimensions(int groupId,CT::string *groupName){
  char flatname[NC_MAX_NAME+1];
  size_t length;
  int nDims;
  status = nc_inq_ndims (groupId,&nDims);
//   CDBDebug("Found %d dims",nDims);
  
  status = nc_inq_dimids(groupId , &nDims, NULL, 0 );
  
//   CDBDebug("Found %d dims",nDims);
  
  if(status!=NC_NOERR){CDBError("For groupName %s: ",groupName->c_str());ncError(__LINE__,className,"nc_inq_dimids: ",status);return 1;}
  int dimIds[nDims];
  status = nc_inq_dimids(groupId , &nDims, dimIds, 0 );
  if(status!=NC_NOERR){CDBError("For groupName %s: ",groupName->c_str());ncError(__LINE__,className,"nc_inq_dimids: ",status);return 1;}
  
  if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_ndims: ",status);return 1;}
  for(int j=0;j<nDims;j++){
    status = nc_inq_dim(groupId,dimIds[j],flatname,&length);

    if(status!=NC_NOERR){CDBError("For groupName %s: ",groupName->c_str());ncError(__LINE__,className,"nc_inq_dim: ",status);return 1;}
    CT::string name=groupName->c_str();
    name.concat(flatname);
    try{
      CDF::Dimension * existingDim = cdfObject->getDimension(name.c_str());
      //Only add non existing variables;
      CDBWarning("Reassigning dim %s",name.c_str());
      if(existingDim->length!=length){
        CDBError("Previously dimensions size for dim %s is not the same as new definition",name.c_str());
        return 1;
      }
    }catch(...){
      
      CDF::Dimension * dim = new CDF::Dimension();
      dim->id=dimIds[j];
      dim->setName(name.c_str());
      dim->length=length;
      
      cdfObject->dimensions.push_back(dim);
    }
  }
  return 0;
}

int CDFNetCDFReader::readAttributes(int root_id,std::vector<CDF::Attribute *> &attributes,int varID,int natt){
  char name[NC_MAX_NAME+1];
  nc_type type;
  size_t length;
  for(int i=0;i<natt;i++){
    status = nc_inq_attname(root_id,varID,i,name);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_attname: ",status);return 1;}
    //Only add non existing attributes;
    int attributeExists=false;
    
    for(size_t k=0;k<attributes.size();k++){if(attributes[k]->name.equals(name)){attributeExists=true;break;}}
    if(attributeExists==false){
      status = nc_inq_att(root_id,varID,name,&type,&length);
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_att: ",status);return 1;}
      CDF::Attribute *attr = new CDF::Attribute();
      attr->setName(name);
      attr->type=_typeConversionAtt(type);
      if(attr->type == CDF_UNKNOWN){
        CDBWarning("Unknown attribute type for attribute %s %d %d %d",name,type,attr->type,attr->length);
      }
      //CDBDebug("%s %d %d %d",name,type,attr->type,attr->length);
      attr->length=length;
      CDF::allocateData(attr->getType(),&attr->data,attr->length+1);
      if(type!=NC_STRING){
        status = nc_get_att(root_id,varID,name,attr->data);
        if(type==NC_CHAR)((char*)attr->data)[attr->length]='\0';
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_att: ",status);return 1;}
      }else{
        status = nc_get_att_string(root_id,varID,name,(char**)attr->data);
        if(type==NC_CHAR)((char*)attr->data)[attr->length]='\0';
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_att: ",status);return 1;}
        //CDBDebug("units %s",((char**)attr->data)[0]);
      }
      attributes.push_back(attr);
    }
  }
  return 0;
}


int CDFNetCDFReader::_findNCGroupIdForCDFVariable(CT::string *varName){
  CT::string * paths=varName->splitToArray(CDFNetCDFGroupSeparator);
  if(paths->count <= 1){
    delete[] paths;
    return root_id;
  }
  int currentId=root_id;
  for(size_t j=0;j<paths->count-1;j++){
    int grp_ncid;
    status = nc_inq_ncid(currentId, paths[j].c_str(), &grp_ncid);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_ncid: ",status);delete[] paths;return -1;}
    currentId = grp_ncid;
  }
  delete[] paths;
  return currentId;
};


int CDFNetCDFReader::readVariables(int groupId,CT::string *groupName,int mode){
  int nGroups;
  status = nc_inq_grps(groupId,&nGroups,NULL);
  if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_grpname_full: ",status);return 1;}
  
  
  if(nGroups>0){
    int *groupIds = new int[nGroups];
    status = nc_inq_grps(groupId, NULL, groupIds);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_grps: ",status);delete[] groupIds;return 1;}
    
    for(int g=0;g<nGroups;g++){
      char foundGroupName[NC_MAX_NAME + 1];
      status = nc_inq_grpname(groupIds[g], foundGroupName);
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_grpname: ",status);delete[] groupIds;return 1;}
      CT::string newGroupName;
      if(groupName->length()>0){
        newGroupName.print("%s%s%s",groupName->c_str(),foundGroupName,CDFNetCDFGroupSeparator);
      }else{
        newGroupName.print("%s%s",foundGroupName,CDFNetCDFGroupSeparator);
      }
      status = readVariables(groupIds[g],&newGroupName,mode);
      if(status!=0){CDBError("readVariables failed for group [%s]",newGroupName.c_str());delete[] groupIds;return 1;}
    }
    
    delete[] groupIds;
  }
  #ifdef CCDFNETCDFWRITER_DEBUG           
  CDBDebug("Start reading group [%s] with id [%d]",groupName->c_str(),groupId);
  #endif  
  
  if(mode == 0){
    status = readDimensions(groupId,groupName);if(status!=0)return 1;
    if(status!=0){CDBError("readDimensions failed for group [%s]",groupName);return 1;}
    #ifdef MEASURETIME
    StopWatch_Stop("readDim");
    #endif
    return 0;
  }
  
  //  status = nc_inq_grpname_full (int groupId, size_t *lenp, char *full_name);
  //  if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_grpname_full: ",status);return 1;}
  
  char flatname[NC_MAX_NAME+1];
  nc_type type;
  int ndims;
  int natt;
  int dimids[NC_MAX_VAR_DIMS];
  bool isDimension;
  int nVars;
  status = nc_inq_nvars(groupId,&nVars);
  if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_nvars: ",status);return 1;}
  for(int j=0;j<nVars;j++){
    status = nc_inq_var(groupId,j,flatname,&type,&ndims,dimids,&natt);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_var: ",status);return 1;}
    
    CT::string name=groupName->c_str();
    name.concat(flatname);
    
//     CDBDebug("%s Numdims NC : %d",name.c_str(),ndims);
    
    //Only add non existing variables...
    try{
      cdfObject->getVariable(name.c_str());
    }catch(...){
      CDF::Variable * var = new CDF::Variable();
      cdfObject->variables.push_back(var);
      isDimension = false;
      //Is this a dimension:
      for(size_t i=0;i<cdfObject->dimensions.size();i++){if(cdfObject->dimensions[i]->name.equals(name.c_str())){isDimension=true;break;}}
      //Dimension links:
      
//       CDBDebug("%s Numdims: %d",name.c_str(),var->dimensionlinks.size());
      
      for(int k=0;k<ndims;k++){
        bool foundDim = false;
        for(size_t i=0;i<cdfObject->dimensions.size();i++){
          if(cdfObject->dimensions[i]->id==dimids[k]){
//             CDBDebug("%s %d %d",name.c_str(),cdfObject->dimensions[i]->id,dimids[k]);
            var->dimensionlinks.push_back(cdfObject->dimensions[i]);
            foundDim = true;
            break;
          }
        }
        if(foundDim == false){
          CDBError("For variable [%s] unable to find dimensionwith id %d: Nr of needed dims: %d, numdims found in cdfobject: %d",name.c_str(),dimids[k],ndims,cdfObject->dimensions.size());
          for(size_t j=0;j<cdfObject->dimensions.size();j++){
            CDBDebug("%d: %s with id %d",j,cdfObject->dimensions[j]->name.c_str(),cdfObject->dimensions[j]->id);
          }
          return 1;
        }
      }
      
      
//       CDBDebug("%s Numdims: %d",name.c_str(),var->dimensionlinks.size());
      
      
      
      //Attributes:
      status = readAttributes(groupId,var->attributes,j,natt);if(status!=0)return 1;
      
      //Check for signed/unsigned status via opendap 
      bool varIsUnsigned = false;
      CDF::Attribute *_Unsigned = var->getAttributeNE("_Unsigned");
      if(_Unsigned != NULL){
        if(_Unsigned->toString().equals("true")){
          varIsUnsigned = true;
        }
      }
      
      //Variable type
      nc_type thisType = _typeConversionVar(type,varIsUnsigned);
      
      if(varIsUnsigned){
        CDF::Attribute *FillValue = var->getAttributeNE("_FillValue");
        if(FillValue != NULL){
          FillValue->type=thisType;
        }
      }
      
      if(thisType == CDF_CHAR && var->dimensionlinks.size() == 2){
        if(var->dimensionlinks[1]->name.equals("maxStrlen64")){
          var->dimensionlinks.pop_back();
          thisType = CDF_STRING;
          var->isString(true);
        }
      }
      
      var->setType(thisType);
      var->nativeType=thisType;
      var->setName(name.c_str());
      var->id=j;
      var->setParentCDFObject(cdfObject);
      var->isDimension=isDimension;
      
      
      
      //It is essential that the variable nows which reader can be used to read the data
      var->setCDFReaderPointer((void*)this);
    }
  }
  return 0;
}

CDFType CDFNetCDFReader::_typeConversionAtt(nc_type type){
  if(type==NC_BYTE)return CDF_BYTE;
  if(type==NC_UBYTE)return CDF_UBYTE;
  if(type==NC_CHAR)return CDF_CHAR;
  if(type==NC_SHORT)return CDF_SHORT;
  if(type==NC_USHORT)return CDF_USHORT;
  if(type==NC_INT)return CDF_INT;
  if(type==NC_UINT)return CDF_UINT;
  if(type==NC_INT64)return CDF_INT64;
  if(type==NC_UINT64)return CDF_UINT64;
  if(type==NC_FLOAT)return CDF_FLOAT;
  if(type==NC_DOUBLE)return CDF_DOUBLE;
  if(type==NC_STRING)return CDF_STRING;
  CDBWarning("Warning unknown attribute type %d",type);
  return CDF_UNKNOWN;
}


CDFType CDFNetCDFReader::_typeConversionVar(nc_type type, bool isUnsigned){
  if(isUnsigned){
    if(type==NC_BYTE)  return CDF_UBYTE;
    if(type==NC_UBYTE) return CDF_UBYTE;
    if(type==NC_CHAR)  return CDF_UBYTE;
    if(type==NC_SHORT) return CDF_USHORT;
    if(type==NC_USHORT)return CDF_USHORT;
    if(type==NC_INT)   return CDF_UINT;
    if(type==NC_UINT)  return CDF_UINT;
    if(type==NC_INT64) return CDF_UINT64;
    if(type==NC_UINT64)return CDF_UINT64;
  }else{
    if(type==NC_BYTE)  return CDF_BYTE;
    if(type==NC_UBYTE) return CDF_UBYTE;
    if(type==NC_CHAR)  return CDF_CHAR;
    if(type==NC_SHORT) return CDF_SHORT;
    if(type==NC_USHORT)return CDF_USHORT;
    if(type==NC_INT)   return CDF_INT;
    if(type==NC_UINT)  return CDF_UINT;
    if(type==NC_INT64) return CDF_INT64;
    if(type==NC_UINT64)return CDF_UINT64;
  }
  if(type==NC_FLOAT)return CDF_FLOAT;
  if(type==NC_DOUBLE)return CDF_DOUBLE;
  if(type==NC_STRING)return CDF_STRING;
  
  return CDF_UNKNOWN;
}

int CDFNetCDFReader::open(const char *fileName){
  
  if(cdfObject == NULL){
    CDBError("No CDFObject defined, use CDFObject::attachCDFReader(CDFNetCDFReader*). Please note that this function should be called by CDFObject open routines.");
    return 1;
  }
  this->fileName=fileName;
  
  if(cdfCache!=NULL){
    int cacheStatus = cdfCache->open(fileName,cdfObject,false);
    if(cacheStatus == 0) {
      //CDBDebug("Succesfully opened from cache for file %s",fileName);
      return 0;
    }
  }
  
  // Check type sizes 
  if(sizeof(char)  !=1){CDBError("The size of char is unequeal to 8 Bits");return 1;}
  if(sizeof(short) !=2){CDBError("The size of short is unequeal to 16 Bits");return 1;}
  if(sizeof(int)   !=4){CDBError("The size of int is unequeal to 32 Bits");return 1;}
  if(sizeof(float) !=4){CDBError("The size of float is unequeal to 32 Bits");return 1;}
  if(sizeof(double)!=8){CDBError("The size of double is unequeal to 64 Bits");return 1;}
  
  //Set cache size
  //status = nc_set_chunk_cache(55353600*10,2000,0.75);
  //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_set_chunk_cache: ",status);return 1;}
  //status = nc_set_chunk_cache(0,0,0);
  //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_set_chunk_cache: ",status);return 1;}
  #ifdef CCDFNETCDFIO_DEBUG_OPEN        
  CDBDebug("NC_OPEN opening %s",fileName);
  #endif      
  
  status = nc_open(fileName,NC_NOWRITE,&root_id);
  if(status!=NC_NOERR){ncError(__LINE__,className,"nc_open: ",status);return 1;}
  
  #ifdef MEASURETIME
  StopWatch_Stop("CDFNetCDFReader open file\n");
  #endif
  int nDims,nVars,nRootAttributes,unlimDimIdP;  
  status = nc_inq(root_id,&nDims,&nVars,&nRootAttributes,&unlimDimIdP);
  if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq: ",status);return 1;}
  #ifdef MEASURETIME
  StopWatch_Stop("NC_INQ");
  #endif
  
  //First readdims
  CT::string groupName="";
  status = readVariables(root_id,&groupName,0);if(status!=0)return 1;
  
  //Second read vars
  groupName="";
  status = readVariables(root_id,&groupName,1);if(status!=0)return 1;
  
  
  #ifdef MEASURETIME
  StopWatch_Stop("readVar");
  #endif
  
  status = readAttributes(root_id,cdfObject->attributes,NC_GLOBAL,nRootAttributes);if(status!=0)return 1;
  #ifdef MEASURETIME
  StopWatch_Stop("readAttr");
  #endif
  if(cdfCache!=NULL){
    int cacheStatus = cdfCache->open(fileName,cdfObject,true);
    if(cacheStatus == 0) return 0;
  }
  return 0;
}


int CDFNetCDFReader::close(){
  if(root_id!=-1){
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("CLOSING %s", fileName.c_str());
    #endif        
    nc_close(root_id);
  }
  root_id=-1;
  return 0;
}







/********************************* NetCDFWriter class *****************************************************************/








void CDFNetCDFWriter::ncError(int line, const char *className, const char * msg,int e){
  if(e==NC_NOERR)return;
  char szTemp[1024];
  snprintf(szTemp,1023,"[E: %s, %d in class %s] %s: %s\n",__FILE__,line,className,msg,nc_strerror(e));
  printErrorStream(szTemp);
}


nc_type CDFNetCDFWriter::NCtypeConversion(CDFType  type){
  if(type==CDF_BYTE)return NC_BYTE;
  if(type==CDF_UBYTE)return NC_UBYTE;
  if(type==CDF_CHAR)return NC_CHAR;
  if(type==CDF_SHORT)return NC_SHORT;
  if(type==CDF_USHORT)return NC_USHORT;
  if(type==CDF_INT)return NC_INT;
  if(type==CDF_UINT)return NC_UINT;
  if(type==CDF_INT64)return NC_INT64;
  if(type==CDF_UINT64)return NC_UINT64;
  if(type==CDF_FLOAT)return NC_FLOAT;
  if(type==CDF_DOUBLE)return NC_DOUBLE;
  if(type==CDF_STRING)return NC_STRING;
  return NC_DOUBLE;
}

CT::string CDFNetCDFWriter::NCtypeConversionToString(CDFType  type){
  CT::string r;
  r="NC_DOUBLE";
  if(type==CDF_BYTE)r="NC_BYTE";
  if(type==CDF_UBYTE)r="NC_UBYTE";
  if(type==CDF_CHAR)r="NC_CHAR";
  if(type==CDF_SHORT)r="NC_SHORT";
  if(type==CDF_USHORT)r="NC_USHORT";
  if(type==CDF_INT)r="NC_INT";
  if(type==CDF_UINT)r="NC_UINT";
  if(type==CDF_INT64)r="NC_INT64";
  if(type==CDF_UINT64)r="NC_UINT64";
  if(type==CDF_FLOAT)r="NC_FLOAT";
  if(type==CDF_DOUBLE)r="NC_DOUBLE";
  if(type==CDF_STRING)r="NC_STRING";
  if(type==CDF_UNKNOWN)r="NC_DOUBLE";
  return r;
}

CDFNetCDFWriter::CDFNetCDFWriter(CDFObject *cdfObject){
  this->cdfObject=cdfObject;
  writeData=true;
  readData=true;
  listNCCommands=false;
  netcdfMode=4;
  shuffle=0;
  deflate=1;
  deflate_level=2;
};

CDFNetCDFWriter::~CDFNetCDFWriter(){
  for(size_t j=0;j<dimensions.size();j++){delete dimensions[j];}
};

void CDFNetCDFWriter::setDeflateShuffle(int deflate, int deflate_level,int shuffle){
  this->deflate = deflate;
  this->shuffle = shuffle;
  this->deflate_level = deflate_level;
}

void CDFNetCDFWriter::setNetCDFMode(int mode){
  if(mode!=3&&mode!=4){
    CDBError("Illegal netcdf mode %d: keeping mode ",mode,netcdfMode);
    return;
  }
  netcdfMode=mode;
};

void CDFNetCDFWriter::disableVariableWrite(){
  writeData=false;
};

void CDFNetCDFWriter::disableReadData(){
  readData=false;
};

void CDFNetCDFWriter::recordNCCommands(bool enable){
  listNCCommands=enable;
}

CT::string CDFNetCDFWriter::getNCCommands(){
  return NCCommands;
};

int CDFNetCDFWriter::write(const char *fileName){
  return write(fileName,NULL);
}

int CDFNetCDFWriter::write(const char *fileName,void(*progress)(const char*message,float percentage)){
  NCCommands="";
  if(listNCCommands){
    NCCommands.printconcat("int root_id;\n");
    NCCommands.printconcat("size_t start[NC_MAX_DIMS];\n");
    NCCommands.printconcat("size_t count[NC_MAX_DIMS];\n");
    NCCommands.printconcat("int dimIDArray[NC_MAX_DIMS];\n");
    NCCommands.printconcat("int shuffle=%d;\n",shuffle);
    NCCommands.printconcat("int deflate=%d;\n",deflate);
    NCCommands.printconcat("int deflate_level=%d;\n",deflate_level);
    NCCommands.printconcat("int numDims=%d;\n",0);
    NCCommands.printconcat("void *variable_data=NULL;\n");
    
    for(size_t j=0;j<cdfObject->dimensions.size();j++){
      NCCommands.printconcat("int dim_id_%d;\n",j);
    }
    for(size_t j=0;j<cdfObject->variables.size();j++){
      NCCommands.printconcat("int var_id_%d;\n",j);
    }
  }
  
  this->fileName=fileName;
  #ifdef CCDFNETCDFWRITER_DEBUG                        
  CDBDebug("Writing to file %s",fileName);
  #endif  
  if(netcdfMode>3){
    status = nc_create(fileName ,NC_NETCDF4|NC_CLOBBER , &root_id);
    if(listNCCommands){
      NCCommands.printconcat("nc_create(\"%s\" ,NC_NETCDF4|NC_CLOBBER , &root_id);\n",fileName);
    }
  }else{
    status = nc_create(fileName , NC_CLOBBER|NC_64BIT_OFFSET, &root_id);
    if(listNCCommands){
      NCCommands.printconcat("nc_create(\"%s\" ,NC_CLOBBER|NC_64BIT_OFFSET , &root_id);\n",fileName);
    }
  }
  if(status!=NC_NOERR){
    CDBError("Unable to create %s",fileName);
    ncError(__LINE__,className,"nc_create: ",status);nc_close(root_id);root_id=-1;
    
    return 1;
    
  }
  status = _write(progress);
  #ifdef CCDFNETCDFWRITER_DEBUG                        
  CDBDebug("Finished writing to file %s",fileName);
  #endif  
  
  nc_close(root_id);root_id=-1;
  if(listNCCommands){
    NCCommands.printconcat("nc_close(root_id);\n");
  }
  
  
  
  return status;
};

int CDFNetCDFWriter::_write(void(*progress)(const char*message,float percentage)){
  #ifdef CCDFNETCDFWRITER_DEBUG                        
  CDBDebug("Writing global attributes");
  #endif  
  
  //Write global attributes
  for(size_t i=0;i<cdfObject->attributes.size();i++){
    status = nc_put_att(root_id, NC_GLOBAL, cdfObject->attributes[i]->name.c_str(),NCtypeConversion(cdfObject->attributes[i]->getType()),cdfObject->attributes[i]->length,cdfObject->attributes[i]->data);
    if(listNCCommands){
      
      void *data=cdfObject->attributes[i]->data;
      size_t length=cdfObject->attributes[i]->length;
      CDFType type=cdfObject->attributes[i]->getType();
      if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE){
        CT::string out="";
        out.concat((const char*)data,length);
        NCCommands.printconcat("nc_put_att(root_id, NC_GLOBAL, \"%s\",%s,%d,\"%s\");\n",cdfObject->attributes[i]->name.c_str(),NCtypeConversionToString(type).c_str(),cdfObject->attributes[i]->length,out.c_str());
      }else{
        if(type==CDF_INT||type==CDF_UINT){
          NCCommands.printconcat("int attrData_%d[]={",i);
          for(size_t n=0;n<length;n++){
            NCCommands.printconcat("%d",((int*)data)[n]);
            if(n<length-1){
              NCCommands.printconcat(",");
            }
            NCCommands.printconcat("};\n");
          }
        }

        if(type==CDF_INT64||type==CDF_UINT64){
          NCCommands.printconcat("int64 attrData_%d[]={",i);
          for(size_t n=0;n<length;n++){
            NCCommands.printconcat("%ld",((long*)data)[n]);
            if(n<length-1){
              NCCommands.printconcat(",");
            }
            NCCommands.printconcat("};\n");
          }
        }
        
        if(type==CDF_SHORT||type==CDF_USHORT){
          NCCommands.printconcat("short attrData_%d[]={",i);
          for(size_t n=0;n<length;n++){
            NCCommands.printconcat("%d",((short*)data)[n]);
            if(n<length-1){
              NCCommands.printconcat(",");
            }
            NCCommands.printconcat("};\n");
          }
        }
        
        
        if(type==CDF_FLOAT){
          NCCommands.printconcat("float attrData_%d[]={",i);
          for(size_t n=0;n<length;n++){
            NCCommands.printconcat("%f",((float*)data)[n]);
            if(n<length-1){
              NCCommands.printconcat(",");
            }
            NCCommands.printconcat("};\n");
          }
        }
        
        if(type==CDF_DOUBLE){
          NCCommands.printconcat("float attrData_%d[]={",i);
          for(size_t n=0;n<length;n++){
            NCCommands.printconcat("%f",((double*)data)[n]);
            if(n<length-1){
              NCCommands.printconcat(",");
            }
            NCCommands.printconcat("};\n");
          }
        }
        NCCommands.printconcat("nc_put_att(root_id, NC_GLOBAL, \"%s\",%s,%d,attrData_%d_%d);\n",cdfObject->attributes[i]->name.c_str(),NCtypeConversionToString(type).c_str(),cdfObject->attributes[i]->length,i);
      }
      
      
    }
    
    if(status!=NC_NOERR){
      char name[1023];
      CDF::getCDFDataTypeName(name,1000,cdfObject->attributes[i]->getType());
      CDBError("For attribute NC_GLOBAL::%s of type %s:",cdfObject->attributes[i]->name.c_str(),name);
      ncError(__LINE__,className,"nc_put_att: ",status);return 1;
    }
  }
  #ifdef CCDFNETCDFWRITER_DEBUG                        
  CDBDebug("Define dimensions");
  #endif  
  
  //Define dimensions
  for(size_t j=0;j<cdfObject->dimensions.size();j++){
    CDF::Dimension * dim = new CDF::Dimension();
    dim->setName(cdfObject->dimensions[j]->name.c_str());
    dim->length=cdfObject->dimensions[j]->length;
    
    status = nc_def_dim(root_id,dim->name.c_str() , dim->length, &dim->id);
    #ifdef CCDFNETCDFWRITER_DEBUG                        
    CDBDebug("DEF DIM %s %d %d",dim->name.c_str(),dim->length,dim->id);
    #endif
    if(listNCCommands){
      NCCommands.printconcat("nc_def_dim(root_id,\"%s\" , %d, &dim_id_%d);\n",dim->name.c_str(),dim->length,j);
    } 
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_def_dim: ",status);delete dim;return 1;}
    dimensions.push_back(dim);
  }
  
  
  int nrVarsWritten=0;
  //Write the variables
  //First write the variables connected to dimensions and later the variables itself
  //writeDimsFirst==0: dimension variables
  //writeDimsFirst==1: variables
  for(int writeDimsFirst=0;writeDimsFirst<2;writeDimsFirst++){
    #ifdef CCDFNETCDFWRITER_DEBUG                        
    if(writeDimsFirst==0)CDBDebug("Write dimensions");
    if(writeDimsFirst==1)CDBDebug("Write variables");
    #endif  
    
    
    //Write all different variables.
    //nc_close(root_id);
    //status = nc_set_chunk_cache(0,0,0);
    //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_set_chunk_cache: ",status);return 1;}
    
    //status = nc_open(fileName ,netcdfWriteMode|NC_WRITE, &root_id);
    //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_create: ",status);nc_close(root_id);return 1;}
    for(size_t j=0;j<cdfObject->variables.size();j++){
      //for(size_t j=0;j<4;j++){          
      //Get the variable names with these dimensions 
      CDF::Variable *variable = cdfObject->variables[j];
      const char *name = variable->name.c_str();
      #ifdef CCDFNETCDFWRITER_DEBUG                        
      if(writeDimsFirst==0)CDBDebug("Writing %s",name);
      #endif  
      
      int numDims=variable->dimensionlinks.size();
      if((variable->isDimension==true &&writeDimsFirst==0)||
        (variable->isDimension==false&&writeDimsFirst==1)){
        {
          int dimIDS[numDims+1];
          int NCCommandID[numDims+1];
          size_t totalVariableSize = 0;
          //Find dim and chunk info
          CT::string variableInfo(name);
          variableInfo.concat("\t(");
          for(int i=0;i<numDims;i++){
            for(size_t k=0;k<dimensions.size();k++){
              if(dimensions[k]->name.equals(&variable->dimensionlinks[i]->name)){
                dimIDS[i]=dimensions[k]->id;
                NCCommandID[i]=k;
                if(totalVariableSize==0)totalVariableSize=1;
                //CDBDebug("EQUALS: %s %d",dimensions[k]->name.c_str(),dimIDS[i]);  
                totalVariableSize*=dimensions[k]->length;
                //chunkSizes[i]=dimensions[k]->length;
                variableInfo.printconcat("%s=%d",dimensions[k]->name.c_str(),dimensions[k]->length);
                if(i+1<numDims)variableInfo.concat(",");
              }
            }
          }
          variableInfo.concat(")");
          int nc_var_id;
          status = nc_redef(root_id);
          if(listNCCommands){
            NCCommands.printconcat("nc_redef(root_id);\n");
          } 
          //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_redef: ",status);return 1;}
          /*for(int j=0;j<numDims;j++){
           *            printf("%d\n",dimIDS[j]);
        }*/
          status = nc_def_var(root_id, name    ,NCtypeConversion(variable->currentType),numDims, dimIDS,&nc_var_id);
          if(status!=NC_NOERR){CDBError("Unable to define variable %s",name);ncError(__LINE__,className,"nc_def_var: ",status);return 1;}
          if(listNCCommands){
            
            NCCommands.printconcat("numDims=%d;\n",numDims);
            for(int k=0;k<numDims;k++){
              NCCommands.printconcat("dimIDArray[%d]=dim_id_%d;\n",k,NCCommandID[k]);
            }
            NCCommands.printconcat("nc_def_var(root_id, \"%s\",%s,numDims, dimIDArray,&var_id_%d);\n",name,NCtypeConversionToString(variable->currentType).c_str(),j);
            
          } 
          
          //Set chunking and deflate options
          
          
          if(netcdfMode>=4&&numDims>0&&1==1){
            
            size_t chunkSizes[variable->dimensionlinks.size()];
            if(variable->dimensionlinks.size()>2){
              
              
              for(size_t m=0;m<variable->dimensionlinks.size();m++){
                chunkSizes[m] = variable->dimensionlinks[m]->getSize();
                try{
                  CT::string standardName = cdfObject->getVariable(variable->dimensionlinks[m]->name.c_str())->getAttribute("standard_name")->getDataAsString();
                  if(standardName.equals("time")){
                    chunkSizes[m] = 1;
                  }
                }catch(int e){
                }
              }
              
              //                   for(size_t m=0;m<variable->dimensionlinks.size();m++){
              //                     CDBDebug("ChunkSizes %s = %d",variable->dimensionlinks[m]->name.c_str(),chunkSizes[m]);
              //                   }
              
              
              status = nc_def_var_chunking(root_id,nc_var_id,0 ,chunkSizes);
              if(status!=NC_NOERR){ncError(__LINE__,className,"nc_def_var_chunking: ",status);return 1;}
              
              
            }
          }
          
          //CDBDebug("shuffle ,deflate, deflate_level %d,%d,%d",shuffle ,deflate, deflate_level);
          status = nc_def_var_deflate(root_id,nc_var_id,shuffle ,deflate, deflate_level);
          if(status!=NC_NOERR){ncError(__LINE__,className,"nc_def_var_deflate: ",status);return 1;}
          if(listNCCommands){
            NCCommands.printconcat("nc_def_var_deflate(root_id,var_id_%d,shuffle ,deflate, deflate_level);\n",j);
          } 
          
          //copy data
          #ifdef CCDFNETCDFWRITER_DEBUG     
          CT::string message;
          message.print("%d/%d Copying data for variable %s: total %d bytes",
                        nrVarsWritten+1,cdfObject->variables.size(),variableInfo.c_str(),int(totalVariableSize)*CDF::getTypeSize(variable->getType()));
          CDBDebug("%s",message.c_str());
          #endif
          //Copy attributes for this specific variable
          for(size_t i=0;i<variable->attributes.size();i++){
            if(!variable->attributes[i]->name.equals("CLASS")&&!variable->attributes[i]->name.equals("_Netcdf4Dimid")){
              nc_type type=NCtypeConversion(variable->attributes[i]->getType());
              if(variable->attributes[i]->name.equals("_FillValue")){
                type=variable->getType();
              }
              status = nc_put_att(root_id, nc_var_id, variable->attributes[i]->name.c_str(),
                                  type,variable->attributes[i]->length,
                                  variable->attributes[i]->data);
              if(listNCCommands){
                
                void *data=variable->attributes[i]->data;
                size_t length=variable->attributes[i]->length;
                if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE){
                  CT::string out="";
                  out.concat((const char*)data,length);
                  NCCommands.printconcat("nc_put_att(root_id, var_id_%d, \"%s\",%s,%d,\"%s\");\n",j,variable->attributes[i]->name.c_str(),NCtypeConversionToString(type).c_str(),variable->attributes[i]->length,out.c_str());
                }else{
                  if(type==CDF_INT||type==CDF_UINT){
                    NCCommands.printconcat("int attrData_%d_%d[]={",j,i);
                    for(size_t n=0;n<length;n++){
                      NCCommands.printconcat("%d",((int*)data)[n]);
                      if(n<length-1){
                        NCCommands.printconcat(",");
                      }
                      NCCommands.printconcat("};\n");
                    }
                  }
                  
                  if(type==CDF_INT64||type==CDF_UINT64){
                    NCCommands.printconcat("int attrData_%d_%d[]={",j,i);
                    for(size_t n=0;n<length;n++){
                      NCCommands.printconcat("%ld",((long*)data)[n]);
                      if(n<length-1){
                        NCCommands.printconcat(",");
                      }
                      NCCommands.printconcat("};\n");
                    }
                  }
                  
                  if(type==CDF_SHORT||type==CDF_USHORT){
                    NCCommands.printconcat("short attrData_%d_%d[]={",j,i);
                    for(size_t n=0;n<length;n++){
                      NCCommands.printconcat("%d",((short*)data)[n]);
                      if(n<length-1){
                        NCCommands.printconcat(",");
                      }
                      NCCommands.printconcat("};\n");
                    }
                  }
                  
                  
                  if(type==CDF_FLOAT){
                    NCCommands.printconcat("float attrData_%d_%d[]={",j,i);
                    for(size_t n=0;n<length;n++){
                      NCCommands.printconcat("%f",((float*)data)[n]);
                      if(n<length-1){
                        NCCommands.printconcat(",");
                      }
                      NCCommands.printconcat("};\n");
                    }
                  }
                  
                  if(type==CDF_DOUBLE){
                    NCCommands.printconcat("double attrData_%d_%d[]={",j,i);
                    for(size_t n=0;n<length;n++){
                      NCCommands.printconcat("%f",((double*)data)[n]);
                      if(n<length-1){
                        NCCommands.printconcat(",");
                      }
                      NCCommands.printconcat("};\n");
                    }
                  }
                  NCCommands.printconcat("nc_put_att(root_id, var_id_%d, \"%s\",%s,%d,attrData_%d_%d);\n",j,variable->attributes[i]->name.c_str(),NCtypeConversionToString(type).c_str(),variable->attributes[i]->length,j,i);
                }
                
              }
              
              if(status!=NC_NOERR){
                char attrType[256],varType[256];
                CDF::getCDFDataTypeName(attrType,255,variable->attributes[i]->getType());
                CDF::getCDFDataTypeName(varType,255,variable->currentType);
                CDBError("Trying to write attribute %s with type %s for variable %s with type %s\nnc_put_att: %s",
                         variable->attributes[i]->name.c_str(),attrType,variable->name.c_str(),varType,nc_strerror(status));return 1;
              }
            }else {
              //CDBDebug("Skipping attribute %s:%s",variable->name.c_str(),variable->attributes[i]->name.c_str());
            }
          }
          if((numDims>0&&writeData==true)){//||(variable->isDimension&&numDims==1)){
            bool needsDimIteration=false;
            int iterativeDimIndex=variable->getIterativeDimIndex();
            if(iterativeDimIndex!=-1)needsDimIteration=true;
            if(variable->isDimension)needsDimIteration=false;
            size_t start[NC_MAX_DIMS],count[NC_MAX_DIMS];
            for(size_t j=0;j<variable->dimensionlinks.size();j++){
              start[j]=0;count[j]=variable->dimensionlinks[j]->getSize();
            }
            status = nc_enddef(root_id);
            if(status!=NC_NOERR){
              CDBError("For variable %s:",variable->name.c_str());
              ncError(__LINE__,className,"nc_enddef: ",status);return 1;
            }
            if(listNCCommands){
              NCCommands.printconcat("nc_enddef(root_id);\n");
            } 
            
            #ifdef CCDFNETCDFWRITER_DEBUG                
            CDBDebug("--- Copying Variable %s. needsDimIteration = %d---",variable->name.c_str(),needsDimIteration);
            #endif                 
            if(needsDimIteration==false){
              int status = copyVar(variable,nc_var_id,start,count);
              if(status!=0)return status;
            }else{
              
              for(size_t id=0;id<variable->dimensionlinks[iterativeDimIndex]->getSize();id++){
                
                CT::string progressMessage;
                progressMessage.print("\"%d/%d iterating dim %s with index %d/%d for variable %s\"",
                                      nrVarsWritten+1,
                                      cdfObject->variables.size(),
                                      variable->dimensionlinks[iterativeDimIndex]->name.c_str(),
                                      id,
                                      variable->dimensionlinks[iterativeDimIndex]->getSize(),
                                      variable->name.c_str());
                
                float varPercentage= float(nrVarsWritten)/float(cdfObject->variables.size());
                float dimPercentage = (float(id)/float(variable->dimensionlinks[iterativeDimIndex]->getSize()))/float(cdfObject->variables.size());
                float percentage = (varPercentage + dimPercentage)*100;
                
                #ifdef CCDFNETCDFWRITER_DEBUG    
                CDBDebug(progressMessage.c_str());
                #endif                 
                (*progress)(progressMessage.c_str(),percentage);
                start[iterativeDimIndex]=id;count[iterativeDimIndex]=1;
                
                int status = copyVar(variable,nc_var_id,start,count);
                
                if(status!=0)return status;
              }
            }
            nc_sync(root_id);
            if(listNCCommands){
              NCCommands.printconcat("nc_sync(root_id);\n");
            } 
            //CDBDebug("DONE!");
          }
          
          nrVarsWritten++;
        }
        }
    }
  }
  return 0;
};

int CDFNetCDFWriter::copyVar(CDF::Variable *variable,int nc_var_id,size_t *start, size_t *count){
  ptrdiff_t stride[NC_MAX_DIMS];
  for(size_t j=0;j<variable->dimensionlinks.size();j++)stride[j]=1;
  
  if(readData==true){
    /*
     *      CDFReader *cdfReader = (CDFReader *)variable->getCDFReaderPointer();
     *    if(cdfReader!=NULL){
     *      status = cdfReader->readVariableData(variable, variable->getType());
     *      if(status!=0){
     *        CDBError("Reading of variable %s failed",variable->name.c_str());
     *        return 1;
  }
  }*/
    //TODO should read iterative for iterative dims.
    
    //         for(size_t j=0;j<variable->dimensionlinks.size();j++){
    //           CDBDebug("%s %d %d",variable->dimensionlinks[j]->name.c_str(),start[j],count[j]);
    //         }
    status = variable->readData(variable->currentType,start,count,stride);
    if(status!=0){
      CDBError("Reading of variable %s failed",variable->name.c_str());
      return 1;
    }
    if(variable->data==NULL){
      CDBError("variable->data == NULL for variable %s",variable->name.c_str());
      return 1;
    }else{
      //CDBWarning("No cdfReaderPointer defined for variable %s",variable->name.c_str());
    }
    #ifdef CCDFNETCDFWRITER_DEBUG                        
    CDBDebug("Variable %s read",variable->name.c_str());
    #endif        
    
    //Apply longitude warping of the data
    //EG 0-360 to -180 till -180
    //warper.warpLonData(variable);
  }
  if(status==0){
    if(variable->data==NULL){
      CDBError("variable->data==NULL for %s",variable->name.c_str());
      return 1;
    }
    //CDBDebug("Writing %d elements",variable->getSize());
    #ifdef CCDFNETCDFWRITER_DEBUG                        
    for(size_t i=0;i<variable->dimensionlinks.size();i++){
      CDBDebug("Writing %s,%d: %d %d\t\t[%d]",variable->name.c_str(),i,start[i],count[i],variable->getSize());
    }
    #endif        
    
    
    status = nc_put_vara(root_id,nc_var_id,start,count,variable->data);
    if(listNCCommands){
      NCCommands.printconcat("//");
      for(size_t j=0;j<variable->dimensionlinks.size();j++){
        NCCommands.printconcat("%s\t",variable->dimensionlinks[j]->name.c_str());
      }
      NCCommands.printconcat("\n");
      for(size_t j=0;j<variable->dimensionlinks.size();j++){
        NCCommands.printconcat("start[%d]=%d;\t",j,start[j]);
      }
      NCCommands.printconcat("\n");
      for(size_t j=0;j<variable->dimensionlinks.size();j++){
        NCCommands.printconcat("count[%d]=%d;\t",j,count[j]);
      }
      NCCommands.printconcat("\n");
      NCCommands.printconcat("//variable_data should be defined here\n");
      NCCommands.printconcat("//nc_put_vara(root_id,var_id_%d,start,count,variable_data);\n",nc_var_id);
    } 
    //printf("Fake put vara\n");
    if(status!=NC_NOERR){
      CDBError("For variable %s:",variable->name.c_str());
      ncError(__LINE__,className,"nc_put_var: ",status);return 1;}
  }
  //Free the variable data
  //if(variable->isDimension==false&&
  if(readData==true){
    #ifdef CCDFNETCDFWRITER_DEBUG                        
    CDBDebug("Free variable %s",variable->name.c_str());
    #endif        
    if(!variable->isDimension)variable->freeData();
  }
  return 0;
};

