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

#include "CCDFVariable.h"
#include "CCDFObject.h"
#include "CCDFReader.h"
const char *CDF::Variable::className="Variable";



int CDF::Variable::readData(CDFType type){
  return readData(type,NULL,NULL,NULL);
}

int CDF::Variable::readData(bool applyScaleOffset){
  return readData(-1,applyScaleOffset);
}

int CDF::Variable::readData(CDFType readType,bool applyScaleOffset){
  return readData(readType,NULL,NULL,NULL,applyScaleOffset);
}



/**
 * Reads data
 * @param readType The datatype to read. When -1 is given, this is determined automatically
 * @param applyScaleOffset Whether or not to apply scale and offset
 */
int CDF::Variable::readData(CDFType readType,size_t *_start,size_t *_count,ptrdiff_t *_stride,bool applyScaleOffset){
  
 if(data!=NULL&&currentType!=readType){freeData();}
 if(data!=NULL){
#ifdef CCDFDATAMODEL_DEBUG            
     CDBDebug("Data is already defined");
#endif     
    return 0;
  }
  
  if(applyScaleOffset==false){
    return readData(currentType,_start,_count,_stride);
  }
  
  double scaleFactor=1,addOffset=0,fillValue = 0;
  bool hasFillValue = false;
  int scaleType=currentType;
  try{
    
    CDF::Attribute * a = getAttribute("scale_factor");
    a->getData(&scaleFactor,1);
    scaleType=a->type;
  }catch(int e){}

  try{
    getAttribute("add_offset")->getData(&addOffset,1);
  }catch(int e){}
  try{
    getAttribute("_FillValue")->getData(&fillValue,1);
    hasFillValue = true;
  }catch(int e){}
  
  if(readType!=-1)scaleType=readType;
  int status = readData(scaleType,_start,_count,_stride);
  if(status != 0)return status;
  
  //Apply scale and offset
  if(scaleFactor!=1||addOffset!=0){
  size_t lsize= getSize();
    if(scaleType == CDF_FLOAT){
      float *scaleData = (float*)data;
      float fscale = float(scaleFactor);
      float foffset = float(addOffset);
      if(scaleFactor!=1||addOffset!=0){
        for(size_t j=0;j<lsize;j++)scaleData[j]=scaleData[j]*fscale+foffset;
        fillValue=fillValue*fscale+foffset;
        float f=(float)fillValue;
        if( hasFillValue)getAttribute("_FillValue")->setData(CDF_FLOAT,&f,1);
      }
    }
    
    if(scaleType == CDF_DOUBLE){
      float *scaleData = (float*)data;
      for(size_t j=0;j<lsize;j++)scaleData[j]=scaleData[j]*scaleFactor+addOffset;
      fillValue=fillValue*scaleFactor+addOffset;
      if( hasFillValue)getAttribute("_FillValue")->setData(CDF_DOUBLE,&fillValue,1);
    }
    //removeAttribute("scale_factor");
    //removeAttribute("add_offset");
  }
  return 0;  
}

int CDF::Variable::readData(CDFType type,size_t *_start,size_t *_count,ptrdiff_t *_stride){

#ifdef CCDFDATAMODEL_DEBUG          
  CDBDebug("reading variable %s",name.c_str());
#endif  
 
 if(data!=NULL&&type!=this->currentType){
   #ifdef CCDFDATAMODEL_DEBUG          
  CDBDebug("Freeing orignal variable %s",name.c_str());
#endif  
   freeData();
  }
  
  
  
  
  //TODO needs to cope correctly with cdfReader.
  if(data!=NULL){
#ifdef CCDFDATAMODEL_DEBUG            
     CDBDebug("Data is already defined");
#endif     
    return 0;
  }
   
  
  //Check for iterative dimension
  bool needsDimIteration=false;
  int iterativeDimIndex=getIterativeDimIndex();
  if(iterativeDimIndex!=-1&&isDimension==false){
    needsDimIteration=true;
  }
  
  if(needsDimIteration==true){
    //CDF::Dimension * iterativeDim;
    bool useStartCountStride=false;if(_start!=NULL&&_count!=NULL){useStartCountStride=true;}
    //iterativeDim=dimensionlinks[iterativeDimIndex];
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
        
         if(!hasCustomReader){
          //TODO NEEDS BETTER CHECKS
          if(cdfReaderPointer==NULL){
            CDBError("No CDFReader defined for variable %s",name.c_str());
             delete[] start;delete[] count;delete[] stride;
            return 1;
          }
          Variable *tVar=tCDFObject->getVariable(name.c_str());
          if(tVar->readData(type,start,count,stride)!=0)throw(__LINE__);
          //Put the read data chunk in our destination variable
  #ifdef CCDFDATAMODEL_DEBUG                
          CDBDebug("Copying %d elements to variable %s",tVar->getSize(),name.c_str());
  #endif        
          DataCopier::copy(data,type,tVar->data,type,dataReadOffset,0,tVar->getSize());dataReadOffset+=tVar->getSize();
          //Free the read data
  #ifdef CCDFDATAMODEL_DEBUG                
          CDBDebug("Free tVar %s",tVar->name.c_str());
  #endif        
          tVar->freeData();
        }
        
        
        
        if(hasCustomReader){
          status = customReader->readData(this,data,_start,count,stride);
        }
        
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
    if(status!=0)return 1;
  }
  
  if(needsDimIteration==false){
   
      //TODO NEEDS BETTER CHECKS
    if(cdfReaderPointer==NULL){
      CDBError("No CDFReader defined for variable %s",name.c_str());
      return 1;
    }
     
    CDFReader *cdfReader = (CDFReader *)cdfReaderPointer;
   
    //CDBDebug("OK");
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
     //CDBDebug("OK");
    if(useStartCountStride==true){
       //CDBDebug("OK");
      status = cdfReader->_readVariableData(this, type,_start,_count,_stride);
       //CDBDebug("OK");
    }else{
       //CDBDebug("OK");
      status = cdfReader->_readVariableData(this, type);
    }
     //CDBDebug("OK");
    if(status!=0){
      CDBError("Unable to read data for variable %s",name.c_str());
      return 1;
    }
  }
//  CDBDebug("Data for %s read %d",name.c_str(),data!=NULL);
  
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
    srcDimVar->readData(currentType);
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
    if(iterativeVar->readData(currentType)!=0){
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
    status = allocateData(currentType,&dstData,currentDimSize+1);
    if(status!=0){
      CDBError("Unable to allocate data");
      throw("__LINE__");
    }
    status = allocateData(currentType,&srcData,1);
    if(status!=0){
      CDBError("Unable to allocate data");  
      throw("__LINE__");
    }
    
     DataCopier::copy(srcData,currentType,srcDimVar->data,currentType,0,0,1);
    //srcDimVar->getData(srcData,1);
#ifdef CCDFDATAMODEL_DEBUG             
    CDBDebug("Read data for %s = %f",srcDimVar->name.c_str(),((double*)srcData)[0]);
#endif    
  // for(int j=0;j<iterativeDim->getSize();j++){
    //  CDBDebug("1 %d------------------ %f",((double*)iterativeVar->data)[j]);
    //}

    DataCopier::copy(dstData,currentType,iterativeVar->data,currentType,0,0,currentDimSize);
    DataCopier::copy(dstData,currentType,srcData,currentType,currentDimSize,0,1);
    
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
      DataCopier::copy(srcData,srcVar->data,srcVar->type,srcSize);
      DataCopier::copy(dstData,dstVar->data,dstVar->type,dstSize);
      
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
