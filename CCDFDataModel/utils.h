
#include <stdio.h>
#include <vector>
#include <iostream>
#include <CTString.h>
#include "CDebugger.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CCDFGeoJSONIO.h"
#include "CCDFCSVReader.h"
#include "CCDFPNGIO.h"

#ifndef ADAGUCUTILS_H
#define ADAGUCUTILS_H

CDFReader *findReaderByFileName(CT::string fileName);

#endif