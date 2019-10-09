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

#ifndef CCONVERTCURVILINEAR_H
#define CCONVERTCURVILINEAR_H
#include "CDataSource.h"
#include "CImageWarper.h"
class CConvertCurvilinear{
  private:
  
  
  DEF_ERRORFUNCTION();
  static double *getBBOXFromLatLonFields( CDF::Variable *swathMiddleLon, CDF::Variable *swathMiddleLat);
  static int checkIfIsCurvilinear(CDFObject *cdfObject,CServerParams *srvParams, bool &hasLatLonBounds);
  public:
  static int convertCurvilinearHeader(CDFObject *cdfObject,CServerParams *srvParams);
  static int convertCurvilinearData(CDataSource *dataSource,int mode);
};
#endif
