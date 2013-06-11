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
const char *CDFNetCDFReader::className="NetCDFReader";
const char *CDFNetCDFWriter::className="NetCDFWriter";
const char *CCDFWarper::className="CCDFWarper";

void ncError(int line, const char *className, const char * msg,int e){
  if(e==NC_NOERR)return;
  char szTemp[1024];
  snprintf(szTemp,1023,"[E: %s, %d in class %s] %s: %s\n",__FILE__,line,className,msg,nc_strerror(e));
  printErrorStream(szTemp);
  
}
