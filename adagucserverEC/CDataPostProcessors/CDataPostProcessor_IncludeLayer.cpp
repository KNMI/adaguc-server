#include <ranges>
#include "CDataPostProcessor_IncludeLayer.h"
#include "CRequest.h"
#include <utils/LayerUtils.h>
/************************/
/*CDPPIncludeLayer */
/************************/
const char *CDPPIncludeLayer::className = "CDPPIncludeLayer";

const char *CDPPIncludeLayer::getId() { return "include_layer"; }
int CDPPIncludeLayer::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals("include_layer")) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

CDataSource *CDPPIncludeLayer::getDataSource(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource) {
  CDataSource *dataSourceToInclude = new CDataSource();
  CT::string additionalLayerName = proc->attr.name.c_str();
  size_t additionalLayerNo = 0;
  for (size_t j = 0; j < dataSource->srvParams->cfg->Layer.size(); j++) {
    CT::string layerName = makeUniqueLayerName(dataSource->srvParams->cfg->Layer[j], nullptr);
    // CDBDebug("comparing for additionallayer %s==%s", additionalLayerName.c_str(), layerName.c_str());
    if (additionalLayerName.equals(layerName)) {
      additionalLayerNo = j;
      break;
    }
  }
  dataSourceToInclude->setCFGLayer(dataSource->srvParams, dataSource->srvParams->configObj->Configuration[0], dataSource->srvParams->cfg->Layer[additionalLayerNo], additionalLayerName.c_str(), 0);
  return dataSourceToInclude;
}

int CDPPIncludeLayer::setDimsForNewDataSource(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, CDataSource *dataSourceToInclude) {
  CT::string additionalLayerName = proc->attr.name.c_str();
  bool dataIsFound = false;
  try {
    if (CRequest::setDimValuesForDataSource(dataSourceToInclude, dataSource->srvParams) == 0) {
      dataIsFound = true;
    }
  } catch (ServiceExceptionCode e) {
  }
  if (dataIsFound == false) {
    CDBDebug("No data available for layer %s", additionalLayerName.c_str());
    return 1;
  }
  return 0;
}

int CDPPIncludeLayer::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }

  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {

    // CDBDebug("CDATAPOSTPROCESSOR_RUNBEFOREREADING::Applying include_layer");

    /* First check if this was already added */
    auto dataObjectsVector = *dataSource->getDataObjectsVector();
    for (auto it = dataObjectsVector.begin(); it != dataObjectsVector.end(); ++it) {
      CDataSource::DataObject *dataObject = *it;
      if (dataObject->dataObjectName.equals(proc->attr.name)) { // TODO SHould think of another identifier
        CDBDebug("This processor was already applied, skipping metadata part");
        return 0;
      }
    }

    // Load the other datasource.
    CDataSource *dataSourceToInclude = getDataSource(proc, dataSource);
    if (dataSourceToInclude == NULL) {
      CDBDebug("dataSourceToInclude has no data");
      return 0;
    }

    int status = setDimsForNewDataSource(proc, dataSource, dataSourceToInclude);
    if (status != 0) {
      CDBError("Trying to include datasource, but it has no values for given dimensions");
      delete dataSourceToInclude;
      ;
      return 1;
    }

    // CDBDebug("TEMPORAL METADATA READER");
    CDataReader reader;
    reader.enablePostProcessors = false;
    reader.enableObjectCache = false;
    status = reader.open(dataSourceToInclude, CNETCDFREADER_MODE_OPEN_HEADER); // Only read metadata
    if (status != 0) {
      CDBDebug("Can't open file %s for layer %s", dataSourceToInclude->getFileName(), proc->attr.name.c_str());
      return 1;
    }

    for (size_t dataObjectNr = 0; dataObjectNr < dataSource->getNumDataObjects(); dataObjectNr++) {
      if (dataSource->getDataObject(dataObjectNr)->cdfVariable->name.equals(dataSourceToInclude->getDataObject(0)->cdfVariable->name)) {
        CDBDebug("Probably already done");
        delete dataSourceToInclude;

        return 0;
      }
    }
    for (size_t dataObjectNr = 0; dataObjectNr < dataSourceToInclude->getNumDataObjects(); dataObjectNr++) {
      CDataSource::DataObject *currentDataObject = dataSource->getDataObject(0);
      CDF::Variable *varToClone = NULL;
      try {
        varToClone = dataSourceToInclude->getDataObject(dataObjectNr)->cdfVariable;
      } catch (int e) {
      }
      if (varToClone == NULL) {
        CDBError("Variable not found");
        return 1;
      }

      int mode = 0; // 0:append, 1:prepend
      if (proc->attr.mode.empty() == false) {
        if (proc->attr.mode.equals("append")) mode = 0;
        if (proc->attr.mode.equals("prepend")) mode = 1;
      }

      CDataSource::DataObject *newDataObject = new CDataSource::DataObject();
      if (mode == 0) dataSource->getDataObjectsVector()->push_back(newDataObject);

      if (mode == 1) dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), newDataObject);

      // CDBDebug("--------> newDataObject %d ", dataSource->getDataObjectsVector()->size());

      newDataObject->variableName.copy(varToClone->name.c_str());
      newDataObject->dataObjectName = proc->attr.name;

      newDataObject->cdfVariable = new CDF::Variable();
      CT::string text;
      text.print("{\"variable\":\"%s\",\"datapostproc\":\"%s\"}", varToClone->name.c_str(), this->getId());
      newDataObject->cdfObject = currentDataObject->cdfObject; //(CDFObject*)varToClone->getParentCDFObject();
      // CDBDebug("--------> Adding variable %s ", varToClone->name.c_str());
      newDataObject->cdfObject->addVariable(newDataObject->cdfVariable);
      newDataObject->cdfVariable->setName(varToClone->name.c_str());
      newDataObject->cdfVariable->setType(varToClone->getType());
      newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

      for (size_t j = 0; j < currentDataObject->cdfVariable->dimensionlinks.size(); j++) {
        // CDBDebug("Copying %d %s (%d)", j, currentDataObject->cdfVariable->dimensionlinks[j]->name.c_str(), currentDataObject->cdfVariable->dimensionlinks[j]->getSize());
        newDataObject->cdfVariable->dimensionlinks.push_back(currentDataObject->cdfVariable->dimensionlinks[j]);
      }

      // for (size_t j = 0; j < newDataObject->cdfVariable->dimensionlinks.size(); j++) {
      //   CDBDebug("NowHas dimensions %d %s (%d)", j, newDataObject->cdfVariable->dimensionlinks[j]->name.c_str(), newDataObject->cdfVariable->dimensionlinks[j]->getSize());
      // }

      for (size_t j = 0; j < varToClone->attributes.size(); j++) {
        newDataObject->cdfVariable->attributes.push_back(new CDF::Attribute(varToClone->attributes[j]));
      }
      newDataObject->cdfVariable->removeAttribute("ADAGUC_DATAOBJECTID");
      newDataObject->cdfVariable->setAttributeText("ADAGUC_DATAOBJECTID", text.c_str());

      newDataObject->cdfVariable->removeAttribute("scale_factor");
      newDataObject->cdfVariable->removeAttribute("add_offset");
      newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
    }
    reader.close();
    // CDBDebug("CLOSING TEMPORAL METADATA READER");
    delete dataSourceToInclude;
    return 0;
  }

  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    // CDBDebug("CDATAPOSTPROCESSOR_RUNAFTERREADING::Applying include_layer");

    // Load the other datasource.
    CDataSource *dataSourceToInclude = getDataSource(proc, dataSource);
    if (dataSourceToInclude == NULL) {
      CDBDebug("dataSourceToInclude has no data");
      return 0;
    }
    int status = setDimsForNewDataSource(proc, dataSource, dataSourceToInclude);
    if (status != 0) {
      CDBError("Trying to include datasource, but it has no values for given dimensions");
      delete dataSourceToInclude;
      ;
      return 1;
    }

    if (dataSourceToInclude->getNumTimeSteps() == dataSource->getNumTimeSteps()) {

      dataSourceToInclude->setTimeStep(dataSource->getCurrentTimeStep());
    }

    // CDBDebug("TEMPORAL FULL READER");
    CDataReader reader;
    reader.enablePostProcessors = false;
    reader.enableObjectCache = false;
    //    CDBDebug("Opening %s",dataSourceToInclude->getFileName());
    status = reader.open(dataSourceToInclude, CNETCDFREADER_MODE_OPEN_ALL); // Now open the data as well.
    if (status != 0) {
      CDBDebug("Can't open file %s for layer %s", dataSourceToInclude->getFileName(), proc->attr.name.c_str());
      return 1;
    }

    for (size_t dataObjectNr = 0; dataObjectNr < dataSourceToInclude->getNumDataObjects(); dataObjectNr++) {
      // This is the variable to read from
      CDF::Variable *varToClone = dataSourceToInclude->getDataObject(dataObjectNr)->cdfVariable;

      // This is the variable to write To
      auto *dataObjectForClone = dataSource->getDataObjectByName(varToClone->name.c_str());
      if (dataObjectForClone == nullptr) {
        return 1;
      }
      CDF::Variable *varToWriteTo = dataObjectForClone->cdfVariable;
      CDF::fill(varToWriteTo->data, varToWriteTo->getType(), 0, (size_t)dataSource->dHeight * (size_t)dataSource->dWidth);

      CDPPIncludeLayerSettings settings;
      settings.width = dataSource->dWidth;
      settings.height = dataSource->dHeight;
      settings.data = (void *)varToWriteTo->data;  // To write TO
      void *sourceData = (void *)varToClone->data; // To read FROM

      CGeoParams sourceGeo;
      sourceGeo.dWidth = dataSourceToInclude->dWidth;
      sourceGeo.dHeight = dataSourceToInclude->dHeight;
      sourceGeo.dfBBOX[0] = dataSourceToInclude->dfBBOX[0];
      sourceGeo.dfBBOX[1] = dataSourceToInclude->dfBBOX[1];
      sourceGeo.dfBBOX[2] = dataSourceToInclude->dfBBOX[2];
      sourceGeo.dfBBOX[3] = dataSourceToInclude->dfBBOX[3];
      sourceGeo.dfCellSizeX = dataSourceToInclude->dfCellSizeX;
      sourceGeo.dfCellSizeY = dataSourceToInclude->dfCellSizeY;
      sourceGeo.CRS = dataSourceToInclude->nativeProj4;

      CGeoParams destGeo;
      destGeo.dWidth = dataSource->dWidth;
      destGeo.dHeight = dataSource->dHeight;
      destGeo.dfBBOX[0] = dataSource->dfBBOX[0];
      destGeo.dfBBOX[1] = dataSource->dfBBOX[1];
      destGeo.dfBBOX[2] = dataSource->dfBBOX[2];
      destGeo.dfBBOX[3] = dataSource->dfBBOX[3];
      destGeo.dfCellSizeX = dataSource->dfCellSizeX;
      destGeo.dfCellSizeY = dataSource->dfCellSizeY;
      destGeo.CRS = dataSource->nativeProj4;

      CImageWarper warper;

      status = warper.initreproj(dataSourceToInclude, &destGeo, &dataSource->srvParams->cfg->Projection);
      if (status != 0) {
        CDBError("Unable to initialize projection");
        return 1;
      }
      GenericDataWarper genericDataWarper;

      switch (varToWriteTo->getType()) {
      case CDF_CHAR:
        genericDataWarper.render<char>(&warper, sourceData, &sourceGeo, &destGeo,
                                       [&settings](int x, int y, char val, GDWState &warperState) { return drawFunction<char>(x, y, val, warperState, &settings); });
        break;
      case CDF_BYTE:
        genericDataWarper.render<uchar>(&warper, sourceData, &sourceGeo, &destGeo,
                                        [&settings](int x, int y, uchar val, GDWState &warperState) { return drawFunction<uchar>(x, y, val, warperState, &settings); });
        break;
      case CDF_UBYTE:
        genericDataWarper.render<ubyte>(&warper, sourceData, &sourceGeo, &destGeo,
                                        [&settings](int x, int y, ubyte val, GDWState &warperState) { return drawFunction<ubyte>(x, y, val, warperState, &settings); });
        break;
      case CDF_SHORT:
        genericDataWarper.render<short>(&warper, sourceData, &sourceGeo, &destGeo,
                                        [&settings](int x, int y, short val, GDWState &warperState) { return drawFunction<short>(x, y, val, warperState, &settings); });
        break;
      case CDF_USHORT:
        genericDataWarper.render<ushort>(&warper, sourceData, &sourceGeo, &destGeo,
                                         [&settings](int x, int y, ushort val, GDWState &warperState) { return drawFunction<ushort>(x, y, val, warperState, &settings); });
        break;
      case CDF_INT:
        genericDataWarper.render<int>(&warper, sourceData, &sourceGeo, &destGeo,
                                      [&settings](int x, int y, int val, GDWState &warperState) { return drawFunction<int>(x, y, val, warperState, &settings); });
        break;
      case CDF_UINT:
        genericDataWarper.render<uint>(&warper, sourceData, &sourceGeo, &destGeo,
                                       [&settings](int x, int y, uint val, GDWState &warperState) { return drawFunction<uint>(x, y, val, warperState, &settings); });
        break;
      case CDF_FLOAT:
        genericDataWarper.render<float>(&warper, sourceData, &sourceGeo, &destGeo,
                                        [&settings](int x, int y, float val, GDWState &warperState) { return drawFunction<float>(x, y, val, warperState, &settings); });
        break;
      case CDF_DOUBLE:
        genericDataWarper.render<double>(&warper, sourceData, &sourceGeo, &destGeo,
                                         [&settings](int x, int y, double val, GDWState &warperState) { return drawFunction<double>(x, y, val, warperState, &settings); });
        break;
      }
    }

    reader.close();
    // CDBDebug("CLOSING TEMPORAL FULL READER");
    delete dataSourceToInclude;
  }
  return 0;
}
