#include <stdio.h>
#include <vector>
#include <iostream>
#include <CTypes.h>
#include "CDebugger.h" 
#include "CCDFDataModel.h"
#include "CCDFGeoJSONIO.h"
DEF_ERRORMAIN();

int main(int argCount,char **argVars){
    CDFGeoJSONReader *geoJSONReader = NULL;
    CDFObject *cdfObject = NULL;
    if(argCount<=2){
      printf("geojsondump [-h] file\n");
      printf("  [-h]             Header information only, no data\n");
      return 0;
    }
    const char *inputfile=argVars[argCount-1];//"/nobackup/users/plieger/projects/msgcpp/oud/meteosat9.fl.geo.h5";
    int status = 0;
    try{
      cdfObject=new CDFObject();
      geoJSONReader = new CDFGeoJSONReader();
      cdfObject->attachCDFReader(geoJSONReader);
      status = geoJSONReader->open(inputfile);
      if(status != 0){CDBError("Unable to read file %s",inputfile);throw(__LINE__);}
      CT::string dumpString = CDF::dump(cdfObject);
      printf("%s\n",dumpString.c_str());
      delete geoJSONReader;geoJSONReader=NULL;
      delete cdfObject;cdfObject=NULL;
    }
    catch(int e){
      delete geoJSONReader;geoJSONReader=NULL;
      delete cdfObject;cdfObject=NULL;
    }
      
  return 0;
}
