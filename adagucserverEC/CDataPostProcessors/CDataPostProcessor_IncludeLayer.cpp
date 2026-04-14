#include <ranges>
#include "CDataPostProcessor_IncludeLayer.h"
#include "CRequest.h"
#include <utils/LayerUtils.h>
/************************/
/*CDPPIncludeLayer */
/************************/

const char *CDPPIncludeLayer::getId() { return "include_layer"; }
int CDPPIncludeLayer::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals("include_layer")) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

CDataSource *getDataSource(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource) {
  CDataSource *dataSourceToInclude = new CDataSource();
  CT::string additionalLayerName = proc->attr.name.c_str();
  size_t additionalLayerNo = 0;
  for (size_t j = 0; j < dataSource->srvParams->cfg->Layer.size(); j++) {
    CT::string layerName = makeUniqueLayerName(dataSource->srvParams->cfg->Layer[j]);
    // CDBDebug("comparing for additionallayer %s==%s", additionalLayerName.c_str(), layerName.c_str());
    if (additionalLayerName.equals(layerName)) {
      additionalLayerNo = j;
      break;
    }
  }
  dataSourceToInclude->setCFGLayer(dataSource->srvParams, dataSource->srvParams->cfg->Layer[additionalLayerNo], 0);
  return dataSourceToInclude;
}

int CDPPIncludeLayer::setDimsForNewDataSource(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, CDataSource *dataSourceToInclude) {
  CT::string additionalLayerName = proc->attr.name.c_str();
  bool dataIsFound = false;
  try {
    if (CRequest::setDimValuesForDataSource(dataSourceToInclude, dataSource->srvParams) == 0) {
      dataIsFound = true;
    }
  } catch (ServiceExceptionType e) {
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

    for (const auto &dataObject: dataSource->dataObjects) {
      if (dataObject.dataObjectName == proc->attr.name.c_str()) { // TODO SHould think of another identifier
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
    reader.enableObjectCache = true;
    status = reader.open(dataSourceToInclude, CNETCDFREADER_MODE_OPEN_HEADER); // Only read metadata
    if (status != 0) {
      CDBDebug("Can't open file %s for layer %s", dataSourceToInclude->getFileName().c_str(), proc->attr.name.c_str());
      delete dataSourceToInclude;
      return 1;
    }

    for (size_t dataObjectNr = 0; dataObjectNr < dataSource->getNumDataObjects(); dataObjectNr++) {
      if (dataSource->getDataObject(dataObjectNr)->cdfVariable->name.equals(dataSourceToInclude->getDataObject(0)->cdfVariable->name)) {
        CDBDebug("Probably already done");
        reader.close();
        delete dataSourceToInclude;

        return 0;
      }
    }
    if (dataSource->dataObjects.size() == 0) {
      CDBError("datasource has no dataobjects");
      delete dataSourceToInclude;
      return 1;
    }
    auto baseCDFObject = dataSource->dataObjects[0].cdfVariable->getParentCDFObject();
    int appendOrPrepend = proc->attr.mode.equals("prepend"); // 0:append, 1:prepend

    for (const auto &dataObjectToInClude: dataSourceToInclude->dataObjects) {
      if (dataObjectToInClude.cdfVariable == NULL) {
        CDBError("Variable not found");
        delete dataSourceToInclude;
        return 1;
      }
      DataObject newDataObject;
      newDataObject.variableName = dataObjectToInClude.cdfVariable->name.c_str();
      newDataObject.dataObjectName = proc->attr.name;
      newDataObject.cdfVariable = new CDF::Variable();
      CT::string text;
      text.print("{\"variable\":\"%s\",\"datapostproc\":\"%s\"}", dataObjectToInClude.cdfVariable->name.c_str(), this->getId());
      newDataObject.cdfObject = baseCDFObject; //(CDFObject*)varToClone->getParentCDFObject();
      baseCDFObject->addVariable(newDataObject.cdfVariable);
      newDataObject.cdfVariable->setName(dataObjectToInClude.cdfVariable->name.c_str());
      newDataObject.cdfVariable->setType(dataObjectToInClude.cdfVariable->getType());
      newDataObject.cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);
      newDataObject.cdfVariable->dimensionlinks = dataObjectToInClude.cdfVariable->dimensionlinks;
      for (size_t j = 0; j < dataObjectToInClude.cdfVariable->attributes.size(); j++) {
        newDataObject.cdfVariable->attributes.push_back(new CDF::Attribute(dataObjectToInClude.cdfVariable->attributes[j]));
      }
      newDataObject.cdfVariable->removeAttribute("ADAGUC_DATAOBJECTID");
      newDataObject.cdfVariable->setAttributeText("ADAGUC_DATAOBJECTID", text.c_str());

      newDataObject.cdfVariable->removeAttribute("scale_factor");
      newDataObject.cdfVariable->removeAttribute("add_offset");
      newDataObject.cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

      if (appendOrPrepend == 0) {
        dataSource->dataObjects.push_back(newDataObject);
      } else {
        dataSource->dataObjects.insert(dataSource->dataObjects.begin(), newDataObject);
      }
    }
    reader.close();
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
    reader.enableObjectCache = true;
    //    CDBDebug("Opening %s",dataSourceToInclude->getFileName());
    status = reader.open(dataSourceToInclude, CNETCDFREADER_MODE_OPEN_ALL); // Now open the data as well.
    if (status != 0) {
      CDBDebug("Can't open file %s for layer %s", dataSourceToInclude->getFileName().c_str(), proc->attr.name.c_str());
      delete dataSourceToInclude;
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

      GeoParameters sourceGeo;
      sourceGeo.width = dataSourceToInclude->dWidth;
      sourceGeo.height = dataSourceToInclude->dHeight;
      sourceGeo.bbox = dataSourceToInclude->dfBBOX;
      sourceGeo.cellsizeX = dataSourceToInclude->dfCellSizeX;
      sourceGeo.cellsizeY = dataSourceToInclude->dfCellSizeY;
      sourceGeo.crs = dataSourceToInclude->nativeProj4;

      GeoParameters destGeo;
      destGeo.width = dataSource->dWidth;
      destGeo.height = dataSource->dHeight;
      destGeo.bbox = dataSource->dfBBOX;
      destGeo.cellsizeX = dataSource->dfCellSizeX;
      destGeo.cellsizeY = dataSource->dfCellSizeY;
      destGeo.crs = dataSource->nativeProj4;

      CImageWarper warper;

      status = warper.initreproj(dataSourceToInclude, destGeo, &dataSource->srvParams->cfg->Projection);
      if (status != 0) {
        CDBError("Unable to initialize projection");
        return 1;
      }

      auto dataType = varToWriteTo->getType();

      GenericDataWarper genericDataWarper;
      GDWArgs args = {.warper = &warper, .sourceData = sourceData, .sourceGeoParams = sourceGeo, .destGeoParams = destGeo};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return drawFunction(x, y, val, warperState, settings); });
      ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
    }

    reader.close();
    // CDBDebug("CLOSING TEMPORAL FULL READER");
    delete dataSourceToInclude;
  }
  return 0;
}
