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

#include <algorithm>
#include <vector>
#include <string>
#include "CXMLGen.h"
#include "CDBFactory.h"
#include "LayerTypeLiveUpdate/LayerTypeLiveUpdate.h"
#include <json_adaguc.h>
#include "utils/LayerMetadataStore.h"
#include "utils/XMLGenUtils.h"
#include "utils/CXMLTemplates.h"
#include "utils/LayerUtils.h"

// #define CXMLGEN_DEBUG
// #define MEASURE_TIME

const char *CXMLGen::className = "CXMLGen";
int CXMLGen::WCSDescribeCoverage(CServerParams *srvParam, CT::string *XMLDocument) { return OGCGetCapabilities(srvParam, XMLDocument); }

const MetadataLayer *getFirstLayerWithoutError(std::vector<MetadataLayer *> *myWMSLayerList) {
  if (myWMSLayerList->size() == 0) {
    return nullptr;
  }
  for (size_t lnr = 0; lnr < myWMSLayerList->size(); lnr++) {
    MetadataLayer *layer = (*myWMSLayerList)[lnr];
    if (layer->hasError == 0) {
      return layer;
    }
  }
  return (*myWMSLayerList)[0];
}

void addErrorInXMLForMisconfiguredLayer(CT::string *XMLDoc, MetadataLayer *layer) { XMLDoc->printconcat("\n<!-- Note: Error: Layer [%s] is misconfigured -->\n", layer->layerMetadata.name.c_str()); }

int CXMLGen::getWMS_1_0_0_Capabilities(CT::string *XMLDoc, std::vector<MetadataLayer *> *myWMSLayerList) {
  CT::string onlineResource = srvParam->getOnlineResource();
  onlineResource.concat("SERVICE=WMS&amp;");
  XMLDoc->copy(WMS_1_0_0_GetCapabilities_Header);
  XMLDoc->replaceSelf("[SERVICETITLE]", srvParam->cfg->WMS[0]->Title[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICEABSTRACT]", srvParam->cfg->WMS[0]->Abstract[0]->value.c_str());
  XMLDoc->replaceSelf("[GLOBALLAYERTITLE]", srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICEONLINERESOURCE]", onlineResource.c_str());
  XMLDoc->replaceSelf("[SERVICEINFO]", serviceInfo.c_str());
  const auto firstWMLayer = getFirstLayerWithoutError(myWMSLayerList);
  if (firstWMLayer != nullptr) {
    for (size_t p = 0; p < firstWMLayer->layerMetadata.projectionList.size(); p++) {
      LayerMetadataProjection *proj = firstWMLayer->layerMetadata.projectionList[p];
      XMLDoc->concat("<SRS>");
      XMLDoc->concat(&proj->name);
      XMLDoc->concat("</SRS>\n");
    }

    for (size_t lnr = 0; lnr < myWMSLayerList->size(); lnr++) {
      MetadataLayer *layer = (*myWMSLayerList)[lnr];
      if (layer->hasError != 0) {
        addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
      }
      if (layer->hasError == 0) {
        XMLDoc->printconcat("<Layer queryable=\"%d\">\n", layer->layerMetadata.isQueryable);
        XMLDoc->concat("<Name>");
        XMLDoc->concat(&layer->layerMetadata.name);
        XMLDoc->concat("</Name>\n");
        CT::string layerTitle = layer->layerMetadata.title;
        layerTitle.encodeXMLSelf();
        XMLDoc->concat("<Title>");
        XMLDoc->concat(&layerTitle);
        XMLDoc->concat("</Title>\n");

        XMLDoc->concat("<SRS>");
        for (size_t p = 0; p < layer->layerMetadata.projectionList.size(); p++) {
          LayerMetadataProjection *proj = layer->layerMetadata.projectionList[p];
          XMLDoc->concat(&proj->name);
          if (p + 1 < layer->layerMetadata.projectionList.size()) XMLDoc->concat(" ");
        }
        XMLDoc->concat("</SRS>\n");
        XMLDoc->printconcat("<LatLonBoundingBox minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[1],
                            layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[3]);
        // Dims
        for (size_t d = 0; d < layer->layerMetadata.dimList.size(); d++) {
          LayerMetadataDim *dim = layer->layerMetadata.dimList[d];
          if (dim->hidden) continue;
          XMLDoc->printconcat("<Dimension name=\"%s\" units=\"%s\"/>\n", dim->name.c_str(), dim->units.c_str());
          XMLDoc->printconcat("<Extent name=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\">", dim->name.c_str(), dim->defaultValue.c_str(), 1);
          XMLDoc->concat(dim->values.c_str());
          XMLDoc->concat("</Extent>\n");
        }
        XMLDoc->concat("</Layer>\n");
      } else {
        CDBError("Skipping layer %s", layer->layerMetadata.name.c_str());
      }
    }
  }
  XMLDoc->concat("    </Layer>\n  </Capability>\n</WMT_MS_Capabilities>\n");
  return 0;
}

int CXMLGen::getWMS_1_1_1_Capabilities(CT::string *XMLDoc, std::vector<MetadataLayer *> *myWMSLayerList) {
  CT::string onlineResource = srvParam->getOnlineResource();
  onlineResource.concat("SERVICE=WMS&amp;");
  XMLDoc->copy(WMS_1_1_1_GetCapabilities_Header);
  XMLDoc->replaceSelf("[SERVICETITLE]", srvParam->cfg->WMS[0]->Title[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICEABSTRACT]", srvParam->cfg->WMS[0]->Abstract[0]->value.c_str());
  XMLDoc->replaceSelf("[GLOBALLAYERTITLE]", srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICEONLINERESOURCE]", onlineResource.c_str());
  XMLDoc->replaceSelf("[SERVICEINFO]", serviceInfo.c_str());
  const auto firstWMLayer = getFirstLayerWithoutError(myWMSLayerList);
  if (firstWMLayer != nullptr) {
    for (size_t p = 0; p < firstWMLayer->layerMetadata.projectionList.size(); p++) {
      LayerMetadataProjection *proj = firstWMLayer->layerMetadata.projectionList[p];
      XMLDoc->concat("<SRS>");
      XMLDoc->concat(&proj->name);
      XMLDoc->concat("</SRS>\n");
    }

    // Make a unique list of all groups
    std::vector<std::string> groupKeys;
    for (size_t lnr = 0; lnr < myWMSLayerList->size(); lnr++) {
      MetadataLayer *layer = (*myWMSLayerList)[lnr];
      std::string key = "";
      if (layer->layerMetadata.group.length() > 0) key = layer->layerMetadata.group.c_str();
      size_t j = 0;
      for (j = 0; j < groupKeys.size(); j++) {
        if (groupKeys[j] == key) break;
      }
      if (j >= groupKeys.size()) groupKeys.push_back(key);
    }
    // Sort the groups alphabetically
    std::sort(groupKeys.begin(), groupKeys.end(), compareStringCase);

    // Loop through the groups
    int currentGroupDepth = 0;
    for (size_t groupIndex = 0; groupIndex < groupKeys.size(); groupIndex++) {
      // CDBError("group %s",groupKeys[groupIndex].c_str());
      int groupDepth = 0;

      // if(groupKeys[groupIndex].size()>0)
      {
        CT::string key = groupKeys[groupIndex].c_str();
        CT::string *subGroups = key.splitToArray("/");
        groupDepth = subGroups->count;

        if (groupIndex > 0) {
          CT::string prevKey = groupKeys[groupIndex - 1].c_str();
          CT::string *prevSubGroups = prevKey.splitToArray("/");

          for (size_t j = subGroups->count; j < prevSubGroups->count; j++) {
            // CDBError("<");
            currentGroupDepth--;
            XMLDoc->concat("</Layer>\n");
          }

          // CDBError("subGroups->count %d",subGroups->count);
          // CDBError("prevSubGroups->count %d",prevSubGroups->count);
          int removeGroups = 0;
          for (size_t j = 0; j < subGroups->count && j < prevSubGroups->count; j++) {
            // CDBError("CC %d",j);
            if (subGroups[j].equals(&prevSubGroups[j]) == false || removeGroups == 1) {
              removeGroups = 1;
              // CDBError("!=%d %s!=%s",j,subGroups[j].c_str(),prevSubGroups[j].c_str());
              // CDBError("<");
              XMLDoc->concat("</Layer>\n");
              currentGroupDepth--;
              // break;
            }
          }
          // CDBDebug("!!! %d",currentGroupDepth);
          for (size_t j = currentGroupDepth; j < subGroups->count; j++) {
            XMLDoc->concat("<Layer>\n");
            XMLDoc->concat("<Title>");
            // CDBError("> %s",subGroups[j].c_str());
            XMLDoc->concat(subGroups[j].c_str());
            XMLDoc->concat("</Title>\n");
          }

          delete[] prevSubGroups;
        } else {
          for (size_t j = 0; j < subGroups->count; j++) {
            XMLDoc->concat("<Layer>\n");
            XMLDoc->concat("<Title>");
            // CDBError("> %s grpupindex %d",subGroups[j].c_str(),groupIndex);
            XMLDoc->concat(subGroups[j].c_str());
            XMLDoc->concat("</Title>\n");
          }
        }
        delete[] subGroups;
        currentGroupDepth = groupDepth;
        // CDBDebug("currentGroupDepth = %d",currentGroupDepth);
      }

      for (size_t lnr = 0; lnr < myWMSLayerList->size(); lnr++) {
        MetadataLayer *layer = (*myWMSLayerList)[lnr];
        if (layer->layerMetadata.group.equals(groupKeys[groupIndex])) {
          // CDBError("layer %d %s",groupDepth,layer->name.c_str());
          if (layer->hasError != 0) {
            addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
          }
          if (layer->hasError == 0) {
            XMLDoc->printconcat("<Layer queryable=\"%d\" opaque=\"1\" cascaded=\"%d\">\n", layer->layerMetadata.isQueryable, layer->dataSource->dLayerType == CConfigReaderLayerTypeCascaded ? 1 : 0);
            XMLDoc->concat("<Name>");
            XMLDoc->concat(&layer->layerMetadata.name);
            XMLDoc->concat("</Name>\n");
            CT::string layerTitle = layer->layerMetadata.title;
            layerTitle.encodeXMLSelf();
            XMLDoc->concat("<Title>");
            XMLDoc->concat(&layerTitle);
            XMLDoc->concat("</Title>\n");

            for (size_t p = 0; p < layer->layerMetadata.projectionList.size(); p++) {
              LayerMetadataProjection *proj = layer->layerMetadata.projectionList[p];
              XMLDoc->concat("<SRS>");
              XMLDoc->concat(&proj->name);
              XMLDoc->concat("</SRS>\n");
              XMLDoc->printconcat("<BoundingBox SRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj->name.c_str(), proj->dfBBOX[0], proj->dfBBOX[1], proj->dfBBOX[2],
                                  proj->dfBBOX[3]);
            }

            XMLDoc->printconcat("<LatLonBoundingBox minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[1],
                                layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[3]);
            // Dims
            for (size_t d = 0; d < layer->layerMetadata.dimList.size(); d++) {
              LayerMetadataDim *dim = layer->layerMetadata.dimList[d];
              if (dim->hidden) continue;
              XMLDoc->printconcat("<Dimension name=\"%s\" units=\"%s\"/>\n", dim->name.c_str(), dim->units.c_str());
              XMLDoc->printconcat("<Extent name=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\">", dim->name.c_str(), dim->defaultValue.c_str(), 1);
              XMLDoc->concat(dim->values.c_str());
              XMLDoc->concat("</Extent>\n");
            }

            // Styles
            for (size_t s = 0; s < layer->layerMetadata.styleList.size(); s++) {
              LayerMetadataStyle *style = layer->layerMetadata.styleList[s];

              XMLDoc->concat("   <Style>");
              XMLDoc->printconcat("    <Name>%s</Name>", style->name.c_str());
              XMLDoc->printconcat("    <Title>%s</Title>", style->title.c_str());
              if (style->abstract.length() > 0) {
                XMLDoc->printconcat("    <Abstract>%s</Abstract>", style->abstract.encodeXML().c_str());
              }
              XMLDoc->printconcat("    <LegendURL width=\"%d\" height=\"%d\">", LEGEND_WIDTH, LEGEND_HEIGHT);
              XMLDoc->concat("       <Format>image/png</Format>");
              XMLDoc->printconcat("       <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" "
                                  "xlink:href=\"%s&amp;version=1.1.1&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=image/png&amp;STYLE=%s\"/>",
                                  onlineResource.c_str(), layer->layerMetadata.name.c_str(), style->name.c_str());
              XMLDoc->concat("    </LegendURL>");
              XMLDoc->concat("  </Style>");
            }

            if (layer->layer->MetadataURL.size() > 0) {
              CT::string layerMetaDataURL = firstWMLayer->layer->MetadataURL[0]->value.c_str();
              layerMetaDataURL.replaceSelf("&", "&amp;");
              XMLDoc->concat("   <MetadataURL type=\"TC211\">\n");
              XMLDoc->concat("     <Format>text/xml</Format>\n");
              XMLDoc->printconcat("     <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\"/>", layerMetaDataURL.c_str());
              XMLDoc->concat("   </MetadataURL>\n");
            }

            XMLDoc->concat("        <ScaleHint min=\"0\" max=\"10000\" />\n");
            XMLDoc->concat("</Layer>\n");
          } else {
            CDBError("Skipping layer %s", layer->layerMetadata.name.c_str());
          }
        }
      }
    }

    // CDBDebug("** %d",currentGroupDepth);
    for (int j = 0; j < currentGroupDepth; j++) {
      XMLDoc->concat("</Layer>\n");
    }
  }
  XMLDoc->concat("    </Layer>\n  </Capability>\n</WMT_MS_Capabilities>\n");
  return 0;
}

int CXMLGen::getWMS_1_3_0_Capabilities(CT::string *XMLDoc, std::vector<MetadataLayer *> *myWMSLayerList) {
  CT::string onlineResource = srvParam->getOnlineResource();
  onlineResource.concat("SERVICE=WMS&amp;");
  XMLDoc->copy(WMS_1_3_0_GetCapabilities_Header);
  XMLDoc->replaceSelf("[SERVICETITLE]", srvParam->cfg->WMS[0]->Title[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICEABSTRACT]", srvParam->cfg->WMS[0]->Abstract[0]->value.c_str());
  // XMLDoc->replaceSelf("[GLOBALLAYERTITLE]",srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICEONLINERESOURCE]", onlineResource.c_str());
  XMLDoc->replaceSelf("[SERVICEINFO]", serviceInfo.c_str());

  int useINSPIREScenario = 0; //{ 0 == default WMS service, 1 == extended inspire capabilities scenario 1, 2 == extended inspire capabilities scenario 2}

  bool inspireMetadataIsAvailable = false;
  CT::string datasetCSWURL;
  CT::string viewServiceCSWURL;
#ifdef ENABLE_INSPIRE
  CInspire::InspireMetadataFromCSW inspireMetadata;

  if (srvParam->cfg->WMS[0]->Inspire.size() == 1) {
    if (srvParam->cfg->WMS[0]->Inspire[0]->ViewServiceCSW.size() == 1) {
      if (srvParam->cfg->WMS[0]->Inspire[0]->ViewServiceCSW[0]->value.empty() == false) {
        viewServiceCSWURL = srvParam->cfg->WMS[0]->Inspire[0]->ViewServiceCSW[0]->value.c_str();
        viewServiceCSWURL.replaceSelf("&", "&amp;");
      }
    }
    if (srvParam->cfg->WMS[0]->Inspire[0]->DatasetCSW.size() == 1) {
      if (srvParam->cfg->WMS[0]->Inspire[0]->DatasetCSW[0]->value.empty() == false) {
        datasetCSWURL = srvParam->cfg->WMS[0]->Inspire[0]->DatasetCSW[0]->value.c_str();
        datasetCSWURL.replaceSelf("&", "&amp;");
      }
    }
  }

  if (viewServiceCSWURL.length() > 0 && datasetCSWURL.length() > 0) {
    useINSPIREScenario = 1;
  }

#endif
  // Use inspire scenario 1.
  if (useINSPIREScenario > 0) {
#ifdef ENABLE_INSPIRE
    // *** Enable INSPIRE ***
    inspireMetadataIsAvailable = true;

    // Download CSW information

    try {
      CT::string URL = datasetCSWURL.c_str();
      URL.replaceSelf("&amp;", "&");
      inspireMetadata = CInspire::readInspireMetadataFromCSW(URL.c_str());
    } catch (int a) {
      CT::string URL = datasetCSWURL.c_str();

      URL.replaceSelf("&", "&amp;");
      CDBError("Unable to read from catalog service: %s, Inspire CSW Service : \"%s\"", CInspire::getErrorMessage(a).c_str(), URL.c_str());
      return 1;
    }

    /*Scenario 1*/
    if (useINSPIREScenario == 1) {
      CT::string inspirexsi = "xmlns:inspire_common=\"http://inspire.ec.europa.eu/schemas/common/1.0\"\n"
                              "xmlns:inspire_vs=\"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0\"\n"
                              "xsi:schemaLocation=\"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd\"\n";

      XMLDoc->printconcat("<inspire_vs:ExtendedCapabilities %s>\n", inspirexsi.c_str());
      XMLDoc->concat("  <inspire_common:MetadataUrl xsi:type=\"inspire_common:resourceLocatorType\">\n");
      XMLDoc->printconcat("    <inspire_common:URL>%s</inspire_common:URL>\n", viewServiceCSWURL.c_str());
      // XMLDoc->concat("    <inspire_common:MediaType>application/vnd.ogc.csw.GetRecordByIdResponse_xml</inspire_common:MediaType>\n");
      XMLDoc->concat("    <inspire_common:MediaType>application/vnd.iso.19139+xml</inspire_common:MediaType>\n");

      XMLDoc->concat("  </inspire_common:MetadataUrl>\n");
      XMLDoc->concat("  <inspire_common:SupportedLanguages xsi:type=\"inspire_common:supportedLanguagesType\">\n");
      XMLDoc->concat("    <inspire_common:DefaultLanguage>\n");
      XMLDoc->concat("      <inspire_common:Language>eng</inspire_common:Language>\n");
      XMLDoc->concat("    </inspire_common:DefaultLanguage>\n");
      XMLDoc->concat("    <inspire_common:SupportedLanguage>\n");
      XMLDoc->concat("      <inspire_common:Language>eng</inspire_common:Language>\n");
      XMLDoc->concat("    </inspire_common:SupportedLanguage>\n");
      XMLDoc->concat("  </inspire_common:SupportedLanguages>\n");
      XMLDoc->concat("  <inspire_common:ResponseLanguage>\n");
      XMLDoc->concat("    <inspire_common:Language>eng</inspire_common:Language>\n");
      XMLDoc->concat("  </inspire_common:ResponseLanguage>\n");
      XMLDoc->concat("</inspire_vs:ExtendedCapabilities>\n");
    }

    /*Scenario 2*/
    if (useINSPIREScenario == 2) {
      XMLDoc->concat("<inspire_vs:ExtendedCapabilities>\n");
      XMLDoc->concat("  <inspire_common:ResourceLocator>\n");
      XMLDoc->concat("    <inspire_common:URL></inspire_common:URL>\n");
      XMLDoc->concat("  </inspire_common:ResourceLocator>\n");
      XMLDoc->concat("  <inspire_common:ResourceType>service</inspire_common:ResourceType>\n");
      XMLDoc->concat("  <inspire_common:TemporalReference>\n");
      XMLDoc->concat("  </inspire_common:TemporalReference>\n");
      XMLDoc->concat("\n");
      XMLDoc->concat("  <inspire_common:Conformity>\n");
      XMLDoc->concat("    <inspire_common:Specification>\n");
      XMLDoc->concat("      <inspire_common:Title>D2.8.III.13-14 Data Specification on Atmospheric Conditions – Guidelines</inspire_common:Title>\n");
      XMLDoc->concat("      <inspire_common:DateOfPublication>2012-04-20</inspire_common:DateOfPublication>\n");
      XMLDoc->concat("    </inspire_common:Specification>\n");
      XMLDoc->concat("    <inspire_common:Degree>conformant</inspire_common:Degree>\n");
      XMLDoc->concat("  </inspire_common:Conformity>\n");
      XMLDoc->concat("\n");
      XMLDoc->concat("  <inspire_common:MetadataPointOfContact>\n");
      XMLDoc->concat("    <inspire_common:OrganisationName>KNMI</inspire_common:OrganisationName>\n");
      XMLDoc->concat("    <inspire_common:EmailAddress>adaguc@knmi.nl</inspire_common:EmailAddress>\n");
      XMLDoc->concat("  </inspire_common:MetadataPointOfContact>\n");
      XMLDoc->concat("\n");
      XMLDoc->concat("  <inspire_common:MetadataDate>2015-01-01</inspire_common:MetadataDate>\n");
      XMLDoc->concat("  <inspire_common:SpatialDataServiceType>view</inspire_common:SpatialDataServiceType>\n");
      XMLDoc->concat("  <inspire_common:MandatoryKeyword>\n");
      XMLDoc->concat("    <inspire_common:KeywordValue>infoMapAccessService</inspire_common:KeywordValue>\n");
      XMLDoc->concat("  </inspire_common:MandatoryKeyword>\n");
      XMLDoc->concat("\n");
      XMLDoc->concat("  <inspire_common:Keyword>\n");
      XMLDoc->concat("    <inspire_common:OriginatingControlledVocabulary>\n");
      XMLDoc->concat("      <inspire_common:Title>AC-MF Data Type</inspire_common:Title>\n");
      XMLDoc->concat("      <inspire_common:DateOfCreation>2012-04-20</inspire_common:DateOfCreation>\n");
      XMLDoc->concat("      <inspire_common:URI>urn:x-inspire:specification:DS-AC-MF:dataType</inspire_common:URI>\n");
      XMLDoc->concat("      <inspire_common:ResourceLocator>\n");
      XMLDoc->concat("        <inspire_common:URL></inspire_common:URL>\n");
      XMLDoc->concat("      </inspire_common:ResourceLocator>\n");
      XMLDoc->concat("    </inspire_common:OriginatingControlledVocabulary>\n");
      XMLDoc->concat("    <inspire_common:KeywordValue>prediction</inspire_common:KeywordValue>\n");
      XMLDoc->concat("  </inspire_common:Keyword>\n");
      XMLDoc->concat("\n");
      XMLDoc->concat("  <inspire_common:SupportedLanguages>\n");
      XMLDoc->concat("    <inspire_common:DefaultLanguage>\n");
      XMLDoc->concat("      <inspire_common:Language>eng</inspire_common:Language>\n");
      XMLDoc->concat("    </inspire_common:DefaultLanguage>\n");
      XMLDoc->concat("    <inspire_common:SupportedLanguage>\n");
      XMLDoc->concat("      <inspire_common:Language>dut</inspire_common:Language>\n");
      XMLDoc->concat("    </inspire_common:SupportedLanguage>\n");
      XMLDoc->concat("  </inspire_common:SupportedLanguages>\n");
      XMLDoc->concat("  <inspire_common:ResponseLanguage>\n");
      XMLDoc->concat("    <inspire_common:Language>eng</inspire_common:Language>\n");
      XMLDoc->concat("  </inspire_common:ResponseLanguage>\n");
      XMLDoc->concat("</inspire_vs:ExtendedCapabilities>\n");
    }

    // Set INSPIRE SCHEMA
    /*CT::string inspirexsi=
      "xmlns:inspire_common=\"http://inspire.ec.europa.eu/schemas/common/1.0\"\n"
      "xmlns:inspire_vs=\"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0\"\n"
      "xsi:schemaLocation=\"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd\"\n";
    XMLDoc->replaceSelf("[SCHEMADEFINITION]",inspirexsi.c_str());*/

    CT::string wms130xsi = "xsi:schemaLocation=\"http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd\"\n";
    XMLDoc->replaceSelf("[SCHEMADEFINITION]", wms130xsi.c_str());

    // Set INSPIRE contact information
    CT::string contactInformation = "";
    contactInformation.printconcat("    <ContactPersonPrimary>");
    contactInformation.printconcat("      <ContactPerson>%s</ContactPerson>", inspireMetadata.pointOfContact.c_str());
    contactInformation.printconcat("      <ContactOrganization>%s</ContactOrganization>", inspireMetadata.organisationName.c_str());
    contactInformation.printconcat("    </ContactPersonPrimary>");
    contactInformation.printconcat("    <ContactVoiceTelephone>%s</ContactVoiceTelephone>", inspireMetadata.voiceTelephone.c_str());
    contactInformation.printconcat("    <ContactElectronicMailAddress>%s</ContactElectronicMailAddress>", inspireMetadata.email.c_str());

    XMLDoc->replaceSelf("[CONTACTINFORMATION]", contactInformation.c_str());

    XMLDoc->replaceSelf("[INSPIRE::ABSTRACT]", inspireMetadata.abstract.c_str());
#endif
  } else {
    // Default WMS 1.3.0 service
    CT::string wms130xsi = "xsi:schemaLocation=\"http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd\"\n";
    XMLDoc->replaceSelf("[SCHEMADEFINITION]", wms130xsi.c_str());
    XMLDoc->replaceSelf("[CONTACTINFORMATION]", "");
  }

  XMLDoc->concat("<Layer>\n");
  XMLDoc->printconcat("<Title>%s</Title>\n", srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->value.c_str());

  const auto firstWMLayer = getFirstLayerWithoutError(myWMSLayerList);
  if (firstWMLayer != nullptr) {

    for (size_t p = 0; p < firstWMLayer->layerMetadata.projectionList.size(); p++) {
      LayerMetadataProjection *proj = firstWMLayer->layerMetadata.projectionList[p];
      if (!proj->name.empty()) {
        XMLDoc->concat("<CRS>");
        XMLDoc->concat(&proj->name);
        XMLDoc->concat("</CRS>\n");
      }
    }
    for (size_t p = 0; p < firstWMLayer->layerMetadata.projectionList.size(); p++) {
      LayerMetadataProjection *proj = firstWMLayer->layerMetadata.projectionList[p];
      if (!proj->name.empty()) {
        if (srvParam->checkBBOXXYOrder(proj->name.c_str()) == true) {
          XMLDoc->printconcat("<BoundingBox CRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj->name.c_str(), proj->dfBBOX[1], proj->dfBBOX[0], proj->dfBBOX[3], proj->dfBBOX[2]);
        } else {
          XMLDoc->printconcat("<BoundingBox CRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj->name.c_str(), proj->dfBBOX[0], proj->dfBBOX[1], proj->dfBBOX[2], proj->dfBBOX[3]);
        }
      }
    }

#ifdef ENABLE_INSPIRE

    if (inspireMetadataIsAvailable) {
      XMLDoc->replaceSelf("[INSPIRE::TITLE]", inspireMetadata.title.c_str());
      XMLDoc->concat("  <MetadataURL type=\"ISO19115:2005\">\n");
      XMLDoc->concat("     <Format>application/gml+xml; version=3.2</Format>\n");
      XMLDoc->printconcat("     <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>", datasetCSWURL.c_str());
      XMLDoc->concat("  </MetadataURL>\n");
    }

#endif
    // Make a unique list of all groups
    std::vector<std::string> groupKeys;
    for (size_t lnr = 0; lnr < myWMSLayerList->size(); lnr++) {
      MetadataLayer *layer = (*myWMSLayerList)[lnr];
      std::string key = "";
      if (layer->layerMetadata.group.length() > 0) key = layer->layerMetadata.group.c_str();
      size_t j = 0;
      for (j = 0; j < groupKeys.size(); j++) {
        if (groupKeys[j] == key) break;
      }
      if (j >= groupKeys.size()) groupKeys.push_back(key);
    }
    // Sort the groups alphabetically
    std::sort(groupKeys.begin(), groupKeys.end(), compareStringCase);

    // Loop through the groups
    int currentGroupDepth = 0;
    for (size_t groupIndex = 0; groupIndex < groupKeys.size(); groupIndex++) {
#ifdef CXMLGEN_DEBUG
      CDBDebug("group %s", groupKeys[groupIndex].c_str());
#endif
      // CDBError("group %s",groupKeys[groupIndex].c_str());
      int groupDepth = 0;

      // if(groupKeys[groupIndex].size()>0)
      {
        CT::string key = groupKeys[groupIndex].c_str();
        CT::string *subGroups = key.splitToArray("/");
        groupDepth = subGroups->count;

        if (groupIndex > 0) {
          CT::string prevKey = groupKeys[groupIndex - 1].c_str();
          CT::string *prevSubGroups = prevKey.splitToArray("/");

          for (size_t j = subGroups->count; j < prevSubGroups->count; j++) {
            // CDBError("<");
            currentGroupDepth--;
            XMLDoc->concat("</Layer>\n");
          }

          // CDBError("subGroups->count %d",subGroups->count);
          // CDBError("prevSubGroups->count %d",prevSubGroups->count);
          int removeGroups = 0;
          for (size_t j = 0; j < subGroups->count && j < prevSubGroups->count; j++) {
            // CDBError("CC %d",j);
            if (subGroups[j].equals(&prevSubGroups[j]) == false || removeGroups == 1) {
              removeGroups = 1;
              // CDBError("!=%d %s!=%s",j,subGroups[j].c_str(),prevSubGroups[j].c_str());
              // CDBError("<");
              XMLDoc->concat("</Layer>\n");
              currentGroupDepth--;
              // break;
            }
          }
          // CDBDebug("!!! %d",currentGroupDepth);
          for (size_t j = currentGroupDepth; j < subGroups->count; j++) {
            XMLDoc->concat("<Layer>\n");
            XMLDoc->concat("<Title>");
            // CDBError("> %s",subGroups[j].c_str());
            XMLDoc->concat(subGroups[j].c_str());
            XMLDoc->concat("</Title>\n");
          }

          delete[] prevSubGroups;
        } else {
          for (size_t j = 0; j < subGroups->count; j++) {
            XMLDoc->concat("<Layer>\n");
            XMLDoc->concat("<Title>");
            // CDBError("> %s grpupindex %d",subGroups[j].c_str(),groupIndex);
            XMLDoc->concat(subGroups[j].c_str());
            XMLDoc->concat("</Title>\n");
          }
        }
        delete[] subGroups;
        currentGroupDepth = groupDepth;
        // CDBDebug("currentGroupDepth = %d",currentGroupDepth);
      }

      for (size_t lnr = 0; lnr < myWMSLayerList->size(); lnr++) {
        MetadataLayer *layer = (*myWMSLayerList)[lnr];
#ifdef CXMLGEN_DEBUG
        CDBDebug("Comparing %s == %s", layer->group.c_str(), groupKeys[groupIndex].c_str());
#endif

        if (layer->layerMetadata.group.equals(groupKeys[groupIndex])) {
#ifdef CXMLGEN_DEBUG
          CDBDebug("layer %d %s", groupDepth, layer->name.c_str());
#endif

          if (layer->hasError != 0) {
            addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
          }
          if (layer->hasError == 0) {
            XMLDoc->printconcat("<Layer queryable=\"%d\" opaque=\"1\" cascaded=\"%d\">\n", layer->layerMetadata.isQueryable, layer->dataSource->dLayerType == CConfigReaderLayerTypeCascaded ? 1 : 0);
            XMLDoc->concat("<Name>");
            XMLDoc->concat(&layer->layerMetadata.name);
            XMLDoc->concat("</Name>\n");
            CT::string layerTitle = layer->layerMetadata.title;
            layerTitle.encodeXMLSelf();
            XMLDoc->concat("<Title>");
            XMLDoc->concat(&layerTitle);
            XMLDoc->concat("</Title>\n");
            // TODO

            if (layer->layerMetadata.abstract.length() > 0) {
              XMLDoc->concat("<Abstract>");
              XMLDoc->concat(layer->layerMetadata.abstract.encodeXML().c_str());
              XMLDoc->concat("</Abstract>\n");
            }
#ifdef ENABLE_INSPIRE
            if (inspireMetadataIsAvailable) {
              // Set INSPIRE layer keywords
              XMLDoc->concat("<KeywordList>\n");
              for (size_t j = 0; j < inspireMetadata.keywords.size(); j++) {
                XMLDoc->printconcat("<Keyword>%s</Keyword>\n", inspireMetadata.keywords[j].c_str()); // TODO
              }
              XMLDoc->concat("</KeywordList>\n");
            }
#endif
            // XMLDoc->concat("<Keyword>"); XMLDoc->concat(&layer->abstract);XMLDoc->concat("</Keyword>\n");

            /*if(layer->layerMetadata.cfgLayer->MetadataURL.size()>0){
                XMLDoc->concat("  <KeywordList><Keyword>precipitation_amount</Keyword></KeywordList>\n");
            }*/
            XMLDoc->printconcat("<EX_GeographicBoundingBox>\n"
                                "  <westBoundLongitude>%f</westBoundLongitude>\n"
                                "  <eastBoundLongitude>%f</eastBoundLongitude>\n"
                                "  <southBoundLatitude>%f</southBoundLatitude>\n"
                                "  <northBoundLatitude>%f</northBoundLatitude>\n"
                                "</EX_GeographicBoundingBox>",
                                layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[1], layer->layerMetadata.dfLatLonBBOX[3]);

            for (size_t p = 0; p < layer->layerMetadata.projectionList.size(); p++) {
              LayerMetadataProjection *proj = layer->layerMetadata.projectionList[p];

              if (srvParam->checkBBOXXYOrder(proj->name.c_str()) == true) {
                XMLDoc->printconcat("<BoundingBox CRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj->name.c_str(), proj->dfBBOX[1], proj->dfBBOX[0], proj->dfBBOX[3],
                                    proj->dfBBOX[2]);
              } else {
                XMLDoc->printconcat("<BoundingBox CRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj->name.c_str(), proj->dfBBOX[0], proj->dfBBOX[1], proj->dfBBOX[2],
                                    proj->dfBBOX[3]);
              }
            }

            if (firstWMLayer->layer->MetadataURL.size() > 0) {
              CT::string layerMetaDataURL = firstWMLayer->layer->MetadataURL[0]->value.c_str();
              layerMetaDataURL.replaceSelf("&", "&amp;");
              XMLDoc->concat("  <MetadataURL type=\"ISO19115:2005\">\n");
              XMLDoc->concat("     <Format>application/gml+xml; version=3.2</Format>\n");
              XMLDoc->printconcat("     <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>", layerMetaDataURL.c_str());
              XMLDoc->concat("  </MetadataURL>\n");
            } else if (inspireMetadataIsAvailable) {
              //               XMLDoc->concat("  <MetadataURL type=\"ISO19115:2005\">\n");
              //               XMLDoc->concat("     <Format>application/gml+xml; version=3.2</Format>\n");
              //               XMLDoc->printconcat("     <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>",datasetCSWURL.c_str());
              //               XMLDoc->concat("  </MetadataURL>\n");
            }

            // Dims
            for (size_t d = 0; d < layer->layerMetadata.dimList.size(); d++) {
              LayerMetadataDim *dim = layer->layerMetadata.dimList[d];
              if (dim->hidden) continue;
              if (dim->name.indexOf("time") != -1) {
                XMLDoc->printconcat("<Dimension name=\"%s\" units=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\" current=\"1\">", dim->name.c_str(), dim->units.c_str(),
                                    dim->defaultValue.c_str(), 1);
              } else {
                XMLDoc->printconcat("<Dimension name=\"%s\" units=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\" >", dim->name.c_str(), dim->units.c_str(), dim->defaultValue.c_str(),
                                    1);
              }
              XMLDoc->concat(dim->values.c_str());
              XMLDoc->concat("</Dimension>\n");
            }
            if (inspireMetadataIsAvailable) {
              CT::string authorityName = "unknown";
              CT::string authorityOnlineResource = "unknown";
              CT::string identifierAuthority = "unknown";
              CT::string identifierId = "unknown";
              if (srvParam->cfg->WMS[0]->Inspire.size() == 1) {
                if (srvParam->cfg->WMS[0]->Inspire[0]->AuthorityURL.size() == 1) {
                  if (!srvParam->cfg->WMS[0]->Inspire[0]->AuthorityURL[0]->attr.name.empty()) authorityName = srvParam->cfg->WMS[0]->Inspire[0]->AuthorityURL[0]->attr.name.c_str();
                  if (!srvParam->cfg->WMS[0]->Inspire[0]->AuthorityURL[0]->attr.onlineresource.empty())
                    authorityOnlineResource = srvParam->cfg->WMS[0]->Inspire[0]->AuthorityURL[0]->attr.onlineresource.c_str();
                }
                if (srvParam->cfg->WMS[0]->Inspire[0]->Identifier.size() == 1) {
                  if (!srvParam->cfg->WMS[0]->Inspire[0]->Identifier[0]->attr.authority.empty()) identifierAuthority = srvParam->cfg->WMS[0]->Inspire[0]->Identifier[0]->attr.authority.c_str();
                  if (!srvParam->cfg->WMS[0]->Inspire[0]->Identifier[0]->attr.id.empty()) identifierId = srvParam->cfg->WMS[0]->Inspire[0]->Identifier[0]->attr.id.c_str();
                }
              }
              XMLDoc->printconcat(" <AuthorityURL name=\"%s\"><OnlineResource xlink:href=\"%s\" /></AuthorityURL>\n", authorityName.c_str(), authorityOnlineResource.c_str());
              // XMLDoc->printconcat(" <Identifier authority=\"%s\">%s</Identifier>\n",identifierAuthority.c_str(),identifierId.c_str());
              XMLDoc->printconcat(" <Identifier authority=\"%s\">%s</Identifier>\n", identifierAuthority.c_str(), layer->layerMetadata.name.c_str());
            }

            // Styles
            for (size_t s = 0; s < layer->layerMetadata.styleList.size(); s++) {
              LayerMetadataStyle *style = layer->layerMetadata.styleList[s];

              XMLDoc->concat("   <Style>");
              XMLDoc->printconcat("    <Name>%s</Name>", style->name.c_str());
              XMLDoc->printconcat("    <Title>%s</Title>", style->title.c_str());
              if (style->abstract.length() > 0) {
                XMLDoc->printconcat("    <Abstract>%s</Abstract>", style->abstract.c_str());
              }
              XMLDoc->printconcat("    <LegendURL width=\"%d\" height=\"%d\">", LEGEND_WIDTH, LEGEND_HEIGHT);
              XMLDoc->concat("       <Format>image/png</Format>");
              XMLDoc->printconcat(
                  "       <OnlineResource xlink:type=\"simple\" xlink:href=\"%s&amp;version=1.1.1&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=image/png&amp;STYLE=%s\"/>",
                  onlineResource.c_str(), layer->layerMetadata.name.c_str(), style->name.c_str());
              XMLDoc->concat("    </LegendURL>");
              XMLDoc->concat("  </Style>");
            }

            XMLDoc->concat("</Layer>\n");
          } else {
            CDBError("Skipping layer %s", layer->layerMetadata.name.c_str());
          }
        }
      }
    }

    // CDBDebug("** %d",currentGroupDepth);
    for (int j = 0; j < currentGroupDepth; j++) {
      XMLDoc->concat("</Layer>\n");
    }
  }
  XMLDoc->concat("    </Layer>\n  </Capability>\n</WMS_Capabilities>\n");
  return 0;
}

int CXMLGen::getWCS_1_0_0_Capabilities(CT::string *XMLDoc, std::vector<MetadataLayer *> *myWMSLayerList) {
  CT::string onlineResource = srvParam->getOnlineResource();
  onlineResource.concat("SERVICE=WCS&amp;");

  XMLDoc->copy(WCS_1_0_0_GetCapabilities_Header);
  if (srvParam->cfg->WCS[0]->Title.size() == 0) {
    CDBError("No title defined for WCS");
    return 1;
  }
  if (srvParam->cfg->WCS[0]->Name.size() == 0) {
    srvParam->cfg->WCS[0]->Name.push_back(new CServerConfig::XMLE_Name());
    srvParam->cfg->WCS[0]->Name[0]->value.copy(srvParam->cfg->WCS[0]->Title[0]->value.c_str());
  }
  if (srvParam->cfg->WCS[0]->Abstract.size() == 0) {
    srvParam->cfg->WCS[0]->Abstract.push_back(new CServerConfig::XMLE_Abstract());
    srvParam->cfg->WCS[0]->Abstract[0]->value.copy(srvParam->cfg->WCS[0]->Title[0]->value.c_str());
  }
  XMLDoc->replaceSelf("[SERVICENAME]", srvParam->cfg->WCS[0]->Title[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICETITLE]", srvParam->cfg->WCS[0]->Name[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICEABSTRACT]", srvParam->cfg->WCS[0]->Abstract[0]->value.c_str());
  XMLDoc->replaceSelf("[SERVICEONLINERESOURCE]", onlineResource.c_str());
  XMLDoc->replaceSelf("[SERVICEINFO]", serviceInfo.c_str());

  if (myWMSLayerList->size() > 0) {

    for (size_t lnr = 0; lnr < myWMSLayerList->size(); lnr++) {
      MetadataLayer *layer = (*myWMSLayerList)[lnr];
      if (layer->hasError != 0) {
        addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
      }
      if (layer->hasError == 0) {
        XMLDoc->printconcat("<CoverageOfferingBrief>\n");
        XMLDoc->concat("<description>");
        XMLDoc->concat(&layer->layerMetadata.name);
        XMLDoc->concat("</description>\n");
        XMLDoc->concat("<name>");
        XMLDoc->concat(&layer->layerMetadata.name);
        XMLDoc->concat("</name>\n");
        CT::string layerTitle = layer->layerMetadata.title;
        layerTitle.encodeXMLSelf();
        XMLDoc->concat("<label>");
        XMLDoc->concat(&layerTitle);
        XMLDoc->concat("</label>\n");
        XMLDoc->printconcat("  <lonLatEnvelope srsName=\"urn:ogc:def:crs:OGC:1.3:CRS84\">\n"
                            "    <gml:pos>%f %f</gml:pos>\n"
                            "    <gml:pos>%f %f</gml:pos>\n",
                            layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[1], layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[3]);

        XMLDoc->printconcat("</lonLatEnvelope>\n");
        XMLDoc->printconcat("</CoverageOfferingBrief>\n");

      } else {
        CDBError("Skipping layer %s", layer->layerMetadata.name.c_str());
      }
    }
  }
  XMLDoc->concat("</ContentMetadata>\n</WCS_Capabilities>\n");
  return 0;
}

int CXMLGen::getWCS_1_0_0_DescribeCoverage(CT::string *XMLDoc, std::vector<MetadataLayer *> *myWMSLayerList) {

  XMLDoc->copy("<?xml version='1.0' encoding=\"ISO-8859-1\" ?>\n"
               "<CoverageDescription\n"
               "   version=\"1.0.0\" \n"
               "   updateSequence=\"0\" \n"
               "   xmlns=\"http://www.opengis.net/wcs\" \n"
               "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n"
               "   xmlns:gml=\"http://www.opengis.net/gml\" \n"
               "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
               "   xsi:schemaLocation=\"http://www.opengis.net/wcs http://schemas.opengis.net/wcs/1.0.0/describeCoverage.xsd\">\n");
  const auto firstWMLayer = getFirstLayerWithoutError(myWMSLayerList);
  if (firstWMLayer != nullptr) {
    for (size_t layerIndex = 0; layerIndex < (unsigned)srvParam->WMSLayers->count; layerIndex++) {
      for (size_t lnr = 0; lnr < myWMSLayerList->size(); lnr++) {
        MetadataLayer *layer = (*myWMSLayerList)[lnr];
        if (layer->layerMetadata.name.equals(&srvParam->WMSLayers[layerIndex])) {
          if (layer->hasError != 0) {
            addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
          }
          if (layer->hasError == 0) {

            // Look wether and which dimension is a time dimension
            int timeDimIndex = -1;
            for (size_t d = 0; d < layer->layerMetadata.dimList.size(); d++) {
              LayerMetadataDim *dim = layer->layerMetadata.dimList[d];
              // if(dim->hasMultipleValues==0){
              if (dim->units.equals("ISO8601")) {
                timeDimIndex = d;
              }
            }

            if (srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
              // XMLDoc->print("<?xml version='1.0' encoding=\"ISO-8859-1\" ?>\n");
              CT::string layerTitle = layer->layerMetadata.title;
              layerTitle.encodeXMLSelf();
              XMLDoc->printconcat("  <CoverageOffering>\n"
                                  "  <description>%s</description>\n"
                                  "  <name>%s</name>\n"
                                  "  <label>%s</label>\n",
                                  layer->layerMetadata.name.c_str(), layer->layerMetadata.name.c_str(), layerTitle.c_str());
              if (layer->layerMetadata.variableList.size() > 0) {
                XMLDoc->printconcat("  <uom>%s</uom>\n", layer->layerMetadata.variableList[0]->units.c_str());
              }
              XMLDoc->printconcat("  <lonLatEnvelope srsName=\"urn:ogc:def:crs:OGC:1.3:CRS84\">\n"
                                  "    <gml:pos>%f %f</gml:pos>\n"
                                  "    <gml:pos>%f %f</gml:pos>\n",
                                  layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[1], layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[3]);

              if (timeDimIndex >= 0) {
                // For information about this, visit http://www.galdosinc.com/archives/151
                CT::string *timeDimSplit = layer->layerMetadata.dimList[timeDimIndex]->values.splitToArray("/");
                if (timeDimSplit->count == 3) {
                  XMLDoc->concat("        <gml:TimePeriod>\n");
                  XMLDoc->printconcat("          <gml:begin>%s</gml:begin>\n", timeDimSplit[0].c_str());
                  XMLDoc->printconcat("          <gml:end>%s</gml:end>\n", timeDimSplit[1].c_str());
                  XMLDoc->printconcat("          <gml:duration>%s</gml:duration>\n", timeDimSplit[2].c_str());
                  XMLDoc->concat("        </gml:TimePeriod>\n");
                }
                delete[] timeDimSplit;
              }
              XMLDoc->concat("  </lonLatEnvelope>\n"
                             "  <domainSet>\n"
                             "    <spatialDomain>\n");
              for (size_t p = 0; p < layer->layerMetadata.projectionList.size(); p++) {
                LayerMetadataProjection *proj = layer->layerMetadata.projectionList[p];
                CT::string encodedProjString(proj->name.c_str());
                // encodedProjString.encodeURLSelf();

                XMLDoc->printconcat("        <gml:Envelope srsName=\"%s\">\n"
                                    "          <gml:pos>%f %f</gml:pos>\n"
                                    "          <gml:pos>%f %f</gml:pos>\n"
                                    "        </gml:Envelope>\n",
                                    encodedProjString.c_str(), proj->dfBBOX[0], proj->dfBBOX[1], proj->dfBBOX[2], proj->dfBBOX[3]);
              }
              int width = layer->layerMetadata.width - 1;
              int height = layer->layerMetadata.height - 1;
              if (width <= 1) {
                width = 999;
              }
              if (height <= 1) {
                height = 999;
              }
              XMLDoc->printconcat("        <gml:RectifiedGrid dimension=\"2\">\n"
                                  "          <gml:limits>\n"
                                  "            <gml:GridEnvelope>\n"
                                  "              <gml:low>0 0</gml:low>\n"
                                  "              <gml:high>%d %d</gml:high>\n"
                                  "            </gml:GridEnvelope>\n"
                                  "          </gml:limits>\n"
                                  "          <gml:axisName>x</gml:axisName>\n"
                                  "          <gml:axisName>y</gml:axisName>\n"
                                  "          <gml:origin>\n"
                                  "            <gml:pos>%f %f</gml:pos>\n"
                                  "          </gml:origin>\n"
                                  "          <gml:offsetVector>%f 0</gml:offsetVector>\n"
                                  "          <gml:offsetVector>0 %f</gml:offsetVector>\n"
                                  "        </gml:RectifiedGrid>\n"
                                  "      </spatialDomain>\n",
                                  width, height,
                                  layer->layerMetadata.dfBBOX[0], //+layer->layerMetadata.dfCellSizeX/2,
                                  layer->layerMetadata.dfBBOX[3], //+layer->layerMetadata.dfCellSizeY/2,
                                  layer->layerMetadata.cellsizeX, layer->layerMetadata.cellsizeY);

              if (timeDimIndex >= 0) {
                XMLDoc->concat("      <temporalDomain>\n");
                if (layer->layerMetadata.dimList[timeDimIndex]->hasMultipleValues == 0) {
                  CT::string *timeDimSplit = layer->layerMetadata.dimList[timeDimIndex]->values.splitToArray("/");
                  if (timeDimSplit->count == 3) {
                    XMLDoc->concat("        <gml:TimePeriod>\n");
                    XMLDoc->printconcat("          <gml:begin>%s</gml:begin>\n", timeDimSplit[0].c_str());
                    XMLDoc->printconcat("          <gml:end>%s</gml:end>\n", timeDimSplit[1].c_str());
                    XMLDoc->printconcat("          <gml:duration>%s</gml:duration>\n", timeDimSplit[2].c_str());
                    XMLDoc->concat("        </gml:TimePeriod>\n");
                  }
                  delete[] timeDimSplit;
                } else {

                  CT::string *positions = layer->layerMetadata.dimList[timeDimIndex]->values.splitToArray(",");
                  for (size_t p = 0; p < positions->count; p++) {
                    XMLDoc->printconcat("        <gml:timePosition>%s</gml:timePosition>\n", (positions[p]).c_str());
                  }
                  delete[] positions;
                }
                XMLDoc->concat("      </temporalDomain>\n");
              }
              XMLDoc->concat("    </domainSet>\n"
                             "    <rangeSet>\n"
                             "      <RangeSet>\n"
                             "        <name>bands</name>\n"
                             "        <label>bands</label>\n"
                             "        <axisDescription>\n"
                             "          <AxisDescription>\n"
                             "            <name>bands</name>\n"
                             "            <label>Bands/Channels/Samples</label>\n"
                             "            <values>\n"
                             "              <singleValue>1</singleValue>\n"
                             "            </values>\n"
                             "          </AxisDescription>\n"
                             "        </axisDescription>\n"
                             "      </RangeSet>\n"
                             "    </rangeSet>\n");
              // Supported CRSs
              XMLDoc->concat("    <supportedCRSs>\n");

              for (size_t p = 0; p < layer->layerMetadata.projectionList.size(); p++) {
                LayerMetadataProjection *proj = firstWMLayer->layerMetadata.projectionList[p];
                CT::string encodedProjString(proj->name.c_str());

                XMLDoc->printconcat("      <requestResponseCRSs>%s</requestResponseCRSs>\n", encodedProjString.c_str());
              }

              CT::string prettyCRS = layer->layerMetadata.nativeEPSG.c_str();
              XMLDoc->printconcat("      <nativeCRSs>%s</nativeCRSs>\n    </supportedCRSs>\n", prettyCRS.c_str());

              XMLDoc->concat("    <supportedFormats nativeFormat=\"NetCDF4\">\n"
                             "      <formats>GeoTIFF</formats>\n"
                             "      <formats>AAIGRID</formats>\n");

              for (size_t p = 0; p < srvParam->cfg->WCS[0]->WCSFormat.size(); p++) {
                XMLDoc->printconcat("      <formats>%s</formats>\n", srvParam->cfg->WCS[0]->WCSFormat[p]->attr.name.c_str());
              }
              XMLDoc->concat("    </supportedFormats>\n");
              XMLDoc->printconcat("    <supportedInterpolations default=\"nearest neighbor\">\n"
                                  "      <interpolationMethod>nearest neighbor</interpolationMethod>\n"
                                  //     "      <interpolationMethod>bilinear</interpolationMethod>\n"
                                  "    </supportedInterpolations>\n");
              XMLDoc->printconcat("</CoverageOffering>\n");
            }
          }
        }
      }
    }
  }
  XMLDoc->concat("</CoverageDescription>\n");

  return 0;
}

int CXMLGen::OGCGetCapabilities(CServerParams *_srvParam, CT::string *XMLDocument) {

  this->srvParam = _srvParam;

  int status = 0;
  std::vector<MetadataLayer *> myWMSLayerList;

  for (size_t j = 0; j < srvParam->cfg->Layer.size(); j++) {
    if (srvParam->cfg->Layer[j]->attr.type.equals("autoscan")) {
      continue;
    }
    if (srvParam->cfg->Layer[j]->attr.hidden.equals("true")) {
      continue;
    }
    // Create a new layer and push it in the list
    MetadataLayer *metadataLayer = new MetadataLayer();
    myWMSLayerList.push_back(metadataLayer);
    metadataLayer->layer = srvParam->cfg->Layer[j];
    metadataLayer->srvParams = srvParam;
    populateLayerMetadataStruct(metadataLayer, true);
  }

#ifdef CXMLGEN_DEBUG
  if (myWMSLayerList.size() > 0) {
    CT::string finalLayerList;
    finalLayerList = myWMSLayerList[0]->name.c_str();
    for (size_t j = 1; j < myWMSLayerList.size(); j++) {
      finalLayerList.printconcat(",%s", myWMSLayerList[j]->name.c_str());
    }
    CDBDebug("Final layerlist: \"%s\"", finalLayerList.c_str());
  }

#endif

  serviceInfo.print("ADAGUCServer version %s, of %s %s", ADAGUCSERVER_VERSION, __DATE__, __TIME__);
  // Generate an XML document on basis of the information gathered above.
  CT::string XMLDoc;
  status = 0;
  if (srvParam->requestType == REQUEST_WMS_GETCAPABILITIES) {
    if (srvParam->OGCVersion == WMS_VERSION_1_0_0) {
      status = getWMS_1_0_0_Capabilities(&XMLDoc, &myWMSLayerList);
    }
    if (srvParam->OGCVersion == WMS_VERSION_1_1_1) {
      status = getWMS_1_1_1_Capabilities(&XMLDoc, &myWMSLayerList);
    }
    if (srvParam->OGCVersion == WMS_VERSION_1_3_0) {
      status = getWMS_1_3_0_Capabilities(&XMLDoc, &myWMSLayerList);
    }
  }
  try {
    if (srvParam->requestType == REQUEST_WCS_GETCAPABILITIES) {
#ifndef ADAGUC_USE_GDAL
      CServerParams::showWCSNotEnabledErrorMessage();
      throw(__LINE__);
#else
      status = getWCS_1_0_0_Capabilities(&XMLDoc, &myWMSLayerList);
#endif
    }

    if (srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
#ifndef ADAGUC_USE_GDAL
      CServerParams::showWCSNotEnabledErrorMessage();
      throw(__LINE__);
#else
      status = getWCS_1_0_0_DescribeCoverage(&XMLDoc, &myWMSLayerList);
#endif
    }
  } catch (int e) {
    status = 1;
  }

  bool errorsHaveOccured = false;

  for (size_t j = 0; j < myWMSLayerList.size(); j++) {
    if (myWMSLayerList[j]->hasError) errorsHaveOccured = true;
    delete myWMSLayerList[j];
    myWMSLayerList[j] = NULL;
  }

  if (status != 0) {
    CDBError("XML geneneration failed, please check logs. ");
    return CXMLGEN_FATAL_ERROR_OCCURED;
  }
  XMLDocument->concat(&XMLDoc);

  resetErrors();

  if (errorsHaveOccured) return CXML_NON_FATAL_ERRORS_OCCURED;
  return 0;
}
