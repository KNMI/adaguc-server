
#include <stdio.h>
#include <vector>
#include <iostream>
#include <CTypes.h>
#include "CDebugger.h" 
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CCDFGeoJSONIO.h"
#include "CCDFCSVReader.h"
#include "CCDFPNGIO.h"

#ifndef ADAGUCUTILS_H
#define ADAGUCUTILS_H

CDFReader*findReaderByFileName(CT::string fileName){
  if(fileName.endsWith(".nc")){
    return new CDFNetCDFReader() ;
  }
  if(fileName.endsWith(".h5")){
    return new CDFHDF5Reader();
  }
  if(fileName.endsWith(".geojson")){
    return new CDFGeoJSONReader();
  }
  if(fileName.endsWith(".csv")){
    return new CDFCSVReader();
  }
  if(fileName.endsWith(".png")){
    return new CDFPNGReader();
  }
  return NULL;
}

#endif