/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2026-09-10
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
#include <sstream>
#include <string>
#include "CXMLGen.h"
#include "CDBFactory.h"
#include "LayerTypeLiveUpdate/LayerTypeLiveUpdate.h"
#include "timeutils.h"
#include <json_adaguc.h>
#include "utils/LayerMetadataStore.h"
#include "utils/XMLGenUtils.h"
#include "utils/CXMLTemplates.h"
#include "utils/LayerUtils.h"

static const bool CXMLGEN_DEBUG = false;

int CXMLGen::WCSDescribeCoverage(CServerParams *srvParam, std::string &XMLDocument) { return OGCGetCapabilities(srvParam, XMLDocument); }

const MetadataLayer *getFirstLayerWithoutError(const std::vector<MetadataLayer *> &metadataLayerList) {
  if (metadataLayerList.size() == 0) {
    return nullptr;
  }
  for (size_t lnr = 0; lnr < metadataLayerList.size(); lnr++) {
    MetadataLayer *layer = metadataLayerList[lnr];
    if (layer->hasError == 0) {
      return layer;
    }
  }
  return metadataLayerList[0];
}

void addErrorInXMLForMisconfiguredLayer(std::string &XMLDoc, MetadataLayer *layer) {
  CT::printfconcat(XMLDoc, "\n<!-- Note: Error: Layer [%s] is misconfigured -->\n", layer->layerMetadata.name.c_str());
}

int CXMLGen::getWMS_1_0_0_Capabilities(std::string &XMLDoc, const std::vector<MetadataLayer *> &metadataLayerList) {
  std::string onlineResource = srvParam->getOnlineResource();
  onlineResource += "SERVICE=WMS&amp;";
  XMLDoc = (WMS_1_0_0_GetCapabilities_Header);
  CT::replaceSelf(XMLDoc, "[SERVICETITLE]", srvParam->cfg->WMS[0]->Title[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEABSTRACT]", srvParam->cfg->WMS[0]->Abstract[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[GLOBALLAYERTITLE]", srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEONLINERESOURCE]", onlineResource.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEINFO]", serviceInfo.c_str());
  const auto firstWMLayer = getFirstLayerWithoutError(metadataLayerList);
  if (firstWMLayer != nullptr) {
    for (auto projection: firstWMLayer->layerMetadata.projectionList) {
      XMLDoc += "<SRS>";
      XMLDoc += projection.name;
      XMLDoc += "</SRS>\n";
    }

    for (size_t lnr = 0; lnr < metadataLayerList.size(); lnr++) {
      MetadataLayer *layer = metadataLayerList[lnr];
      if (layer->hasError != 0) {
        addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
      }
      if (layer->hasError == 0) {
        CT::printfconcat(XMLDoc, "<Layer queryable=\"%d\">\n", layer->layerMetadata.isQueryable);
        XMLDoc += "<Name>";
        XMLDoc += layer->layerMetadata.name;
        XMLDoc += "</Name>\n";
        std::string layerTitle = layer->layerMetadata.title;
        layerTitle = CT::encodeXml(layerTitle);
        XMLDoc += "<Title>";
        XMLDoc += layerTitle;
        XMLDoc += "</Title>\n";

        XMLDoc += "<SRS>";
        for (size_t p = 0; p < layer->layerMetadata.projectionList.size(); p++) {
          XMLDoc += layer->layerMetadata.projectionList[p].name;
          if (p + 1 < layer->layerMetadata.projectionList.size()) XMLDoc += " ";
        }
        XMLDoc += "</SRS>\n";
        CT::printfconcat(XMLDoc, "<LatLonBoundingBox minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[1],
                         layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[3]);
        // Dims
        for (auto dim: layer->layerMetadata.dimList) {
          if (dim.hidden) continue;
          CT::printfconcat(XMLDoc, "<Dimension name=\"%s\" units=\"%s\"/>\n", dim.serviceName.c_str(), dim.units.c_str());
          CT::printfconcat(XMLDoc, "<Extent name=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\">", dim.serviceName.c_str(), dim.defaultValue.c_str(), 1);
          XMLDoc += dim.values.c_str();
          XMLDoc += "</Extent>\n";
        }
        XMLDoc += "</Layer>\n";
      } else {
        CDBError("Skipping layer %s", layer->layerMetadata.name.c_str());
      }
    }
  }
  XMLDoc += "    </Layer>\n  </Capability>\n</WMT_MS_Capabilities>\n";
  return 0;
}

int CXMLGen::getWMS_1_1_1_Capabilities(std::string &XMLDoc, const std::vector<MetadataLayer *> &metadataLayerList) {
  std::string onlineResource = srvParam->getOnlineResource();
  onlineResource += "SERVICE=WMS&amp;";
  XMLDoc = (WMS_1_1_1_GetCapabilities_Header);
  CT::replaceSelf(XMLDoc, "[SERVICETITLE]", srvParam->cfg->WMS[0]->Title[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEABSTRACT]", srvParam->cfg->WMS[0]->Abstract[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[GLOBALLAYERTITLE]", srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEONLINERESOURCE]", onlineResource.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEINFO]", serviceInfo.c_str());
  const auto firstWMLayer = getFirstLayerWithoutError(metadataLayerList);
  if (firstWMLayer != nullptr) {
    for (auto proj: firstWMLayer->layerMetadata.projectionList) {
      XMLDoc += "<SRS>";
      XMLDoc += proj.name;
      XMLDoc += "</SRS>\n";
    }

    // Make a unique list of all groups
    std::vector<std::string> groupKeys;
    for (size_t lnr = 0; lnr < metadataLayerList.size(); lnr++) {
      MetadataLayer *layer = metadataLayerList[lnr];
      std::string key = "";
      if (layer->layerMetadata.wmsgroup.length() > 0) key = layer->layerMetadata.wmsgroup.c_str();
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
      int groupDepth = 0;

      {
        std::string key = groupKeys[groupIndex];
        auto subGroups = CT::split(key, "/");
        groupDepth = subGroups.size();

        if (groupIndex > 0) {
          std::string prevKey = groupKeys[groupIndex - 1].c_str();
          auto prevSubGroups = CT::split(prevKey, "/");

          for (size_t j = subGroups.size(); j < prevSubGroups.size(); j++) {
            currentGroupDepth--;
            XMLDoc += "</Layer>\n";
          }

          int removeGroups = 0;
          for (size_t j = 0; j < subGroups.size() && j < prevSubGroups.size(); j++) {
            if (subGroups[j] != prevSubGroups[j] || removeGroups == 1) {
              removeGroups = 1;
              XMLDoc += "</Layer>\n";
              currentGroupDepth--;
            }
          }
          for (size_t j = currentGroupDepth; j < subGroups.size(); j++) {
            XMLDoc += "<Layer>\n";
            XMLDoc += "<Title>";
            XMLDoc += subGroups[j].c_str();
            XMLDoc += "</Title>\n";
          }

        } else {
          for (size_t j = 0; j < subGroups.size(); j++) {
            XMLDoc += "<Layer>\n";
            XMLDoc += "<Title>";
            XMLDoc += subGroups[j].c_str();
            XMLDoc += "</Title>\n";
          }
        }
        currentGroupDepth = groupDepth;
      }

      for (size_t lnr = 0; lnr < metadataLayerList.size(); lnr++) {
        MetadataLayer *layer = metadataLayerList[lnr];
        if (layer->layerMetadata.wmsgroup == groupKeys[groupIndex]) {
          if (layer->hasError != 0) {
            addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
          }
          if (layer->hasError == 0) {
            CT::printfconcat(XMLDoc, "<Layer queryable=\"%d\" opaque=\"1\" cascaded=\"%d\">\n", layer->layerMetadata.isQueryable,
                             layer->dataSource->dLayerType == CConfigReaderLayerTypeGraticule && layer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate ? 1 : 0);
            XMLDoc += "<Name>";
            XMLDoc += layer->layerMetadata.name;
            XMLDoc += "</Name>\n";
            std::string layerTitle = layer->layerMetadata.title;
            layerTitle = CT::encodeXml(layerTitle);
            XMLDoc += "<Title>";
            XMLDoc += layerTitle;
            XMLDoc += "</Title>\n";

            for (auto proj: layer->layerMetadata.projectionList) {
              XMLDoc += "<SRS>";
              XMLDoc += proj.name;
              XMLDoc += "</SRS>\n";
              CT::printfconcat(XMLDoc, "<BoundingBox SRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj.name.c_str(), proj.dfBBOX[0], proj.dfBBOX[1], proj.dfBBOX[2],
                               proj.dfBBOX[3]);
            }

            CT::printfconcat(XMLDoc, "<LatLonBoundingBox minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[1],
                             layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[3]);
            // Dims
            for (auto dim: layer->layerMetadata.dimList) {
              if (dim.hidden) continue;
              CT::printfconcat(XMLDoc, "<Dimension name=\"%s\" units=\"%s\"/>\n", dim.serviceName.c_str(), dim.units.c_str());
              CT::printfconcat(XMLDoc, "<Extent name=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\">", dim.serviceName.c_str(), dim.defaultValue.c_str(), 1);
              XMLDoc += dim.values.c_str();
              XMLDoc += "</Extent>\n";
            }

            // Styles
            for (auto style: layer->layerMetadata.styleList) {

              XMLDoc += "   <Style>\n";
              CT::printfconcat(XMLDoc, "    <Name>%s</Name>\n", style.name.c_str());
              CT::printfconcat(XMLDoc, "    <Title>%s</Title>\n", style.title.c_str());
              if (style.abstract.length() > 0) {
                CT::printfconcat(XMLDoc, "    <Abstract>%s</Abstract>\n", CT::encodeXml(style.abstract).c_str());
              }
              CT::printfconcat(XMLDoc, "    <LegendURL width=\"%d\" height=\"%d\">\n", LEGEND_WIDTH, LEGEND_HEIGHT);
              XMLDoc += "       <Format>image/png</Format>\n";
              CT::printfconcat(XMLDoc,
                               "       <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" "
                               "xlink:href=\"%s&amp;version=1.1.1&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=image/png&amp;STYLE=%s\"/>\n",
                               onlineResource.c_str(), layer->layerMetadata.name.c_str(), style.name.c_str());
              XMLDoc += "    </LegendURL>\n";
              XMLDoc += "  </Style>\n";
            }

            if (layer->layer->MetadataURL.size() > 0) {
              std::string layerMetaDataURL = layer->layer->MetadataURL[0]->elementValue.c_str();
              CT::replaceSelf(layerMetaDataURL, "&", "&amp;");
              XMLDoc += "   <MetadataURL type=\"TC211\">\n";
              XMLDoc += "     <Format>text/xml</Format>\n";
              CT::printfconcat(XMLDoc, "     <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\"/>", layerMetaDataURL.c_str());
              XMLDoc += "   </MetadataURL>\n";
            }

            XMLDoc += "        <ScaleHint min=\"0\" max=\"10000\" />\n";
            XMLDoc += "</Layer>\n";
          } else {
            CDBError("Skipping layer %s", layer->layerMetadata.name.c_str());
          }
        }
      }
    }

    for (int j = 0; j < currentGroupDepth; j++) {
      XMLDoc += "</Layer>\n";
    }
  }
  XMLDoc += "    </Layer>\n  </Capability>\n</WMT_MS_Capabilities>\n";
  return 0;
}

int CXMLGen::getWMS_1_3_0_Capabilities(std::string &XMLDoc, const std::vector<MetadataLayer *> &metadataLayerList) {
  std::string onlineResource = srvParam->getOnlineResource();
  onlineResource += "SERVICE=WMS&amp;";
  XMLDoc = (WMS_1_3_0_GetCapabilities_Header);
  CT::replaceSelf(XMLDoc, "[SERVICETITLE]", srvParam->cfg->WMS[0]->Title[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEABSTRACT]", srvParam->cfg->WMS[0]->Abstract[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEONLINERESOURCE]", onlineResource.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEINFO]", serviceInfo.c_str());

  int useINSPIREScenario = 0; //{ 0 == default WMS service, 1 == extended inspire capabilities scenario 1, 2 == extended inspire capabilities scenario 2}

  bool inspireMetadataIsAvailable = false;
  std::string datasetCSWURL;
  std::string viewServiceCSWURL;
#ifdef ENABLE_INSPIRE
  CInspire::InspireMetadataFromCSW inspireMetadata;

  if (srvParam->cfg->WMS[0]->Inspire.size() == 1) {
    if (srvParam->cfg->WMS[0]->Inspire[0]->ViewServiceCSW.size() == 1) {
      if (srvParam->cfg->WMS[0]->Inspire[0]->ViewServiceCSW[0]->elementValue.empty() == false) {
        viewServiceCSWURL = srvParam->cfg->WMS[0]->Inspire[0]->ViewServiceCSW[0]->elementValue.c_str();
        CT::replaceSelf(viewServiceCSWURL, "&", "&amp;");
      }
    }
    if (srvParam->cfg->WMS[0]->Inspire[0]->DatasetCSW.size() == 1) {
      if (srvParam->cfg->WMS[0]->Inspire[0]->DatasetCSW[0]->elementValue.empty() == false) {
        datasetCSWURL = srvParam->cfg->WMS[0]->Inspire[0]->DatasetCSW[0]->elementValue.c_str();
        CT::replaceSelf(datasetCSWURL, "&", "&amp;");
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
      std::string URL = datasetCSWURL;
      CT::replaceSelf(URL, "&amp;", "&");
      inspireMetadata = CInspire::readInspireMetadataFromCSW(URL.c_str());
    } catch (int a) {
      std::string URL = datasetCSWURL;

      CT::replaceSelf(URL, "&", "&amp;");
      CDBError("Unable to read from catalog service: %s, Inspire CSW Service : \"%s\"", CInspire::getErrorMessage(a).c_str(), URL.c_str());
      return 1;
    }

    /*Scenario 1*/
    if (useINSPIREScenario == 1) {
      std::string inspirexsi = "xmlns:inspire_common=\"http://inspire.ec.europa.eu/schemas/common/1.0\"\n"
                               "xmlns:inspire_vs=\"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0\"\n"
                               "xsi:schemaLocation=\"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd\"\n";

      CT::printfconcat(XMLDoc, "<inspire_vs:ExtendedCapabilities %s>\n", inspirexsi.c_str());
      XMLDoc += "  <inspire_common:MetadataUrl xsi:type=\"inspire_common:resourceLocatorType\">\n";
      CT::printfconcat(XMLDoc, "    <inspire_common:URL>%s</inspire_common:URL>\n", viewServiceCSWURL.c_str());
      XMLDoc += "    <inspire_common:MediaType>application/vnd.iso.19139+xml</inspire_common:MediaType>\n";

      XMLDoc += "  </inspire_common:MetadataUrl>\n";
      XMLDoc += "  <inspire_common:SupportedLanguages xsi:type=\"inspire_common:supportedLanguagesType\">\n";
      XMLDoc += "    <inspire_common:DefaultLanguage>\n";
      XMLDoc += "      <inspire_common:Language>eng</inspire_common:Language>\n";
      XMLDoc += "    </inspire_common:DefaultLanguage>\n";
      XMLDoc += "    <inspire_common:SupportedLanguage>\n";
      XMLDoc += "      <inspire_common:Language>eng</inspire_common:Language>\n";
      XMLDoc += "    </inspire_common:SupportedLanguage>\n";
      XMLDoc += "  </inspire_common:SupportedLanguages>\n";
      XMLDoc += "  <inspire_common:ResponseLanguage>\n";
      XMLDoc += "    <inspire_common:Language>eng</inspire_common:Language>\n";
      XMLDoc += "  </inspire_common:ResponseLanguage>\n";
      XMLDoc += "</inspire_vs:ExtendedCapabilities>\n";
    }

    /*Scenario 2*/
    if (useINSPIREScenario == 2) {
      XMLDoc += "<inspire_vs:ExtendedCapabilities>\n";
      XMLDoc += "  <inspire_common:ResourceLocator>\n";
      XMLDoc += "    <inspire_common:URL></inspire_common:URL>\n";
      XMLDoc += "  </inspire_common:ResourceLocator>\n";
      XMLDoc += "  <inspire_common:ResourceType>service</inspire_common:ResourceType>\n";
      XMLDoc += "  <inspire_common:TemporalReference>\n";
      XMLDoc += "  </inspire_common:TemporalReference>\n";
      XMLDoc += "\n";
      XMLDoc += "  <inspire_common:Conformity>\n";
      XMLDoc += "    <inspire_common:Specification>\n";
      XMLDoc += "      <inspire_common:Title>D2.8.III.13-14 Data Specification on Atmospheric Conditions – Guidelines</inspire_common:Title>\n";
      XMLDoc += "      <inspire_common:DateOfPublication>2012-04-20</inspire_common:DateOfPublication>\n";
      XMLDoc += "    </inspire_common:Specification>\n";
      XMLDoc += "    <inspire_common:Degree>conformant</inspire_common:Degree>\n";
      XMLDoc += "  </inspire_common:Conformity>\n";
      XMLDoc += "\n";
      XMLDoc += "  <inspire_common:MetadataPointOfContact>\n";
      XMLDoc += "    <inspire_common:OrganisationName>KNMI</inspire_common:OrganisationName>\n";
      XMLDoc += "    <inspire_common:EmailAddress>adaguc@knmi.nl</inspire_common:EmailAddress>\n";
      XMLDoc += "  </inspire_common:MetadataPointOfContact>\n";
      XMLDoc += "\n";
      XMLDoc += "  <inspire_common:MetadataDate>2015-01-01</inspire_common:MetadataDate>\n";
      XMLDoc += "  <inspire_common:SpatialDataServiceType>view</inspire_common:SpatialDataServiceType>\n";
      XMLDoc += "  <inspire_common:MandatoryKeyword>\n";
      XMLDoc += "    <inspire_common:KeywordValue>infoMapAccessService</inspire_common:KeywordValue>\n";
      XMLDoc += "  </inspire_common:MandatoryKeyword>\n";
      XMLDoc += "\n";
      XMLDoc += "  <inspire_common:Keyword>\n";
      XMLDoc += "    <inspire_common:OriginatingControlledVocabulary>\n";
      XMLDoc += "      <inspire_common:Title>AC-MF Data Type</inspire_common:Title>\n";
      XMLDoc += "      <inspire_common:DateOfCreation>2012-04-20</inspire_common:DateOfCreation>\n";
      XMLDoc += "      <inspire_common:URI>urn:x-inspire:specification:DS-AC-MF:dataType</inspire_common:URI>\n";
      XMLDoc += "      <inspire_common:ResourceLocator>\n";
      XMLDoc += "        <inspire_common:URL></inspire_common:URL>\n";
      XMLDoc += "      </inspire_common:ResourceLocator>\n";
      XMLDoc += "    </inspire_common:OriginatingControlledVocabulary>\n";
      XMLDoc += "    <inspire_common:KeywordValue>prediction</inspire_common:KeywordValue>\n";
      XMLDoc += "  </inspire_common:Keyword>\n";
      XMLDoc += "\n";
      XMLDoc += "  <inspire_common:SupportedLanguages>\n";
      XMLDoc += "    <inspire_common:DefaultLanguage>\n";
      XMLDoc += "      <inspire_common:Language>eng</inspire_common:Language>\n";
      XMLDoc += "    </inspire_common:DefaultLanguage>\n";
      XMLDoc += "    <inspire_common:SupportedLanguage>\n";
      XMLDoc += "      <inspire_common:Language>dut</inspire_common:Language>\n";
      XMLDoc += "    </inspire_common:SupportedLanguage>\n";
      XMLDoc += "  </inspire_common:SupportedLanguages>\n";
      XMLDoc += "  <inspire_common:ResponseLanguage>\n";
      XMLDoc += "    <inspire_common:Language>eng</inspire_common:Language>\n";
      XMLDoc += "  </inspire_common:ResponseLanguage>\n";
      XMLDoc += "</inspire_vs:ExtendedCapabilities>\n";
    }

    // Set INSPIRE SCHEMA
    /*std::string inspirexsi=
      "xmlns:inspire_common=\"http://inspire.ec.europa.eu/schemas/common/1.0\"\n"
      "xmlns:inspire_vs=\"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0\"\n"
      "xsi:schemaLocation=\"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd\"\n";
    CT::replaceSelf(XMLDoc, "[SCHEMADEFINITION]",inspirexsi.c_str());*/

    std::string wms130xsi = "xsi:schemaLocation=\"http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd\"\n";
    CT::replaceSelf(XMLDoc, "[SCHEMADEFINITION]", wms130xsi.c_str());

    // Set INSPIRE contact information
    std::string contactInformation = "";
    CT::printfconcat(contactInformation, "    <ContactPersonPrimary>");
    CT::printfconcat(contactInformation, "      <ContactPerson>%s</ContactPerson>", inspireMetadata.pointOfContact.c_str());
    CT::printfconcat(contactInformation, "      <ContactOrganization>%s</ContactOrganization>", inspireMetadata.organisationName.c_str());
    CT::printfconcat(contactInformation, "    </ContactPersonPrimary>");
    CT::printfconcat(contactInformation, "    <ContactVoiceTelephone>%s</ContactVoiceTelephone>", inspireMetadata.voiceTelephone.c_str());
    CT::printfconcat(contactInformation, "    <ContactElectronicMailAddress>%s</ContactElectronicMailAddress>", inspireMetadata.email.c_str());

    CT::replaceSelf(XMLDoc, "[CONTACTINFORMATION]", contactInformation.c_str());

    CT::replaceSelf(XMLDoc, "[INSPIRE::ABSTRACT]", inspireMetadata.abstract.c_str());
#endif
  } else {
    // Default WMS 1.3.0 service
    std::string wms130xsi = "xsi:schemaLocation=\"http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd\"\n";
    CT::replaceSelf(XMLDoc, "[SCHEMADEFINITION]", wms130xsi.c_str());
    CT::replaceSelf(XMLDoc, "[CONTACTINFORMATION]", "");
  }

  XMLDoc += "<Layer>\n";
  CT::printfconcat(XMLDoc, "<Title>%s</Title>\n", srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->elementValue.c_str());

  const auto firstWMLayer = getFirstLayerWithoutError(metadataLayerList);
  if (firstWMLayer != nullptr) {

    for (auto proj: firstWMLayer->layerMetadata.projectionList) {
      if (!proj.name.empty()) {
        XMLDoc += "<CRS>";
        XMLDoc += proj.name;
        XMLDoc += "</CRS>\n";
      }
    }
    for (auto proj: firstWMLayer->layerMetadata.projectionList) {
      if (!proj.name.empty()) {
        if (srvParam->checkBBOXXYOrder(proj.name.c_str()) == true) {
          CT::printfconcat(XMLDoc, "<BoundingBox CRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj.name.c_str(), proj.dfBBOX[1], proj.dfBBOX[0], proj.dfBBOX[3], proj.dfBBOX[2]);
        } else {
          CT::printfconcat(XMLDoc, "<BoundingBox CRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj.name.c_str(), proj.dfBBOX[0], proj.dfBBOX[1], proj.dfBBOX[2], proj.dfBBOX[3]);
        }
      }
    }

#ifdef ENABLE_INSPIRE

    if (inspireMetadataIsAvailable) {
      CT::replaceSelf(XMLDoc, "[INSPIRE::TITLE]", inspireMetadata.title.c_str());
      XMLDoc += "  <MetadataURL type=\"ISO19115:2005\">\n";
      XMLDoc += "     <Format>application/gml+xml; version=3.2</Format>\n";
      CT::printfconcat(XMLDoc, "     <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>", datasetCSWURL.c_str());
      XMLDoc += "  </MetadataURL>\n";
    }

#endif
    // Make a unique list of all groups
    std::vector<std::string> groupKeys;
    for (size_t lnr = 0; lnr < metadataLayerList.size(); lnr++) {
      MetadataLayer *layer = metadataLayerList[lnr];
      std::string key = "";
      if (layer->layerMetadata.wmsgroup.length() > 0) key = layer->layerMetadata.wmsgroup.c_str();
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
      if (CXMLGEN_DEBUG) {
        CDBDebug("group %s", groupKeys[groupIndex].c_str());
      }
      int groupDepth = 0;

      {
        std::string key = groupKeys[groupIndex];
        auto subGroups = CT::split(key, "/");
        groupDepth = subGroups.size();

        if (groupIndex > 0) {
          std::string prevKey = groupKeys[groupIndex - 1].c_str();
          auto prevSubGroups = CT::split(prevKey, "/");

          for (size_t j = subGroups.size(); j < prevSubGroups.size(); j++) {
            currentGroupDepth--;
            XMLDoc += "</Layer>\n";
          }

          int removeGroups = 0;
          for (size_t j = 0; j < subGroups.size() && j < prevSubGroups.size(); j++) {
            if (subGroups[j] != prevSubGroups[j] || removeGroups == 1) {
              removeGroups = 1;
              XMLDoc += "</Layer>\n";
              currentGroupDepth--;
            }
          }
          for (size_t j = currentGroupDepth; j < subGroups.size(); j++) {
            XMLDoc += "<Layer>\n";
            XMLDoc += "<Title>";
            XMLDoc += subGroups[j].c_str();
            XMLDoc += "</Title>\n";
          }

        } else {
          for (size_t j = 0; j < subGroups.size(); j++) {
            XMLDoc += "<Layer>\n";
            XMLDoc += "<Title>";
            XMLDoc += subGroups[j].c_str();
            XMLDoc += "</Title>\n";
          }
        }
        currentGroupDepth = groupDepth;
      }

      for (size_t lnr = 0; lnr < metadataLayerList.size(); lnr++) {
        MetadataLayer *layer = metadataLayerList[lnr];

        if (layer->layerMetadata.wmsgroup == groupKeys[groupIndex]) {

          if (layer->hasError != 0) {
            addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
          }
          if (layer->hasError == 0) {
            CT::printfconcat(XMLDoc, "<Layer queryable=\"%d\" opaque=\"1\" cascaded=\"%d\">\n", layer->layerMetadata.isQueryable,
                             layer->dataSource->dLayerType == CConfigReaderLayerTypeGraticule && layer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate ? 1 : 0);
            XMLDoc += "<Name>";
            XMLDoc += layer->layerMetadata.name;
            XMLDoc += "</Name>\n";
            std::string layerTitle = layer->layerMetadata.title;
            layerTitle = CT::encodeXml(layerTitle);
            XMLDoc += "<Title>";
            XMLDoc += layerTitle;
            XMLDoc += "</Title>\n";
            // TODO

            if (layer->layerMetadata.abstract.length() > 0) {
              XMLDoc += "<Abstract>";
              XMLDoc += CT::encodeXml(layer->layerMetadata.abstract).c_str();
              XMLDoc += "</Abstract>\n";
            }
#ifdef ENABLE_INSPIRE
            if (inspireMetadataIsAvailable) {
              // Set INSPIRE layer keywords
              XMLDoc += "<KeywordList>\n";
              for (size_t j = 0; j < inspireMetadata.keywords.size(); j++) {
                CT::printfconcat(XMLDoc, "<Keyword>%s</Keyword>\n", inspireMetadata.keywords[j].c_str()); // TODO
              }
              XMLDoc += "</KeywordList>\n";
            }
#endif

            /*if(layer->layerMetadata.cfgLayer->MetadataURL.size()>0){
                XMLDoc += "  <KeywordList><Keyword>precipitation_amount</Keyword></KeywordList>\n";
            }*/
            CT::printfconcat(XMLDoc,
                             "<EX_GeographicBoundingBox>\n"
                             "  <westBoundLongitude>%f</westBoundLongitude>\n"
                             "  <eastBoundLongitude>%f</eastBoundLongitude>\n"
                             "  <southBoundLatitude>%f</southBoundLatitude>\n"
                             "  <northBoundLatitude>%f</northBoundLatitude>\n"
                             "</EX_GeographicBoundingBox>",
                             layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[1], layer->layerMetadata.dfLatLonBBOX[3]);

            for (auto proj: layer->layerMetadata.projectionList) {
              if (srvParam->checkBBOXXYOrder(proj.name.c_str()) == true) {
                CT::printfconcat(XMLDoc, "<BoundingBox CRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj.name.c_str(), proj.dfBBOX[1], proj.dfBBOX[0], proj.dfBBOX[3],
                                 proj.dfBBOX[2]);
              } else {
                CT::printfconcat(XMLDoc, "<BoundingBox CRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n", proj.name.c_str(), proj.dfBBOX[0], proj.dfBBOX[1], proj.dfBBOX[2],
                                 proj.dfBBOX[3]);
              }
            }

            if (firstWMLayer->layer->MetadataURL.size() > 0) {
              std::string layerMetaDataURL = firstWMLayer->layer->MetadataURL[0]->elementValue.c_str();
              CT::replaceSelf(layerMetaDataURL, "&", "&amp;");
              XMLDoc += "  <MetadataURL type=\"ISO19115:2005\">\n";
              XMLDoc += "     <Format>application/gml+xml; version=3.2</Format>\n";
              CT::printfconcat(XMLDoc, "     <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>", layerMetaDataURL.c_str());
              XMLDoc += "  </MetadataURL>\n";
            }

            // Dims
            for (auto dim: layer->layerMetadata.dimList) {
              if (dim.hidden) continue;
              if (CT::indexOf(dim.serviceName, "time") != -1) {
                CT::printfconcat(XMLDoc, "<Dimension name=\"%s\" units=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\" current=\"1\">", dim.serviceName.c_str(), dim.units.c_str(),
                                 dim.defaultValue.c_str(), 1);
              } else {
                CT::printfconcat(XMLDoc, "<Dimension name=\"%s\" units=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\" >", dim.serviceName.c_str(), dim.units.c_str(),
                                 dim.defaultValue.c_str(), 1);
              }
              XMLDoc += dim.values.c_str();
              XMLDoc += "</Dimension>\n";
            }
            if (inspireMetadataIsAvailable) {
              std::string authorityName = "unknown";
              std::string authorityOnlineResource = "unknown";
              std::string identifierAuthority = "unknown";
              std::string identifierId = "unknown";
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
              CT::printfconcat(XMLDoc, " <AuthorityURL name=\"%s\"><OnlineResource xlink:href=\"%s\" /></AuthorityURL>\n", authorityName.c_str(), authorityOnlineResource.c_str());
              CT::printfconcat(XMLDoc, " <Identifier authority=\"%s\">%s</Identifier>\n", identifierAuthority.c_str(), layer->layerMetadata.name.c_str());
            }
            // Styles
            for (auto style: layer->layerMetadata.styleList) {

              XMLDoc += "   <Style>\n";
              CT::printfconcat(XMLDoc, "    <Name>%s</Name>\n", style.name.c_str());
              CT::printfconcat(XMLDoc, "    <Title>%s</Title>\n", style.title.c_str());
              if (style.abstract.length() > 0) {
                CT::printfconcat(XMLDoc, "    <Abstract>%s</Abstract>\n", style.abstract.c_str());
              }
              CT::printfconcat(XMLDoc, "    <LegendURL width=\"%d\" height=\"%d\">\n", LEGEND_WIDTH, LEGEND_HEIGHT);
              XMLDoc += "       <Format>image/png</Format>\n";
              CT::printfconcat(
                  XMLDoc,
                  "       <OnlineResource xlink:type=\"simple\" xlink:href=\"%s&amp;version=1.1.1&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=image/png&amp;STYLE=%s\"/>\n",
                  onlineResource.c_str(), layer->layerMetadata.name.c_str(), style.name.c_str());
              XMLDoc += "    </LegendURL>\n";
              XMLDoc += "  </Style>\n";
            }

            XMLDoc += "</Layer>\n";
          } else {
            CDBError("Skipping layer %s", layer->layerMetadata.name.c_str());
          }
        }
      }
    }

    for (int j = 0; j < currentGroupDepth; j++) {
      XMLDoc += "</Layer>\n";
    }
  }
  XMLDoc += "    </Layer>\n  </Capability>\n</WMS_Capabilities>\n";
  return 0;
}

int CXMLGen::getWCS_1_0_0_Capabilities(std::string &XMLDoc, const std::vector<MetadataLayer *> &metadataLayerList) {
  std::string onlineResource = srvParam->getOnlineResource();
  onlineResource += "SERVICE=WCS&amp;";

  XMLDoc = (WCS_1_0_0_GetCapabilities_Header);
  if (srvParam->cfg->WCS[0]->Title.size() == 0) {
    CDBError("No title defined for WCS");
    return 1;
  }
  if (srvParam->cfg->WCS[0]->Name.size() == 0) {
    srvParam->cfg->WCS[0]->Name.push_back(new CServerConfig::XMLE_Name());
    srvParam->cfg->WCS[0]->Name[0]->elementValue = (srvParam->cfg->WCS[0]->Title[0]->elementValue.c_str());
  }
  if (srvParam->cfg->WCS[0]->Abstract.size() == 0) {
    srvParam->cfg->WCS[0]->Abstract.push_back(new CServerConfig::XMLE_Abstract());
    srvParam->cfg->WCS[0]->Abstract[0]->elementValue = (srvParam->cfg->WCS[0]->Title[0]->elementValue.c_str());
  }
  CT::replaceSelf(XMLDoc, "[SERVICENAME]", srvParam->cfg->WCS[0]->Title[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICETITLE]", srvParam->cfg->WCS[0]->Name[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEABSTRACT]", srvParam->cfg->WCS[0]->Abstract[0]->elementValue.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEONLINERESOURCE]", onlineResource.c_str());
  CT::replaceSelf(XMLDoc, "[SERVICEINFO]", serviceInfo.c_str());

  if (metadataLayerList.size() > 0) {

    for (size_t lnr = 0; lnr < metadataLayerList.size(); lnr++) {
      MetadataLayer *layer = metadataLayerList[lnr];
      if (layer->hasError != 0) {
        addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
      }
      if (layer->hasError == 0) {
        CT::printfconcat(XMLDoc, "<CoverageOfferingBrief>\n");
        XMLDoc += "<description>";
        XMLDoc += layer->layerMetadata.name;
        XMLDoc += "</description>\n";
        XMLDoc += "<name>";
        XMLDoc += layer->layerMetadata.name;
        XMLDoc += "</name>\n";
        std::string layerTitle = layer->layerMetadata.title;
        layerTitle = CT::encodeXml(layerTitle);
        XMLDoc += "<label>";
        XMLDoc += layerTitle;
        XMLDoc += "</label>\n";
        CT::printfconcat(XMLDoc,
                         "  <lonLatEnvelope srsName=\"urn:ogc:def:crs:OGC:1.3:CRS84\">\n"
                         "    <gml:pos>%f %f</gml:pos>\n"
                         "    <gml:pos>%f %f</gml:pos>\n",
                         layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[1], layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[3]);

        CT::printfconcat(XMLDoc, "</lonLatEnvelope>\n");
        CT::printfconcat(XMLDoc, "</CoverageOfferingBrief>\n");

      } else {
        CDBError("Skipping layer %s", layer->layerMetadata.name.c_str());
      }
    }
  }
  XMLDoc += "</ContentMetadata>\n</WCS_Capabilities>\n";
  return 0;
}

void generateWCSRangeSet(std::string &XMLDoc, MetadataLayer *layer) {
  /*
  From the documentation:
  The optional and repeatable axisDescription/AxisDescription element is for compound observations.
  It describes an additional parameter (that is, an independent variable besides space and time),
  and the valid values of this parameter, which GetCoverage requests can use to select subsets of a
  coverage offering.
  */
  if (!layer->layerMetadata.dimList.size()) {
    return;
  }
  XMLDoc += "    <rangeSet>\n"
            "      <RangeSet>\n"
            "        <name>dimensions</name>\n"
            "        <label>dimensions</label>\n";
  // Dims
  for (size_t d = 0; d < layer->layerMetadata.dimList.size(); d++) {
    LayerMetadataDim *dim = &layer->layerMetadata.dimList[d];
    std::string min, max, duration;
    std::vector<std::string> valuesVector;

    // Case of min/max(/duration), for time dimension
    auto valueSplit = CT::split(dim->values, "/");
    if (valueSplit.size() >= 2) {
      min = valueSplit[0];
      max = valueSplit[1];
      // Third value is the interval duration
      if (valueSplit.size() == 3) {
        duration = valueSplit[2];
      }
    } else {
      // General case of a list of values (of any type)
      valueSplit = CT::split(dim->values, ",");
      valuesVector = valueSplit;
      std::sort(valuesVector.begin(), valuesVector.end(), multiTypeSort);
      min = valuesVector[0];
      max = valuesVector.back();
    }

    CT::printfconcat(XMLDoc,
                     "        <axisDescription>\n"
                     "          <AxisDescription>\n"
                     "            <name>%s</name>\n"
                     "            <label>%s</label>\n",
                     dim->cdfName.c_str(), dim->cdfName.c_str());
    if (valueSplit.size() >= 2) {
      CT::printfconcat(XMLDoc,
                       "            <values>\n"
                       "              <interval>\n"
                       "                <min>%s</min>\n"
                       "                <max>%s</max>\n",
                       min.c_str(), max.c_str());
      // Precalculate the interval in the case of time (no interval if fewer than 4 values)
      if ((CT::indexOf(dim->cdfName, "time") != -1) && duration.length() > 0) {
        CT::printfconcat(XMLDoc, "                <res>%s</res>\n", duration.c_str()); // .c_str());
      }
      CT::printfconcat(XMLDoc, "              </interval>\n");
      // Print all possible values if there is a relatively small number, for other dimensions
      if ((valueSplit.size() <= 100) && (CT::indexOf(dim->cdfName, "time") == -1)) {
        for (size_t i = 0; i < valueSplit.size(); i++) {
          CT::printfconcat(XMLDoc, "              <singleValue>%s</singleValue>\n", valuesVector[i].c_str());
        }
      }
      CT::printfconcat(XMLDoc, "            </values>\n");
    }
    CT::printfconcat(XMLDoc, "          </AxisDescription>\n"
                             "        </axisDescription>\n");
  }

  XMLDoc += "      </RangeSet>\n"
            "    </rangeSet>\n";
}

int CXMLGen::getWCS_1_0_0_DescribeCoverage(std::string &XMLDoc, const std::vector<MetadataLayer *> &metadataLayerList) {

  XMLDoc = ("<?xml version='1.0' encoding=\"ISO-8859-1\" ?>\n"
            "<CoverageDescription\n"
            "   version=\"1.0.0\" \n"
            "   updateSequence=\"0\" \n"
            "   xmlns=\"http://www.opengis.net/wcs\" \n"
            "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n"
            "   xmlns:gml=\"http://www.opengis.net/gml\" \n"
            "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
            "   xsi:schemaLocation=\"http://www.opengis.net/wcs http://schemas.opengis.net/wcs/1.0.0/describeCoverage.xsd\">\n");
  const auto firstWMLayer = getFirstLayerWithoutError(metadataLayerList);
  if (firstWMLayer != nullptr) {
    for (size_t layerIndex = 0; layerIndex < srvParam->requestedLayerNames.size(); layerIndex++) {
      for (size_t lnr = 0; lnr < metadataLayerList.size(); lnr++) {
        MetadataLayer *layer = metadataLayerList[lnr];
        if (layer->layerMetadata.name == srvParam->requestedLayerNames[layerIndex]) {
          if (layer->hasError != 0) {
            addErrorInXMLForMisconfiguredLayer(XMLDoc, layer);
          }
          if (layer->hasError == 0) {

            // Look wether and which dimension is a time dimension
            int timeDimIndex = -1;
            int d = 0;
            for (auto dim: layer->layerMetadata.dimList) {
              if (dim.units == "ISO8601") {
                timeDimIndex = d;
              }
              d++;
            }

            if (srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
              std::string layerTitle = layer->layerMetadata.title;
              layerTitle = CT::encodeXml(layerTitle);
              CT::printfconcat(XMLDoc,
                               "  <CoverageOffering>\n"
                               "  <description>%s</description>\n"
                               "  <name>%s</name>\n"
                               "  <label>%s</label>\n",
                               layer->layerMetadata.name.c_str(), layer->layerMetadata.name.c_str(), layerTitle.c_str());
              if (layer->layerMetadata.variableList.size() > 0) {
                CT::printfconcat(XMLDoc, "  <uom>%s</uom>\n", layer->layerMetadata.variableList[0].units.c_str());
              }
              CT::printfconcat(XMLDoc,
                               "  <lonLatEnvelope srsName=\"urn:ogc:def:crs:OGC:1.3:CRS84\">\n"
                               "    <gml:pos>%f %f</gml:pos>\n"
                               "    <gml:pos>%f %f</gml:pos>\n",
                               layer->layerMetadata.dfLatLonBBOX[0], layer->layerMetadata.dfLatLonBBOX[1], layer->layerMetadata.dfLatLonBBOX[2], layer->layerMetadata.dfLatLonBBOX[3]);

              if (timeDimIndex >= 0) {
                // For information about this, visit http://www.galdosinc.com/archives/151
                auto timeDimSplit = CT::split(layer->layerMetadata.dimList[timeDimIndex].values, "/");
                if (timeDimSplit.size() == 3) {
                  XMLDoc += "        <gml:TimePeriod>\n";
                  CT::printfconcat(XMLDoc, "          <gml:begin>%s</gml:begin>\n", timeDimSplit[0].c_str());
                  CT::printfconcat(XMLDoc, "          <gml:end>%s</gml:end>\n", timeDimSplit[1].c_str());
                  CT::printfconcat(XMLDoc, "          <gml:duration>%s</gml:duration>\n", timeDimSplit[2].c_str());
                  XMLDoc += "        </gml:TimePeriod>\n";
                }
              }
              XMLDoc += "  </lonLatEnvelope>\n"
                        "  <domainSet>\n"
                        "    <spatialDomain>\n";
              for (auto proj: layer->layerMetadata.projectionList) {

                std::string encodedProjString(proj.name.c_str());

                CT::printfconcat(XMLDoc,
                                 "        <gml:Envelope srsName=\"%s\">\n"
                                 "          <gml:pos>%f %f</gml:pos>\n"
                                 "          <gml:pos>%f %f</gml:pos>\n"
                                 "        </gml:Envelope>\n",
                                 encodedProjString.c_str(), proj.dfBBOX[0], proj.dfBBOX[1], proj.dfBBOX[2], proj.dfBBOX[3]);
              }
              int width = layer->layerMetadata.width - 1;
              int height = layer->layerMetadata.height - 1;
              if (width <= 1) {
                width = 999;
              }
              if (height <= 1) {
                height = 999;
              }
              CT::printfconcat(XMLDoc,
                               "        <gml:RectifiedGrid dimension=\"2\">\n"
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
                XMLDoc += "      <temporalDomain>\n";
                if (layer->layerMetadata.dimList[timeDimIndex].hasMultipleValues == 0) {
                  auto timeDimSplit = CT::split(layer->layerMetadata.dimList[timeDimIndex].values, "/");
                  if (timeDimSplit.size() == 3) {
                    XMLDoc += "        <gml:TimePeriod>\n";
                    CT::printfconcat(XMLDoc, "          <gml:begin>%s</gml:begin>\n", timeDimSplit[0].c_str());
                    CT::printfconcat(XMLDoc, "          <gml:end>%s</gml:end>\n", timeDimSplit[1].c_str());
                    CT::printfconcat(XMLDoc, "          <gml:duration>%s</gml:duration>\n", timeDimSplit[2].c_str());
                    XMLDoc += "        </gml:TimePeriod>\n";
                  }
                } else {

                  auto positions = CT::split(layer->layerMetadata.dimList[timeDimIndex].values, ",");
                  for (size_t p = 0; p < positions.size(); p++) {
                    CT::printfconcat(XMLDoc, "        <gml:timePosition>%s</gml:timePosition>\n", (positions[p]).c_str());
                  }
                }
                XMLDoc += "      </temporalDomain>\n";
              }
              XMLDoc += "    </domainSet>\n";
              // Generate the XML code for RangeSet, including dimensions (AxisDescriptions)
              generateWCSRangeSet(XMLDoc, layer);
              // Supported CRSs
              XMLDoc += "    <supportedCRSs>\n";

              for (auto proj: layer->layerMetadata.projectionList) {
                std::string encodedProjString(proj.name.c_str());
                CT::printfconcat(XMLDoc, "      <requestResponseCRSs>%s</requestResponseCRSs>\n", encodedProjString.c_str());
              }

              std::string prettyCRS = layer->layerMetadata.nativeEPSG.c_str();
              CT::printfconcat(XMLDoc, "      <nativeCRSs>%s</nativeCRSs>\n    </supportedCRSs>\n", prettyCRS.c_str());

              XMLDoc += "    <supportedFormats nativeFormat=\"NetCDF4\">\n"
                        "      <formats>GeoTIFF</formats>\n"
                        "      <formats>AAIGRID</formats>\n";

              for (size_t p = 0; p < srvParam->cfg->WCS[0]->WCSFormat.size(); p++) {
                CT::printfconcat(XMLDoc, "      <formats>%s</formats>\n", srvParam->cfg->WCS[0]->WCSFormat[p]->attr.name.c_str());
              }
              XMLDoc += "    </supportedFormats>\n";
              CT::printfconcat(XMLDoc, "    <supportedInterpolations default=\"nearest neighbor\">\n"
                                       "      <interpolationMethod>nearest neighbor</interpolationMethod>\n"
                                       //     "      <interpolationMethod>bilinear</interpolationMethod>\n"
                                       "    </supportedInterpolations>\n");
              CT::printfconcat(XMLDoc, "</CoverageOffering>\n");
            }
          }
        }
      }
    }
  }
  XMLDoc += "</CoverageDescription>\n";

  return 0;
}

int CXMLGen::OGCGetCapabilities(CServerParams *_srvParam, std::string &XMLDocument) {

  this->srvParam = _srvParam;

  int status = 0;
  std::vector<MetadataLayer *> metadataLayerList;

  for (size_t j = 0; j < srvParam->cfg->Layer.size(); j++) {
    if (srvParam->cfg->Layer[j]->attr.type == "autoscan") {
      continue;
    }
    if (srvParam->cfg->Layer[j]->attr.hidden == "true") {
      continue;
    }
    // Create a new layer and push it in the list
    MetadataLayer *metadataLayer = new MetadataLayer();
    metadataLayerList.push_back(metadataLayer);
    metadataLayer->layer = srvParam->cfg->Layer[j];
    metadataLayer->srvParams = srvParam;
    populateMetadataLayerStruct(metadataLayer, true);
  }

  serviceInfo = CT::printf("ADAGUCServer version %s, of %s %s", ADAGUCSERVER_VERSION, __DATE__, __TIME__);
  // Generate an XML document on basis of the information gathered above.
  std::string XMLDoc;
  status = 0;
  if (srvParam->requestType == REQUEST_WMS_GETCAPABILITIES) {
    if (srvParam->OGCVersion == WMS_VERSION_1_0_0) {
      status = getWMS_1_0_0_Capabilities(XMLDoc, metadataLayerList);
    }
    if (srvParam->OGCVersion == WMS_VERSION_1_1_1) {
      status = getWMS_1_1_1_Capabilities(XMLDoc, metadataLayerList);
    }
    if (srvParam->OGCVersion == WMS_VERSION_1_3_0) {
      status = getWMS_1_3_0_Capabilities(XMLDoc, metadataLayerList);
    }
  }
  try {
    if (srvParam->requestType == REQUEST_WCS_GETCAPABILITIES) {
#ifndef ADAGUC_USE_GDAL
      CServerParams::showWCSNotEnabledErrorMessage();
      throw(__LINE__);
#else
      status = getWCS_1_0_0_Capabilities(XMLDoc, metadataLayerList);
#endif
    }

    if (srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
#ifndef ADAGUC_USE_GDAL
      CServerParams::showWCSNotEnabledErrorMessage();
      throw(__LINE__);
#else
      status = getWCS_1_0_0_DescribeCoverage(XMLDoc, metadataLayerList);
#endif
    }
  } catch (int e) {
    status = 1;
  }

  bool errorsHaveOccured = false;

  for (size_t j = 0; j < metadataLayerList.size(); j++) {
    if (metadataLayerList[j]->hasError) errorsHaveOccured = true;
    delete metadataLayerList[j]->dataSource;
    metadataLayerList[j]->dataSource = nullptr;
    delete metadataLayerList[j];
    metadataLayerList[j] = NULL;
  }
  metadataLayerList.clear();

  if (status != 0) {
    CDBError("XML geneneration failed, please check logs. ");
    return CXMLGEN_FATAL_ERROR_OCCURED;
  }
  XMLDocument += XMLDoc;

  resetErrors();

  if (errorsHaveOccured) return CXML_NON_FATAL_ERRORS_OCCURED;
  return 0;
}
