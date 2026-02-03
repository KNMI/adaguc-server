#include <vector>
#include <iterator>
#include <algorithm>
#include <stdio.h>
#include <netcdf.h>
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CDirReader.h"
#include "CTime.h"

#define VERSION "ADAGUC aggregator 1.6"

class NCFileObject {
public:
  NCFileObject(const char *filename) {
    cdfObject = NULL;
    cdfReader = NULL;
    cdfObject = new CDFObject();
    CT::string f = filename;
    if (f.endsWith(".h5")) {
      cdfReader = new CDFHDF5Reader();
      ((CDFHDF5Reader *)cdfReader)->enableKNMIHDF5toCFConversion();
    } else {
      cdfReader = new CDFNetCDFReader();
    }
    cdfObject->attachCDFReader(cdfReader);
    keep = true;
  }
  ~NCFileObject() {
    delete cdfObject;
    cdfObject = NULL;
    delete cdfReader;
    cdfReader = NULL;
  }
  bool keep;
  CDFObject *cdfObject;
  CDFReader *cdfReader;
  CT::string fullName;
  CT::string baseName;
  std::string dimAggregationValue;
  static bool sortFunction(NCFileObject *i, NCFileObject *j) { return (i->dimAggregationValue < j->dimAggregationValue); }
};

void progress(const char *message, float percentage) { printf("{\"message\":%s,\"percentage\":\"%d\"}\n", message, (int)(percentage)); }

void progresswrite(const char *message, float percentage) { progress(message, percentage / 2. + 50); }
int memberNo = 1;
void applyChangesToCDFObject(const char *_fileName, CDFObject *cdfObject, std::vector<CT::string> variablesToDo, const char *dimNameToAggregate) {

  CT::string memberValue;
#define CLIPC_ENSEMBLES_GERICS
#ifdef CLIPC_ENSEMBLES_KNMI
  CT::string fileName = _fileName;
  int KNMIINDEX = fileName.indexOf("ens-multiModel-");

  if (KNMIINDEX == -1) {
    KNMIINDEX = fileName.indexOf("KNMI");
    memberValue = fileName.substring(KNMIINDEX + 5, -1);
  } else {
    memberValue = fileName.substring(KNMIINDEX + 10, -1);
    KNMIINDEX = memberValue.indexOf("-");
    memberValue = memberValue.substring(KNMIINDEX + 1, -1);
    KNMIINDEX = memberValue.indexOf("-");
    memberValue = memberValue.substring(0, KNMIINDEX);
  }

  int end = memberValue.indexOf("_");
  memberValue.substringSelf(0, end);
#endif

#ifdef CLIPC_ENSEMBLES_GERICS
  CT::string fileName = _fileName;
  std::vector<CT::string> parts = fileName.split("_");
  memberValue = parts[3];
  memberValue.concat("_with_");
  memberValue += parts[6];
  if (memberValue.indexOf("median") != -1) memberValue = "median";
  if (memberValue.indexOf("20p") != -1) memberValue = "20p";
  if (memberValue.indexOf("80p") != -1) memberValue = "80p";
#endif

  for (size_t j = 0; j < variablesToDo.size(); j++) {

    CDF::Dimension *dim = cdfObject->getDimensionNE(dimNameToAggregate);
    if (dim == NULL) {

      dim = new CDF::Dimension();
      dim->name = dimNameToAggregate;
      dim->length = 1;
      cdfObject->addDimension(dim);
    }
    CDF::Variable *dimVar = cdfObject->getVariableNE(dimNameToAggregate);
    if (dimVar == NULL) {
      dimVar = new CDF::Variable();
      cdfObject->addVariable(dimVar);
      dimVar->name = dimNameToAggregate;
      dimVar->setType(CDF_STRING);
      dimVar->dimensionlinks.push_back(dim);
      dimVar->isDimension = true;
      dimVar->setSize(1);

      CDF::allocateData(dimVar->currentType, &dimVar->data, dimVar->getSize());
      ((char **)dimVar->data)[0] = strdup(memberValue.c_str());
    }

    CDF::Variable *varWithoutDimension = cdfObject->getVariableNE(variablesToDo[j].c_str());
    if (varWithoutDimension != NULL) {
      varWithoutDimension->dimensionlinks.insert(varWithoutDimension->dimensionlinks.begin(), dim);
    } else {
      CDBWarning("Variable %s not found", variablesToDo[j].c_str());
    }
  }
}

int main(int argc, const char *argv[]) {
  const char *dimNameToAggregate = "member";
  int status = 0;
  /* Chunk cache needs to be set to zero, otherwise netcdf runs out of its memory.... */
  status = nc_set_chunk_cache(0, 0, 0);
  if (status != NC_NOERR) {
    CDBError("Unable to set nc_set_chunk_cache to zero");
    return 1;
  }

  if (argc != 3 && argc != 4) {
    CDBDebug("Argument count is wrong, please specifiy an input dir and output file, plus optionally a comma separated list of variable names to add the new dimension to.");
    return 1;
  }

  CT::string inputDir = argv[1];
  CT::string outputFile = argv[2];

  CDirReader dirReader;
  CT::string dirFilter = "^.*.*\\.nc";

  dirReader.listDirRecursive(inputDir.c_str(), dirFilter.c_str());
  if (dirReader.fileList.size() == 0) {
    dirFilter = "^.*.*\\.h5";
    dirReader.listDirRecursive(inputDir.c_str(), dirFilter.c_str());
    if (dirReader.fileList.size() == 0) {
      CDBError("No netcdf (*.nc) or hdf5 (*.h5) files files in input directory %s", inputDir.c_str());
      return 1;
    }
  }

  std::vector<CT::string> variablesToAddDimTo;
  if (argc == 4) {
    CT::string variableList = argv[3];
    variablesToAddDimTo = variableList.split(",");
  }

  /* Create a vector which holds information for all the inputfiles. */
  std::vector<NCFileObject *> fileObjects;
  std::vector<NCFileObject *>::iterator fileObjectsIt;

  /* Loop through all files and gather information */
  try {
    for (size_t j = 0; j < dirReader.fileList.size(); j++) {
      NCFileObject *fileObject = new NCFileObject(CT::string(dirReader.fileList[j].c_str()).basename().c_str());
      fileObjects.push_back(fileObject);
      fileObject->fullName = dirReader.fileList[j].c_str();
      fileObject->baseName = CT::string(dirReader.fileList[j].c_str()).basename().c_str();

      status = fileObject->cdfObject->open(fileObject->fullName.c_str());

      if (status != 0) {
        CDBError("Unable to read file %s", fileObject->fullName.c_str());
        throw(__LINE__);
      }

      applyChangesToCDFObject(CT::string(dirReader.fileList[j].c_str()).basename().c_str(), fileObject->cdfObject, variablesToAddDimTo, dimNameToAggregate);

      CDF::Variable *aggregationDim = fileObject->cdfObject->getVariableNE(dimNameToAggregate);
      if (aggregationDim == NULL) {
        CDBError("Unable to find aggregation variable [%s]", dimNameToAggregate);
        throw(__LINE__);
      }
      CT::string message;

      CDFType aggregationType = aggregationDim->getType();

      if (aggregationType != CDF_STRING) {
        status = aggregationDim->readData(CDF_DOUBLE);
      } else {
        status = aggregationDim->readData(CDF_STRING);
      }
      if (status != 0) {
        CDBError("Unable to read variable %s", aggregationDim->name.c_str());
        throw(__LINE__);
      }

      bool isTimeDim = false;
      if (isTimeDim) {
        double value = ((double *)(aggregationDim->data))[0];
        CT::string units;
        try {
          units = aggregationDim->getAttribute("units")->getDataAsString().c_str();
        } catch (int e) {
          CDBError("Unable to find units attribute for time variable");
          throw(__LINE__);
        }

        CTime time;
        time.init(aggregationDim);
        CTime::Date date = time.getDate(value);
        CTime epochCTime;
        epochCTime.init("seconds since 1970-01-01 0:0:0", "");
        CT::string a;
        a.print("%f", epochCTime.dateToOffset(date));
        fileObject->dimAggregationValue = a.c_str();
        ;
        message.print("\"Checking file (%d/%d) %s, has start date %s\"", j, dirReader.fileList.size(), fileObject->baseName.c_str(), epochCTime.dateToISOString(date).c_str());
      } else {
        if (aggregationType != CDF_STRING) {
          double value = ((double *)(aggregationDim->data))[0];
          CT::string a;
          a.print("%f", value);
          fileObject->dimAggregationValue = a.c_str();
          message.print("\"Checking file (%d/%d) %s with value %f", j, dirReader.fileList.size(), fileObject->baseName.c_str(), value);
        } else {
          char *value = ((char **)(aggregationDim->data))[0];
          fileObject->dimAggregationValue = value;
          message.print("\"Checking file (%d/%d) %s with value %s", j, dirReader.fileList.size(), fileObject->baseName.c_str(), value);
        }
      }

      progress(message.c_str(), (float(j) / float(dirReader.fileList.size())) * 50);

      fileObject->keep = true;
      fileObject->cdfObject->close();
    }
  } catch (int linenr) {

    CDBError("Exception at line %d", linenr);
    for (size_t j = 0; j < fileObjects.size(); j++) delete fileObjects[j];
    return 1;
  }

  /* Sort the dates according the dimAggregationValue */
  std::sort(fileObjects.begin(), fileObjects.end(), NCFileObject::sortFunction);

  CT::string netcdfFile = fileObjects[0]->fullName.c_str();
  CT::string netcdfFileBase = fileObjects[0]->baseName.c_str();
  CDBDebug("Reading %s", netcdfFile.c_str());
  CDFObject *destCDFObject = new CDFObject();
  CDFReader *cdfReader;
  if (netcdfFile.endsWith(".h5")) {
    cdfReader = new CDFHDF5Reader();
    ((CDFHDF5Reader *)cdfReader)->enableKNMIHDF5toCFConversion();
  } else {
    cdfReader = new CDFNetCDFReader();
  }
  destCDFObject->attachCDFReader(cdfReader);
  status = destCDFObject->open(netcdfFile.c_str());
  applyChangesToCDFObject(netcdfFileBase.c_str(), destCDFObject, variablesToAddDimTo, dimNameToAggregate);
  if (status != 0) {
    CDBError("Unable to read file %s", netcdfFile.c_str());
    throw(__LINE__);
  }

  CT::string usedInputFiles = "";
  try {
    for (size_t j = 0; j < fileObjects.size(); j++) {
      try {
        if (destCDFObject->aggregateDim(fileObjects[j]->cdfObject, dimNameToAggregate) != 0) throw(__LINE__);
      } catch (int e) {
        CDBError("Unable to aggregate dimension for %s", fileObjects[j]->baseName.c_str());
        throw(__LINE__);
      }
      if (usedInputFiles.length() > 0) {
        usedInputFiles.concat(",");
      }
      usedInputFiles.printconcat("%s", fileObjects[j]->baseName.c_str());
    }
  } catch (int e) {
    for (size_t j = 0; j < fileObjects.size(); j++) delete fileObjects[j];
    delete cdfReader;
    delete destCDFObject;
    return 1;
  }

  destCDFObject->attributes.clear();
  std::vector<CDF::Attribute *> attributes = fileObjects[4]->cdfObject->attributes;
  for (size_t j = 0; j < attributes.size(); j++) {
    destCDFObject->attributes.push_back(new CDF::Attribute(attributes[j]));
  }
  CT::string history;
  history.print("Aggregated members into a single file with ADAGUC. Used input files: %s", usedInputFiles.c_str());
  destCDFObject->setAttributeText("history", history.c_str());

  CDFNetCDFWriter *netCDFWriter = new CDFNetCDFWriter(destCDFObject);
  netCDFWriter->setNetCDFMode(4);
  status = netCDFWriter->write(outputFile.c_str(), &progresswrite);
  delete netCDFWriter;

  for (size_t j = 0; j < fileObjects.size(); j++) delete fileObjects[j];
  delete cdfReader;
  delete destCDFObject;
  //

  return 0;
}
