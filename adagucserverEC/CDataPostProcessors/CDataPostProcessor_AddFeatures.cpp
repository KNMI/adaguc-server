#include "CDataPostProcessor_AddFeatures.h"
#include "CDataReader.h"

/************************/
/*      CDPPAddFeatures     */
/************************/
const char *CDPPAddFeatures::className = "CDPPAddFeatures";

const char *CDPPAddFeatures::getId() { return CDATAPOSTPROCESSOR_ADDFEATURES_ID; }
int CDPPAddFeatures::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_ADDFEATURES_ID)) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPAddFeatures::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    //    dataSource->getDataObject(0)->cdfVariable->setAttributeText("units","mm/hr");
    //    dataSource->getDataObject(0)->setUnits("mm/hr");
    try {
      if (dataSource->getDataObject(0)->cdfVariable->getAttribute("ADAGUC_GEOJSONPOINT")) return 0;
    } catch (int a) {
    }
    CDBDebug("CDATAPOSTPROCESSOR_RUNBEFOREREADING::Adding features from GEOJson");
    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;

    CDataSource::DataObject *newDataObject = new CDataSource::DataObject();

    newDataObject->variableName.copy("indexes");
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin() + 1, newDataObject);

    newDataObject->cdfVariable = new CDF::Variable();
    newDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    newDataObject->cdfObject->addVariable(newDataObject->cdfVariable);
    newDataObject->cdfVariable->setName("indexes");
    newDataObject->cdfVariable->setType(CDF_USHORT);
    newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

    for (size_t j = 0; j < varToClone->dimensionlinks.size(); j++) {
      newDataObject->cdfVariable->dimensionlinks.push_back(varToClone->dimensionlinks[j]);
    }
    newDataObject->cdfVariable->removeAttribute("standard_name");
    newDataObject->cdfVariable->removeAttribute("_FillValue");

    newDataObject->cdfVariable->setAttributeText("standard_name", "indexes");
    newDataObject->cdfVariable->setAttributeText("long_name", "indexes");
    newDataObject->cdfVariable->setAttributeText("units", "1");
    newDataObject->cdfVariable->setAttributeText("ADAGUC_GEOJSONPOINT", "1");
    dataSource->getDataObject(0)->cdfVariable->setAttributeText("ADAGUC_GEOJSONPOINT", "1");

    unsigned short sf = 65535u;
    newDataObject->cdfVariable->setAttribute("_FillValue", CDF_USHORT, &sf, 1);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDataSource featureDataSource;
    if (featureDataSource.setCFGLayer(dataSource->srvParams, dataSource->srvParams->configObj->Configuration[0], dataSource->srvParams->cfg->Layer[0], NULL, 0) != 0) {
      return 1;
    }
    featureDataSource.addStep(proc->attr.a.c_str()); // Set filename
    CDataReader reader;
    CDBDebug("Opening %s", featureDataSource.getFileName());
    int status = reader.open(&featureDataSource, CNETCDFREADER_MODE_OPEN_ALL);
    //   CDBDebug("fds: %s", CDF::dump(featureDataSource.getDataObject(0)->cdfObject).c_str());

    if (status != 0) {
      CDBDebug("Can't open file %s", proc->attr.a.c_str());
      return 1;
    } else {
      CDF::Variable *fvar = featureDataSource.getDataObject(0)->cdfObject->getVariableNE("featureids");
      size_t nrFeatures = 0;
      if (fvar == NULL) {
        CDBDebug("featureids not found");
      } else {
        //       CDBDebug("featureids found %d %d", fvar->getType(), fvar->dimensionlinks[0]->getSize());
        size_t start = 0;
        nrFeatures = fvar->dimensionlinks[0]->getSize();
        ptrdiff_t stride = 1;
        fvar->readData(CDF_STRING, &start, &nrFeatures, &stride, false);
        //         for (size_t i=0; i<nrFeatures; i++) {
        //           CDBDebug(">> %s", ((char **)fvar->data)[i]);
        //         }
      }
      char **names = (char **)fvar->data;

      float destNoDataValue = dataSource->getDataObject(0)->dfNodataValue;

      std::vector<std::string> valueMap;
      size_t nrPoints = dataSource->getDataObject(0)->points.size();
      float valueForFeatureNr[nrFeatures];
      for (size_t f = 0; f < nrFeatures; f++) {
        valueForFeatureNr[f] = destNoDataValue;
        const char *name = names[f];
        bool found = false;
        for (size_t p = 0; p < nrPoints && !found; p++) {
          for (size_t c = 0; c < dataSource->getDataObject(0)->points[p].paramList.size() && !found; c++) {
            CKeyValue ckv = dataSource->getDataObject(0)->points[p].paramList[c];
            //            CDBDebug("ckv: %s %s", ckv.key.c_str(), ckv.value.c_str());
            if (ckv.key.equals("station")) {
              CT::string station = ckv.value;
              //              CDBDebug("  comparing %s %s", station.c_str(), name);
              if (strcmp(station.c_str(), name) == 0) {
                valueForFeatureNr[f] = dataSource->getDataObject(0)->points[p].v;
                //                CDBDebug("Found %s as %d (%f)", name, f, valueForFeatureNr[f]);
                found = true;
              }
            }
          }
        }
      }

      CDF::allocateData(dataSource->getDataObject(1)->cdfVariable->getType(), &dataSource->getDataObject(1)->cdfVariable->data, dataSource->dWidth * dataSource->dHeight); // For a 2D field
      // Copy the gridded values from the geojson grid to the point data's grid
      size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
      unsigned short *src = (unsigned short *)featureDataSource.getDataObject(0)->cdfVariable->data;
      float *dest = (float *)dataSource->getDataObject(0)->cdfVariable->data;
      unsigned short noDataValue = featureDataSource.getDataObject(0)->dfNodataValue;
      unsigned short *indexDest = (unsigned short *)dataSource->getDataObject(1)->cdfVariable->data;

      //     size_t nrFeatures=valueMap.size();
      for (size_t cnt = 0; cnt < l; cnt++) {
        unsigned short featureNr = *src; // index of station in GeoJSON file
        *dest = destNoDataValue;
        *indexDest = 65535u;
        //         if (cnt<30) {
        //           CDBDebug("cnt=%d %d %f", cnt, featureNr, (featureNr!=noDataValue)?featureNr:-9999999);
        //         }
        if (featureNr != noDataValue) {
          *dest = valueForFeatureNr[featureNr];
          *indexDest = featureNr;
        }
        src++;
        dest++;
        indexDest++;
      }
      CDBDebug("Setting ADAGUC_SKIP_POINTS");
      dataSource->getDataObject(0)->cdfVariable->setAttributeText("ADAGUC_SKIP_POINTS", "1");
      dataSource->getDataObject(1)->cdfVariable->setAttributeText("ADAGUC_SKIP_POINTS", "1");
    }
  }
  return 0;
}
