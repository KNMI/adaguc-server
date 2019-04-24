/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#ifndef CCONVERTADAGUCPOINT_H
#define CCONVERTADAGUCPOINT_H
#include "CDataSource.h"
class CConvertADAGUCPoint{
  private:
  DEF_ERRORFUNCTION();
  static void lineInterpolated(float *grid , int W,int H, int startX,int startY, int stopX, int stopY, float startVal, float stopVal);
  static int checkIfADAGUCPointFormat(CDFObject *cdfObject);
  static void convert_BIRA_IASB_NETCDF(CDFObject *cdfObject);
  public:
  static int convertADAGUCPointHeader(CDFObject *cdfObject);
  static int convertADAGUCPointData(CDataSource *dataSource,int mode);
};
#endif
