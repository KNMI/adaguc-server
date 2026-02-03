// /******************************************************************************
//  *
//  * Project:  Generic common data format
//  * Purpose:  Generic Data model to read netcdf and hdf5
//  * Author:   Maarten Plieger, plieger "at" knmi.nl
//  * Date:     2013-06-01
//  *
//  ******************************************************************************
//  *
//  * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
//  *
//  * Licensed under the Apache License, Version 2.0 (the "License");
//  * you may not use this file except in compliance with the License.
//  * You may obtain a copy of the License at
//  *
//  *      http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
//  *
//  ******************************************************************************/
//
// #ifndef CCDFWARPER_H
// #define CCDFWARPER_H
//
//
// #include <cstdio>
// #include <vector>
// #include <iostream>
// #include <netcdf.h>
// #include <cmath>
// #include "CDebugger.h"
// #include "CCDFDataModel.h"
//
// class CCDFWarper{
//   private:
//   DEF_ERRORFUNCTION();
//   bool lonWarpNeeded;size_t lonWarpStartIndex;
//
//   public:
//     bool enableLonWarp;
//   CCDFWarper(){
//     lonWarpNeeded=false;
//     lonWarpStartIndex = 0;
//     enableLonWarp=false;//true;//true;
//   }
//
//
//   int warpLonData(CDF::Variable *variable){
//     //Apply longitude warping of the data
//     //Longitude data must be already present in order to make variable warping available.
//     //EG 0-360 to -180 till -180
//     //TODO This function only works on a single 2D datafield!
//     if(enableLonWarp){
//
//       CDBDebug("Warping data");
//
//       //Find and warp lon variable
//       if(variable->name.equals("lon")){
//         if(variable->dimensionlinks.size()==1){
//
//           //CDBDebug("Warplon: Found variable lon");
//           double *lonData = new double[variable->getSize()];
//           CDF::DataCopier::copy(lonData,CDF_DOUBLE,variable->data,variable->getType(),0,0,variable->getSize());
//           double average=0;
//           double cellSize = fabs(lonData[0]-lonData[1]);
//           for(size_t j=0;j<variable->getSize();j++){
//             average+=lonData[j];
//           }
//           average/=double(variable->getSize());
//           //CDBDebug("Warplon: average = %f",average);
//           if(average>180-cellSize&&average<180+cellSize)lonWarpNeeded=true;
//           if(lonWarpNeeded==true){
//             for(size_t j=0;j<variable->getSize()&&(lonData[j]<=180);j++){
//               lonWarpStartIndex=j;
//             }
//             //Warp longitude:
//             for(size_t j=0;j<variable->getSize();j++)lonData[j]-=180;
//             CDF::DataCopier::copy(variable->data,variable->getType(),lonData,CDF_DOUBLE,0,0,variable->getSize());
//           }
//           delete[] lonData;
//           return 0;
//         }
//       }
//
//       if(lonWarpNeeded==true){
//         //Warp all other variables (except lon)
//         if(variable->dimensionlinks.size()>=2){
//           if(variable->getType()!=CDF_FLOAT&&variable->getType()!=CDF_DOUBLE){
//             CDBError("Warning! warpLonData is not supported for datatypes other than CDF_FLOAT or CDF_DOUBLE!");
//             return 1;
//           }
//           int dimIndex = variable->getDimensionIndexNE("lon");
//           if(dimIndex!=-1){
//             CDBDebug("Starting warpLonData with dimension lon for variable %s",variable->name.c_str());
//             if(dimIndex!=((int)variable->dimensionlinks.size())-1){
//               CDBError("Error while warping longitude dimension for variable %s: longitude is not the first index",variable->name.c_str());
//               return 1;
//             }
//             try{
//               CDF::Dimension *dim = variable->getDimension("lon");
//               size_t offset=0;
//               do{
//                 for(size_t lon=0;lon<lonWarpStartIndex&&(lon+lonWarpStartIndex<dim->length);lon++){
//                   if(variable->getType()==CDF_FLOAT){
//                     float *data=(float*)variable->data;
//                     size_t p1=lon+offset;
//                     size_t p2=lon+lonWarpStartIndex+offset;
//                     float tmp1=data[p1];
//                     float tmp2=data[p2];
//                     data[p1]=tmp2;
//                     data[p2]=tmp1;
//                   }
//                   if(variable->getType()==CDF_DOUBLE){
//                     double *data=(double*)variable->data;
//                     size_t p1=lon+offset;
//                     size_t p2=lon+lonWarpStartIndex+offset;
//                     double tmp1=data[p1];
//                     double tmp2=data[p2];
//                     data[p1]=tmp2;
//                     data[p2]=tmp1;
//                   }
//                 }
//                 offset+=dim->length;
//               }while(offset<variable->getSize());
//             }catch(...){
//               CDBError("Warplon: No dimension lon found for variable %s",variable->name.c_str());
//             }
//           }
//         }
//       }
//     }
//     return 0;
//   }
// };
// #endif
