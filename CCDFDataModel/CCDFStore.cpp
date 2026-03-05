#include "CCDFStore.h"
#include "CCDFObject.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"

std::map<std::string, CDFReader *> CDFStore::cdfReaders = {};

CDFReader *CDFStore::getCDFReader(std::string fileName) {
  // CDBDebug("CDFReader");
  CDFReader *cdfReader = NULL;

  std::map<std::string, CDFReader *>::iterator it = cdfReaders.find(fileName);
  if (it != cdfReaders.end()) {

    cdfReader = (*it).second;
#ifdef CCDFSTORE_DEBUG
    CDBDebug("Found existing cdfReaders object with id %s", fileName.c_str());
#endif
    return cdfReader;
  }

  cdfReader = new CDFNetCDFReader();
#ifdef CCDFSTORE_DEBUG
  CDBDebug("Inserting new cdfreader with id %s", fileName.c_str());
#endif
  cdfReaders.insert(std::pair<std::string, CDFReader *>(fileName, cdfReader));
  return cdfReader;
}

void CDFStore::clear() {
  for (CDFStore_CDFReadersIterator iterator = cdfReaders.begin(); iterator != cdfReaders.end(); iterator++) {
    CDFReader *cdfReader = iterator->second;
    cdfReader->close();
    delete iterator->second;
  }
}