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

#include <stdio.h>
#include <vector>
#include <iostream>
#include <CTypes.h>
#include "CDebugger.h" 
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
DEF_ERRORMAIN();

int main(int argCount,char **argVars){
    CDFHDF5Reader *hdf5Reader = NULL;
    CDFObject *cdfObject = NULL;
    if(argCount<=2){
      printf("h5ncdump [-h] file\n");
      printf("  [-h]             Header information only, no data\n");
      return 0;
    }
    const char *inputfile=argVars[argCount-1];//"/nobackup/users/plieger/projects/msgcpp/oud/meteosat9.fl.geo.h5";
    int status = 0;
    try{
      cdfObject=new CDFObject();
      hdf5Reader = new CDFHDF5Reader();
      cdfObject->attachCDFReader(hdf5Reader);
      status = hdf5Reader->open(inputfile);
      if(status != 0){CDBError("Unable to read file %s",inputfile);throw(__LINE__);}
      CT::string dumpString = CDF::dump(cdfObject);
      printf("%s\n",dumpString.c_str());
      delete hdf5Reader;hdf5Reader=NULL;
      delete cdfObject;cdfObject=NULL;
    }
    catch(int e){
      delete hdf5Reader;hdf5Reader=NULL;
      delete cdfObject;cdfObject=NULL;
    }
      
  return 0;
}
