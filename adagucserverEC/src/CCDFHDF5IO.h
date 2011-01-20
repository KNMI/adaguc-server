#ifndef CCDFHDF5IO_H
#define CCDFHDF5IO_H
#include "CCDFDataModel.h"
#include <stdio.h>
#include <vector>
#include <iostream>
#include <hdf5.h>
#include "CDebugger.h"
#include "CADAGUC_time.h"
void ncError(int line, const char *className, const char * msg,int e);
class CDFHDF5Reader :public CDFReader{
  private:
    CDFType typeConversion(hid_t type){
      if(H5Tequal(type,H5T_NATIVE_SCHAR)>0)return CDF_BYTE;
      if(H5Tequal(type,H5T_NATIVE_UCHAR)>0)return CDF_UBYTE;
      if(H5Tequal(type,H5T_NATIVE_CHAR)>0)return CDF_CHAR;
      if(H5Tequal(type,H5T_NATIVE_SHORT)>0)return CDF_SHORT;
      if(H5Tequal(type,H5T_NATIVE_USHORT)>0)return CDF_USHORT;
      if(H5Tequal(type,H5T_NATIVE_INT)>0)return CDF_INT;
      if(H5Tequal(type,H5T_NATIVE_UINT)>0)return CDF_UINT;
      if(H5Tequal(type,H5T_NATIVE_FLOAT)>0)return CDF_FLOAT;
      if(H5Tequal(type,H5T_NATIVE_DOUBLE)>0)return CDF_DOUBLE;
      return CDF_NONE;
    }
    DEF_ERRORFUNCTION();
  
    int readDimensions(){
      return 0;
    }
    int readAttributes(std::vector<CDF::Attribute *> &attributes,int varID,int natt){
      return 0;
    }
    int readVariables(){
      return 0;
    }
    hid_t H5F_file;
    herr_t status;
    std::vector <hid_t> opengroups;
  //HDF5 error handling
    herr_t (*old_func)(hid_t,void*); 
  
    void *old_client_data;
    hid_t error_stack;
    bool b_EnableKNMIHDF5toCFConversion;
  public:
    CDFHDF5Reader(CDFObject *cdfObject):CDFReader(cdfObject){
      H5F_file=-1;
      //Get error strack
      error_stack = H5Eget_current_stack();
      /* Save old error handler */
      //H5Eget_auto2(error_stack, &old_func, &old_client_data);
      /* Turn off error handling */
      H5Eset_auto2(error_stack, NULL, NULL);
      H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
      b_EnableKNMIHDF5toCFConversion=false;
    }
    ~CDFHDF5Reader(){
      close();
      /* Restore previous error handler */
      //H5Eset_auto2(error_stack, old_func, old_client_data);
    }
    
    int readAttributes(std::vector<CDF::Attribute *> &attributes,hid_t HDF5_group){
      hid_t HDF5_attr_class,HDF5_attribute;
      int dNumAttributes=H5Aget_num_attrs(HDF5_group);
      for(int j=0;j<dNumAttributes;j++){
        HDF5_attribute = H5Aopen_idx(HDF5_group,j);
        size_t attNameSize=H5Aget_name(HDF5_attribute,0,NULL);attNameSize++;
        char attName[attNameSize+1];
        H5Aget_name(HDF5_attribute,attNameSize,attName);
        hid_t HDF5_attr_type = H5Aget_type(HDF5_attribute);
        HDF5_attr_class= H5Tget_class(HDF5_attr_type);
  // Read H5T_INTEGER Attribute
        if (HDF5_attr_class == H5T_INTEGER&&1==1) {
          hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);
          hsize_t stSize = H5Aget_storage_size(HDF5_attribute)/sizeof(int);
          CDF::Attribute *attr = new CDF::Attribute();
          attr->setName(attName);
          
          attr->type=typeConversion(HDF5_attr_memtype);
          attr->length=stSize;
          if(attr->length==0)attr->length=1;//TODO
          //printf("%s %d\n",attName,stSize);
          CDF::allocateData(attr->type,&attr->data,attr->length+1);
          
          status   = H5Aread(HDF5_attribute, HDF5_attr_memtype, attr->data);
          attributes.push_back(attr);
          status   = H5Tclose(HDF5_attr_memtype);
        }
        
        if (HDF5_attr_class == H5T_FLOAT&&1==1) {
          hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);
          hsize_t stSize = H5Aget_storage_size(HDF5_attribute)/sizeof(float);
          CDF::Attribute *attr = new CDF::Attribute();
          attr->setName(attName);
          attr->type=CDF_FLOAT;
          attr->length=stSize;
          CDF::allocateData(attr->type,&attr->data,attr->length+1);
          
          status   = H5Aread(HDF5_attribute, H5T_NATIVE_FLOAT, attr->data);
          attributes.push_back(attr);
          status   = H5Tclose(HDF5_attr_memtype);
        }
        if (HDF5_attr_class == H5T_STRING) {
          hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);
          hsize_t stSize = H5Aget_storage_size(HDF5_attribute)*sizeof(char);
          CDF::Attribute *attr = new CDF::Attribute();
          attr->setName(attName);
          attr->type=CDF_CHAR;
          attr->length=stSize;
          CDF::allocateData(attr->type,&attr->data,attr->length+1);
          status   = H5Aread(HDF5_attribute, HDF5_attr_type, attr->data);
          ((char*)attr->data)[attr->length]='\0';
          //CDBDebug("attr %s = %d = %s",attName,attr->length,attr->data);
          
          attributes.push_back(attr);
          status   = H5Tclose(HDF5_attr_memtype);
        }

        H5Tclose(HDF5_attr_type);
        H5Aclose(HDF5_attribute);
      }
      return 0;
    }
    CDF::Dimension *makeDimension(const char *name,size_t len){
      CDF::Dimension * dim = NULL;
      for(size_t j=0;j<cdfObject->dimensions.size();j++){
        if(cdfObject->dimensions[j]->length==len){
          //dim_1 == dim_1
          if(name[4]==cdfObject->dimensions[j]->name.c_str()[4]){
            dim=cdfObject->dimensions[j];return dim;
          }
        }
      }
      char dimName[256];
      snprintf(dimName,255,"%s_%d",name,int(cdfObject->dimensions.size()));
      dim = new CDF::Dimension();
      dim->length=len;
      CDF::Variable *var= new CDF::Variable();
      var->cdfReaderPointer=this;
      var->setName(dimName);
      var->id=cdfObject->variables.size();
      var->isDimension=true;
      var->dimensionlinks.push_back(dim);
      var->type=CDF_DOUBLE;
      CDF::allocateData(var->type,&var->data,dim->length);
      for(size_t i=0;i<dim->length;i++)((float*)var->data)[i]=0.0f;
      var->setSize(dim->length);
      cdfObject->variables.push_back(var);
      cdfObject->dimensions.push_back(dim);
      dim->setName(dimName);
      
      return dim;
    }
    
    void list(hid_t groupID,char *groupName){
      H5G_info_t group_info ;
      H5Gget_info( groupID, &group_info  );
      //printf("%d\n",(int)group_info.nlinks);
      char name[256];
      
      for(int j=0;j<(int)group_info.nlinks;j++){
        H5Lget_name_by_idx(groupID,".",H5_INDEX_NAME, H5_ITER_INC, j, name, 255, H5P_DEFAULT  );
        hid_t objectID=H5Oopen(groupID, name,H5P_DEFAULT );
        H5I_type_t type = H5Iget_type(objectID);
        
        if(type==H5I_GROUP){
          
          hid_t newGroupID = H5Gopen2( groupID, name, H5P_DEFAULT );
          //CDBDebug("Opened group %s with id %d from %d",name,newGroupID,groupID);
          
          if(newGroupID>0){
            char temp[1024];
            if(strlen(groupName)!=0){
              snprintf(temp,1023,"%s.%s",groupName,name);
            }else snprintf(temp,1023,"%s",name);

          
            CDF::Variable * var = new CDF::Variable();
            var->type=CDF_CHAR;
            var->isDimension=false;
            var->setName(temp);
            var->id= cdfObject->variables.size();
            var->cdfReaderPointer=this;//TODO
          //Attributes:
            readAttributes(var->attributes,newGroupID);
            cdfObject->variables.push_back(var);
            list(newGroupID,temp);
            //CDBDebug("Closing %d",newGroupID);
            H5Gclose(newGroupID);
          }
          //readAttributes(cdfObject->attributes,newGroupID);
        }
        
        if(type==H5I_DATASET){
          //CDBDebug("%d,%s",groupID,name);
          hid_t datasetID = H5Dopen2(groupID, name,H5P_DEFAULT );
          //CDBDebug("Opened dataset %s with id %d from %d",name,datasetID,groupID);
          if(datasetID>0){
            hid_t datasetType=H5Dget_type(datasetID);
            if(datasetType>0){
              hid_t datasetNativeType =  H5Tget_native_type(datasetType,H5T_DIR_ASCEND );
              if(datasetNativeType>0){
                char varName[1024];
                //snprintf(varName,1023,"%s.%s",groupName,name);
                if(strlen(groupName)!=0){
                  snprintf(varName,1023,"%s.%s",groupName,name);
                }else snprintf(varName,1023,"%s",name);

                int cdfType = typeConversion(datasetNativeType);
                //char temp[20];
                //CDF::getDataTypeName(temp,19,cdfType);
                hid_t   HDF5_dataspace = H5Dget_space(datasetID);    /* dataspace handle */
                int     ndims          = H5Sget_simple_extent_ndims(HDF5_dataspace);
                hsize_t dims_out[2];
                H5Sget_simple_extent_dims(HDF5_dataspace, dims_out, NULL);
                CDF::Variable * var=cdfObject->getVariable(varName);
                if(var==NULL){
                  var= new CDF::Variable();
                  cdfObject->variables.push_back(var);
                }
                var->type=cdfType;
                var->isDimension=false;
                var->setName(varName);
                //printf("Adding %s\n",varName);
                var->id= cdfObject->variables.size();
                var->cdfReaderPointer=this;
                readAttributes(var->attributes,datasetID);
                //printf("%s.%s\t %s ",groupName,name,temp);
                CDF::Dimension *dim ;
                for(int d=0;d<ndims;d++){
                 // printf("%d\t",(size_t)dims_out[d]);
                  //Make fake dimensions
                  char dimname[10];snprintf(dimname,9,"dim_%d",d);
                  if(ndims==2){
                    if(d==0)dimname[4]='y';
                    if(d==1)dimname[4]='x';
                  }
                  dim = makeDimension(dimname,dims_out[d]);
                  var->dimensionlinks.push_back(dim);
                }
                //printf("\n");
                
                H5Sclose(HDF5_dataspace);
                H5Tclose(datasetNativeType);
              }
              H5Tclose(datasetType);
            }
            

            H5Dclose(datasetID);
          }
        }
        H5Oclose(objectID);
      }
    }
    
    
    /*******************************/
    /*  Time function        */
    /*******************************/
    int HDF5ToADAGUCTime(char * pszADAGUCTime,const char *pszRadarTime){
      int M;
      char szMonth[4];
  // All month abbreviations
      const char *pszMonths[]={"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
  //Copy from input time the month number
      strncpy(szMonth,pszRadarTime+3,4);szMonth[3]='\0';
  // Uppercase all
      for(unsigned int j=0;j<3;j++)if(szMonth[j]>='a'&&szMonth[j]<='z')szMonth[j]-=32;
  // Try to find the month
      for(M=0;M<12;M++)if(strncmp(szMonth,pszMonths[M],3)==0)break;
      M++;// Months are from 1-12 not 0-11
  // The month string was not found ...
      if(M==13){
        CDBError("Invalid month: %s",szMonth);
        return 1; 
      }
      snprintf(szMonth,3,"%02d\n",M);
      strncpy(pszADAGUCTime,pszRadarTime+7,4);
      strncpy(pszADAGUCTime+4,szMonth,2);
      strncpy(pszADAGUCTime+6,pszRadarTime,2);
      strncpy(pszADAGUCTime+8,"T",1);
      strncpy(pszADAGUCTime+9,pszRadarTime+12,2);
      strncpy(pszADAGUCTime+11,pszRadarTime+15,2);
      strncpy(pszADAGUCTime+13,pszRadarTime+18,2);
      pszADAGUCTime[15]='\0';
      //CDBDebug("%s --> %s",pszRadarTime,pszADAGUCTime);
      return 0;
    }
    
    void enableKNMIHDF5toCFConversion(){
      b_EnableKNMIHDF5toCFConversion=true;
    }
    int convertKNMIHDF5toCF(){
      //Fill in dim ranges
      CDF::Variable *var = cdfObject->getVariable("image1.image_data");
      if(var==NULL){CDBError("variable image1.image_data not found");return 1;}
      CDF::Variable *geo = cdfObject->getVariable("geographic");
      if(geo==NULL){CDBError("variable geographic not found");return 1;}
      if(var->dimensionlinks.size()!=2){CDBError("variable does not have 2 dims");return 1;}
      CDF::Variable *proj = cdfObject->getVariable("geographic.map_projection");
      if(proj==NULL){CDBError("variable geographic.map_projection not found");return 1;}
      CDF::Attribute *cellsizeXattr =geo->getAttribute("geo_pixel_size_x");
      CDF::Attribute *cellsizeYattr =geo->getAttribute("geo_pixel_size_y");
      CDF::Attribute *offsetXattr =geo->getAttribute("geo_column_offset");
      CDF::Attribute *offsetYattr =geo->getAttribute("geo_row_offset");
      CDF::Attribute *proj4attr =proj->getAttribute("projection_proj4_params");
      if(cellsizeXattr==NULL||cellsizeYattr==NULL||offsetXattr==NULL||offsetYattr==NULL||proj4attr==NULL){
        CDBError("geographic attributes incorrect");return 1;
      }
      CDF::Variable *overview = cdfObject->getVariable("overview");
      if(overview==NULL){CDBError("variable overview not found");return 1;}
      CDF::Attribute *product_datetime_start =overview->getAttribute("product_datetime_start");
      if(product_datetime_start==NULL){CDBError("attribute product_datetime_start not found");return 1;}
      
      CDF::Dimension *dimX=var->dimensionlinks[1];
      CDF::Dimension *dimY=var->dimensionlinks[0];
      CDF::Variable *varX=cdfObject->getVariable(dimX->name.c_str());
      CDF::Variable *varY=cdfObject->getVariable(dimY->name.c_str());
      CDF::allocateData(varX->type,&varX->data,dimX->length);
      CDF::allocateData(varY->type,&varY->data,dimY->length);
      
      varX->setName("x");
      varY->setName("y");
      dimX->setName("x");
      dimY->setName("y");
      
      double cellSizeX,cellSizeY,offsetX,offsetY;
      cellsizeXattr->getData(&cellSizeX,1);
      cellsizeYattr->getData(&cellSizeY,1);
      offsetXattr->getData(&offsetX,1);
      offsetYattr->getData(&offsetY,1);
      
      for(size_t j=0;j<dimX->length;j=j+(dimX->length-1)){
        double x=(offsetX+double(j))*cellSizeX+cellSizeX/2;
        ((double*)varX->data)[j]=x;
      }
      
      for(size_t j=0;j<dimY->length;j=j+(dimY->length-1)){
        double y=(offsetY+float(j))*cellSizeY+cellSizeY/2;
        ((double*)varY->data)[j]=y;
      }
      
      CDF::Variable * projection = new CDF::Variable();
      cdfObject->addVariable(projection);
      projection->setName("projection");
      projection->type=CDF_CHAR;
      projection->isDimension=false;
      
      CDF::Attribute* proj4_params = new CDF::Attribute();
      proj4_params->setName("proj4_params");
      proj4_params->setData(CDF_CHAR,proj4attr->data,proj4attr->length);
      projection->addAttribute(proj4_params);
      //Set time dimension
      CDF::Variable * time= new CDF::Variable();
      CDF::Dimension* timeDim= new CDF::Dimension();
      cdfObject->addDimension(timeDim);
      cdfObject->addVariable(time);
      time->setName("time");
      timeDim->setName("time");
      timeDim->length=1;
      time->type=CDF_DOUBLE;
      time->isDimension=true;
      CDF::Attribute *time_units = new CDF::Attribute();
      time_units->setName("units");
      time_units->setData("minutes since 2000-01-01 00:00:00\0");
      time->addAttribute(time_units);
      time->dimensionlinks.push_back(timeDim);
      //Set adaguc time
      CADAGUC_time *adTime=new CADAGUC_time ((char*)time_units->data);
      char szADAGUCTime[100];
      status = HDF5ToADAGUCTime(szADAGUCTime,(char*)product_datetime_start->data);
      if(status!=0){
        CDBError("Could not initialize time: %s",(char*)time_units->data);
        return 1;
      }
      //printf("%s\n",szADAGUCTime);
      double offset;
      status = adTime->YYYYMMDDTHHMMSSTimeToOffset(offset,szADAGUCTime);
      time->setSize(1);
      CDF::allocateData(time->type,&time->data,time->getSize());
      ((double*)time->data)[0]=offset;
      if(status!=0){
        CDBError("Could not initialize time: %s",szADAGUCTime);
        delete adTime;
        return 1;
      }      
      
      delete adTime;
      if(status!=0){
        CDBError("Could not initialize time: %s",(char*)time_units->data);
        return 1;
      }
      //Loop through all images and set grid_mapping name
      CT::string varName;
      int v=1;

      
      
      do{
        varName.print("image%d.image_data",v);
        var = cdfObject->getVariable(varName.c_str());
        if(var!=NULL){
          CDF::Attribute* grid_mapping= new CDF::Attribute();
          grid_mapping->setName("grid_mapping");
          grid_mapping->setData(CDF_CHAR,(char*)"projection\0",11);
          var->addAttribute(grid_mapping);
          var->dimensionlinks.insert(var->dimensionlinks.begin(),1,timeDim);
          //Set no data
          //Get nodatavalue:
          
          varName.print("image%d.calibration",v);
          CDF::Variable *calibration = cdfObject->getVariable(varName.c_str());
          if(calibration!=NULL){
//            CDBDebug("Found calibration group");
            CDF::Attribute *calibration_out_of_image = calibration->getAttribute("calibration_out_of_image"); 
            if(calibration_out_of_image!=NULL){
  //            CDBDebug("Found calibration_out_of_image attribute");
              double dfNodata = 0;
              calibration_out_of_image->getData(&dfNodata,1);
    //          CDBDebug("Found calibration_out_of_image attribute value=%f, status = %d",dfNodata,status);
              if(dfNodata!=0){
                CDF::Attribute* nodata = new CDF::Attribute();
                nodata->setName("_FillValue");
                nodata->setData(CDF_DOUBLE,&dfNodata,1);
                var->addAttribute(nodata);
              }
            }
          }
          
      
          
          /*CDF::Attribute* nodata = new CDF::Attribute();
          nodata->setName("_FillValue");
          nodata->setData(var->type,0,1);
          var->addAttribute(nodata);*/
        }
        v++;
      }while(var!=NULL);
      return 0;
    }
    int open(const char *fileName){
      H5F_file = H5Fopen(fileName,H5F_ACC_RDONLY, H5P_DEFAULT );
      if(H5F_file <0){CDBError("could not open HDF5 file %s",fileName);return 1;}
      
      //Read global attributes
      //hid_t HDF5_group = H5Gopen(H5F_file,"."); API V1.6
      hid_t HDF5_group = H5Gopen2(H5F_file,".",H5P_DEFAULT);
      if(HDF5_group <0){CDBError("could not open HDF5 group");
        H5Fclose(H5F_file );
        
        return 1;
      }
      readAttributes(cdfObject->attributes,HDF5_group);//TODO
      H5Gclose(HDF5_group);
      
      list(H5F_file,(char*)"");
      
      if(b_EnableKNMIHDF5toCFConversion){
        return convertKNMIHDF5toCF();
      }
      return 0;
    }
    int close(){
      if(H5F_file!=-1)H5Fclose(H5F_file);
      return 0;
    }
    
    hid_t openH5GroupByName(char * varNameOut,size_t maxVarNameLen,const char * variableGroupName){
//      CDBError("openH5GroupByName");
      hid_t HDF5_group=H5F_file;hid_t newGroupID;
      CT::string varName(variableGroupName);
      CT::string * paths=varName.split(".");
      for(int j=0;j<paths->count-1;j++){
        
        newGroupID= H5Gopen2(HDF5_group ,paths[j].c_str(),H5P_DEFAULT);
        //CDBDebug("Opened group %s with id %d from %d",paths[j].c_str(),newGroupID,HDF5_group);
        if(newGroupID<0){
          CDBError("group %s not found",paths[j].c_str());delete[] paths;paths=NULL;
          return -1;
        }
        
        opengroups.push_back(newGroupID);
        HDF5_group=newGroupID;
      }
      
      if(maxVarNameLen<paths[paths->count-1].length()+1){
        CDBError("varName string size not large enough to hold variable name ");
        delete[] paths;paths=NULL;
        return -1;
      }
      snprintf(varNameOut,maxVarNameLen,"%s",paths[paths->count-1].c_str());
      delete[] paths;paths=NULL;
      return HDF5_group;
    }
    void closeH5GroupByName(const char *variableGroupName){
      do{
        //CDBDebug("closing with id %d",opengroups.back());
        opengroups.pop_back();
      }while(opengroups.size()>0);
    }
    int readVariableData(CDF::Variable *var, CDFType type,size_t *start,size_t *count,ptrdiff_t *stride){
      
      char typeName[32];
      CDF::getCDFDataTypeName(typeName,31,type);
      CDBDebug("Reading %s with type %s",var->orgName.c_str(),typeName);
      char varName[1024];
      hid_t HDF5_group=openH5GroupByName(varName,1023,var->orgName.c_str());
      if(HDF5_group>0){
        hid_t datasetID = H5Dopen2(HDF5_group,varName,H5P_DEFAULT);
        if(datasetID>0){
          hid_t   HDF5_dataspace = H5Dget_space(datasetID); 
          int     ndims          = H5Sget_simple_extent_ndims(HDF5_dataspace);
          hsize_t dims_out[ndims];
          H5Sget_simple_extent_dims(HDF5_dataspace, dims_out, NULL);
          hsize_t mem_count[ndims],mem_start[ndims];
          hsize_t data_count[ndims],data_start[ndims];
          int totalVariableSize=1;
          int dimDiff=var->dimensionlinks.size()-ndims;
          if(dimDiff<0)dimDiff=0;
          for(int d=0;d<ndims;d++){
            mem_start[d]=start[d+dimDiff];
            mem_count[d]=count[d+dimDiff];
            data_start[d]=mem_start[d];
            data_count[d]=mem_count[d];
            totalVariableSize*=mem_count[d];;
          }
          var->setSize(totalVariableSize);
          CDF::allocateData(type,&var->data,var->getSize());
          hid_t HDF5_memspace = H5Screate_simple(2,mem_count,NULL);
          H5Sselect_hyperslab(HDF5_memspace,H5S_SELECT_SET,mem_start, NULL,mem_count, NULL);
          H5Sselect_hyperslab(HDF5_dataspace,H5S_SELECT_SET,data_start, NULL,data_count, NULL);
          hid_t datasetType=H5Dget_type(datasetID);
          H5Dread(datasetID,datasetType,HDF5_memspace,HDF5_dataspace,H5P_DEFAULT,var->data);
          
                    
          if(type==CDF_USHORT){
          for(size_t j=0;j<var->getSize();j++){
          unsigned short a=((unsigned short*)(var->data))[j];
          unsigned char p1=a;
          unsigned char p2=a/256;
          a=p1*256+p2;
          ((unsigned short*)(var->data))[j]=a;
        }
        }
          
          H5Tclose(datasetType);
          H5Sclose(HDF5_memspace);
          H5Sclose(HDF5_dataspace);
          H5Dclose(datasetID);
        }else {CDBError("Could not open variable with name %s %d",varName,datasetID);}
      }
      closeH5GroupByName(var->name.c_str());
      return 0;
    }
    int readVariableData(CDF::Variable *var, CDFType type){
      //All ready in memory
      int status =0;
//      CDBDebug(" ***** %s size=%d",var->name.c_str(),var->size());
      if(var->data!=NULL){
        //CDBWarning("Not reading any data because it is already in memory");
        return 0;
      }
      char varName[1024];
      hid_t HDF5_group=openH5GroupByName(varName,1023,var->orgName.c_str());
      if(HDF5_group>0){
        hid_t datasetID = H5Dopen2(HDF5_group,varName,H5P_DEFAULT);
        if(datasetID>0){
          hid_t   HDF5_dataspace = H5Dget_space(datasetID); 
          int     ndims          = H5Sget_simple_extent_ndims(HDF5_dataspace);
          hsize_t dims_out[ndims];
          H5Sget_simple_extent_dims(HDF5_dataspace, dims_out, NULL);
          hsize_t mem_count[ndims],mem_start[ndims];
          int totalVariableSize=1;
          
          for(int d=0;d<ndims;d++){
            mem_start[d]=0;
            mem_count[d]=dims_out[d];
            totalVariableSize*=mem_count[d];;
            //CDBDebug("dim size fpr %s = %d",varName,mem_count[d]);
          }

          var->setSize(totalVariableSize);
          CDF::allocateData(type,&var->data,var->getSize());
          hid_t HDF5_memspace = H5Screate_simple(2,mem_count,NULL);
          H5Sselect_hyperslab(HDF5_memspace,H5S_SELECT_SET,mem_start, NULL,mem_count, NULL);
          H5Sselect_hyperslab(HDF5_dataspace,H5S_SELECT_SET,mem_start, NULL,mem_count, NULL);
          hid_t datasetType=H5Dget_type(datasetID);
          
          //datasetType
          H5Dread(datasetID,datasetType,HDF5_memspace,HDF5_dataspace,H5P_DEFAULT,var->data);
          
          
          /*if(type==CDF_USHORT)
          {
            for(size_t j=0;j<var->getSize();j++){
              unsigned short a=((unsigned short*)(var->data))[j];
              unsigned char p1=a;
              unsigned char p2=a/256;
              a=p1*256+p2;
              ((unsigned short*)(var->data))[j]=a;
            }
        }*/
          
          H5Tclose(datasetType);
          H5Sclose(HDF5_memspace);
          H5Sclose(HDF5_dataspace);
          H5Dclose(datasetID);
        }else {
          CDBError("Unable to find dataset id for variable %s",varName);
          status=-1;
        }
      }else{
        CDBError("Unable to find group id");
      }
      closeH5GroupByName(var->name.c_str());
      return status;
    }
};


#endif
