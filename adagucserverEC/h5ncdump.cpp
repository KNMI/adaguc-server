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
      CT::string dumpString;
      CDF::dump(cdfObject,&dumpString);
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
