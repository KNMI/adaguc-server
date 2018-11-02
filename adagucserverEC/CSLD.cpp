#include "CDirReader.h"
#include "CDebugger.h"
#include "CSLD.h"
#include "../hclasses/CReadFile.h"
#include "../hclasses/CDirReader.h"
#include "../hclasses/CReadFile.h"
#include "../hclasses/CXMLParser.h"


const char *CSLD::className = "CSLD";
CT::string CSLD::XML_RULES::RASTER_SYMBOLIZER::NAME = "RasterSymbolizer";
CT::string CSLD::XML_RULES::RASTER_SYMBOLIZER::CHILDS::COLOR_MAP = "ColorMap";

CT::string CSLD::XML_RULES::MIN_SCALE_DENOMINATOR::NAME = "MinScaleDenominator";
CT::string CSLD::XML_RULES::MAX_SCALE_DENOMINATOR::NAME = "MaxScaleDenominator";

CT::string CSLD::STYLE_NAME_TEMPLATE = "ADAGUCSLD_";

CSLD::CSLD() {
  this->parameterName = "SLD";

  //SETUP STYLE variables
  this->myOwnStyle = new CServerConfig::XMLE_Style();
  this->myOwnLegend = new CServerConfig::XMLE_Legend();
}

void CSLD::setServerParams(CServerParams *serverParams) {
  this->serverParams = serverParams;
  this->serverConfig = serverParams->cfg;
}

int CSLD::processSLD(CT::string sldName) {
  CDirReader cDirReader;
  CReadFile cReadFile;
  CT::string sldFile;

  CT::string fileName = this->serverConfig->SLD[0]->attr.sldlocation + "/" + sldName;

  try {
    sldFile = cReadFile.open(fileName.c_str());
  } catch (int e) {
    if (e == CREADFILE_FILENOTFOUND) {
      CDBError("File [%s] not found", fileName.c_str());
      return 1;
    }
    if (e == CREADFILE_FILENOTREAD) {
      CDBError("Unable to read file [%s]", fileName.c_str());
      return 2;
    }
  }

  //Loop trough all NamedLayers in SLD, can be multiple for styling multiple Layers
  try {

    CXMLParserElement parsedElement;
    parsedElement.parse(sldFile);

    //Extract NamedLayers
    CXMLParser::XMLElement::XMLElementPointerList namedLayers = parsedElement.get("StyledLayerDescriptor")->getList("NamedLayer");

    for (size_t i = 0; i < namedLayers.size(); ++i) {

      //LayerName
      CXMLParserElement *namedLayerElement = namedLayers.get(i);
      CT::string sldLayerName = namedLayerElement->get("Name")->getValue();

      for (size_t j = 0; j < this->serverConfig->Layer.size(); j++) {

        CServerConfig::XMLE_Layer *layer = this->serverConfig->Layer[j];

        //Generate unique layer name for layer in Server Config
        CT::string layerUniqueName;
        if (this->serverParams->makeUniqueLayerName(&layerUniqueName, this->serverConfig->Layer[j]) != 0) {
          CDBError("Unable to compose layer name");
          return 1;
        }

        #ifdef CSLD_DEBUG
        CDBDebug("Checking layer [%s]", layerUniqueName.c_str());
        CDBDebug("Layername from SLD [%s]", sldLayerName.c_str());
        #endif

        int status = 0;

        if (layerUniqueName.equals(sldLayerName)) {
          CDBDebug("Found layer [%s], adding style.", layerUniqueName.c_str());

          /* Add SLD style name to Styles element of Layer */
          if (layer->Styles.size() == 0) {
            CServerConfig::XMLE_Styles *layerStyles = new CServerConfig::XMLE_Styles();
            layer->Styles.push_back(layerStyles);
          }

          CT::string uniqueStyleName = STYLE_NAME_TEMPLATE;
          uniqueStyleName.printconcat("%i", i);

          //Initialize styling variables, to make sure they are empty and new
          this->myOwnStyle = new CServerConfig::XMLE_Style();
          this->myOwnLegend = new CServerConfig::XMLE_Legend();

          layer->Styles[0]->value = uniqueStyleName.c_str();
          this->myOwnStyle->attr.name = uniqueStyleName.c_str();
          this->serverConfig->Style.push_back(myOwnStyle);

          this->myOwnLegend->attr.name = "radarlegend";
          this->myOwnLegend->attr.type.print("interval");
          this->serverConfig->Legend.push_back(myOwnLegend);

          CServerConfig::XMLE_Legend *styleLegend = new CServerConfig::XMLE_Legend();
          styleLegend->value = myOwnLegend->attr.name;
          this->myOwnStyle->Legend.push_back(styleLegend);

          CServerConfig::XMLE_RenderMethod *renderMethod = new CServerConfig::XMLE_RenderMethod();
          renderMethod->value = "shadedContour";
          this->myOwnStyle->RenderMethod.push_back(renderMethod);

          status = this->validateSLDElements(namedLayerElement);

          /*
           * Layer in server matched with SLD.
           * Check if all NamedLayers in SLD are looped because there is no need to finish loop Config Layers
           */
          if (i != (namedLayers.size() - 1)) {
            //Break this layer loop and do next NamedLayer
            break;
          }
        }

        /*
         * Check if
         * all NamedLayers in SLD are looped
         * all Layers in the server Configuration are looped
         * status from validating the SLD is 0
         */
        if (i == (namedLayers.size() - 1) && j == (this->serverConfig->Layer.size() - 1) && status == 0) {
          CDBDebug("Looping NamedLayers in SLD and Layers in server configuration are completed with 0 errors");
          return 0;
        } else {
          if (status != 0) {
            CDBError("Something went wrong validatingSLDElements");
            return 1;
          }
        }
      }
    }
  } catch (int e) {
    CT::string message = CXMLParser::getErrorMessage(e);
    CDBError("%s\n", message.c_str());
    return 1;
  }
}


int CSLD::validateSLDElements(CXMLParserElement *element) {
  try {
    //Extract UserStyle
    CXMLParser::XMLElement *userStyle = element->get("UserStyle");

    //Extract Rules from UserStyle
    CXMLParser::XMLElement::XMLElementPointerList rules = userStyle->get("FeatureTypeStyle")->getList("Rule");

    //Check how many Rule elements are supported
    if (rules.size() == MAX_RULE_ELEMENTS) {

      int status;

      //Foreach rules, Can be multiple Rule
      for (size_t i = 0; i < rules.size(); ++i) {

        //Get child elements of Rule
        CXMLParser::XMLElement::XMLElementList *childElementsList = rules.get(i)->getElements();

        for (size_t i = 0; i < childElementsList->size(); i++) {

          CXMLParserElement childElement = childElementsList->at(i);

          //Check if rule Child is supported
          if (childElement.getName().equals(XML_RULES_MIN_SCALE_DENOMINATOR_NAME)) {

            //MinScaleDenominator support
            status = this->buildScaleDenominator(&childElement);

          } else if (childElement.getName().equals(XML_RULES_MAX_SCALE_DENOMINATOR_NAME)) {

            //MaxScaleDenominator support
            status = this->buildScaleDenominator(&childElement);

          } else if (childElement.getName().equals(XML_RULES_RASTER_SYMBOLIZER_NAME)) {

            //RasterSymbolizer support
            status = this->buildRasterSymbolizer(&childElement);

          } else {
            CDBError("Child %s in Rule Element not supported (yet)", childElement.getName().c_str());
            return 1;
          }

          //Check if all child elements in Rule elements are looped.
          if (i == childElementsList->size() - 1) {
            return status;
          }
        }
      }
    } else {
      CDBError("SLD cannot contain more then %i Rule elements", MAX_RULE_ELEMENTS);
      return 1;
    }
  } catch (int e) {
    CT::string message = CXMLParser::getErrorMessage(e);
    CDBError("%s\n", message.c_str());
    return 1;
  }
}

int CSLD::buildScaleDenominator(CXMLParserElement *element) {

  if (element->getName() == XML_RULES_MIN_SCALE_DENOMINATOR_NAME) {

    CServerConfig::XMLE_Min *min = new CServerConfig::XMLE_Min();
    min->value = element->getValue();
    this->myOwnStyle->Min.push_back(min);
    return 0;

  } else if (element->getName() == XML_RULES_MAX_SCALE_DENOMINATOR_NAME) {

    CServerConfig::XMLE_Max *max = new CServerConfig::XMLE_Max();
    max->value = element->getValue();
    this->myOwnStyle->Max.push_back(max);

    return 0;
  }

  CDBError("Something went wrong building element %s", element->getName());
  return 1;
}


int CSLD::buildRasterSymbolizer(CXMLParserElement *childElement) {

  CXMLParser::XMLElement::XMLElementList *childElementsList = childElement->getElements();

  for (size_t i = 0; i < childElementsList->size(); i++) {
    CXMLParserElement element = childElementsList->at(i);

    if (element.getName().equals(XML_RULES_RASTER_SYMBOLIZER_CHILD_COLOR_MAP)) {

      return this->buildColorMap(&element);

    } else {
      CDBError("Child %s in %s Element not supported (yet)", element.getName().c_str(),childElement->getName().c_str());
      return 1;
    }
  }

  CDBError("Something went wrong building RasterSymbolizer");
  return 1;
}

int CSLD::buildColorMap(CXMLParserElement *element) {
  CXMLParser::XMLElement::XMLElementPointerList colorMapEntries = element->getList("ColorMapEntry");

  //Foreach colorMapEntries
  for (size_t i = 0; i < colorMapEntries.size(); i++) {
    CXMLParserElement *colorMapEntry = colorMapEntries.get(i);

    CServerConfig::XMLE_ShadeInterval *shadeInterval = new CServerConfig::XMLE_ShadeInterval();
    shadeInterval->attr.min = colorMapEntry->getAttrValue("quantity");

    //Set the max min value
    CT::string max;

    //Set Max attribute, Current element is < the amount of entries
    if (i + 1 < colorMapEntries.size()) {
      max = colorMapEntries.get(i + 1)->getAttrValue("quantity");
      shadeInterval->attr.max = max;
    } else {
      if (this->myOwnStyle->Max.size() == 1) {
        shadeInterval->attr.max = this->myOwnStyle->Max[0]->value;
      } else {
        CDBError("Missing element %s for setting the last ColorMapEntry Max attribute", XML_RULES_MAX_SCALE_DENOMINATOR_NAME.c_str());
        return 1;
      }
    }

    //Make sure hex code is uppercase
    CT::string hexColor = colorMapEntry->getAttrValue("color");
    hexColor.toUpperCaseSelf();

    shadeInterval->attr.fillcolor.print(hexColor.c_str());
    this->myOwnStyle->ShadeInterval.push_back(shadeInterval);

    if (i == (colorMapEntries.size() - 1)) {
      CDBDebug("Building ColorMap complete");
      return 0;
    }
  }

  CDBError("Something went wrong building ColorMap");
  return 1;
}

bool CSLD::parameterIsSld(CT::string param) {
  if (param.equals(this->parameterName)) {
    return true;
  }
  return false;
}

bool CSLD::serverConfigCheck(CServerConfig::XMLE_Configuration *serverConfig) {
  if (serverConfig->SLD.size() != 1) {
    CDBError("SLD not correctly configured.");
    return false;
  }
  return true;
}



