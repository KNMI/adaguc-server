#ifdef ENABLE_CURL

#include "CDebugger.h"
#include "CSLD.h"
#include "../hclasses/CXMLParser.h"
#include "../hclasses/CHTTPTools.h"

const char *CSLD::className = "CSLD";

void CSLD::setServerParams(CServerParams *serverParams) {
  this->serverParams = serverParams;
  this->serverConfig = serverParams->cfg;
}

//Start processing the SLD url
int CSLD::processSLDUrl(CT::string sldUrl) {
  //Check if url extension is .xml
  if(!sldUrl.startsWith("http") || !sldUrl.endsWith(".xml")){
    CDBError("SLD parameter value needs to be a url pointed to a .xml file");
    return 1;
  }

  //Check if serverParams are initialized in this class
  if(!serverParams || !serverConfig){
    CDBDebug("Variable serverParams or serverConfig config are not set. See function setServerParams.");
    return 1;
  }

  //Get SLD file from URL
  CT::string sldFromUrl;
  try {
    sldFromUrl = CHTTPTools::getString(sldUrl, MAX_FILE_SIZE_ALLOWED);
  } catch(int error){
    CDBError("Could not retrieve SLD from: %s", sldUrl.c_str());
    return 1;
  }

  //Check if the retrieved file contains SLD standard
  if(!sldFromUrl.startsWith("<?xml") || !sldFromUrl.endsWith("StyledLayerDescriptor>")){
    CDBError("Invalid SLD, not matching standard");
    return 1;
  }

  //Loop trough all NamedLayers in SLD, can be multiple for styling multiple Layers
  try {

    CXMLParserElement parsedElement;
    parsedElement.parse(sldFromUrl);

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
          #ifdef CSLD_DEBUG
          CDBDebug("Found layer [%s], adding style.", layerUniqueName.c_str());
          #endif

          /* Add SLD style name to Styles element of Layer */
          if (layer->Styles.size() == 0) {
            CServerConfig::XMLE_Styles *layerStyles = new CServerConfig::XMLE_Styles();
            layer->Styles.push_back(layerStyles);
          }

          CT::string uniqueStyleName = STYLE_NAME_TEMPLATE;
          uniqueStyleName.printconcat("%i", i);

          //Initialize styling variables, to make sure they are empty and new
          CServerConfig::XMLE_Style *myOwnStyle = new CServerConfig::XMLE_Style();
          this->serverConfig->Style.push_back(myOwnStyle);
          
          CServerConfig::XMLE_Legend *myOwnLegend = new CServerConfig::XMLE_Legend();
          this->serverConfig->Legend.push_back(myOwnLegend);
          
          layer->Styles[0]->value = uniqueStyleName.c_str();
          myOwnStyle->attr.name = uniqueStyleName.c_str();
          
          CT::string uniqueLegendName = LEGEND_NAME_TEMPLATE;
          uniqueLegendName.printconcat("%i", i);
          myOwnLegend->attr.name = uniqueLegendName.c_str();          
          myOwnLegend->attr.type.print("interval");
          

          CServerConfig::XMLE_Legend *styleLegend = new CServerConfig::XMLE_Legend();
          myOwnStyle->Legend.push_back(styleLegend);
          styleLegend->value = myOwnLegend->attr.name;
          

          CServerConfig::XMLE_RenderMethod *renderMethod = new CServerConfig::XMLE_RenderMethod();
          myOwnStyle->RenderMethod.push_back(renderMethod);
          renderMethod->value = "shadedContour";
          

          status = this->validateAndParseSLDElements(namedLayerElement, myOwnStyle);

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
          #ifdef CSLD_DEBUG
          CDBDebug("Looping NamedLayers in SLD and Layers in server configuration are completed with 0 errors");
          #endif
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
  //Prevent warning while compiling, must return a value. If code gets here, something is not right
  return 1;
}


int CSLD::validateAndParseSLDElements(CXMLParserElement *element, CServerConfig::XMLE_Style *myOwnStyle) {
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
          if (childElement.getName().equals(RULE_MIN_SCALE_DENOMINATOR)) {

            //MinScaleDenominator support
            status = this->buildScaleDenominator(&childElement, myOwnStyle);

          } else if (childElement.getName().equals(RULE_MAX_SCALE_DENOMINATOR)) {

            //MaxScaleDenominator support
            status = this->buildScaleDenominator(&childElement, myOwnStyle);

          } else if (childElement.getName().equals(RULE_RASTER_SYMBOLIZER)) {

            //RasterSymbolizer support
            status = this->buildRasterSymbolizer(&childElement, myOwnStyle);

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
  //Prevent warning while compiling, must return a value. If code gets here, something is not right
  return 1;
}

int CSLD::buildScaleDenominator(CXMLParserElement *element, CServerConfig::XMLE_Style *myOwnStyle) {

  if (element->getName().equals(RULE_MIN_SCALE_DENOMINATOR)) {

    CServerConfig::XMLE_Min *min = new CServerConfig::XMLE_Min();
    min->value = element->getValue();
    myOwnStyle->Min.push_back(min);
    return 0;

  } else if (element->getName().equals(RULE_MAX_SCALE_DENOMINATOR)) {

    CServerConfig::XMLE_Max *max = new CServerConfig::XMLE_Max();
    max->value = element->getValue();
    myOwnStyle->Max.push_back(max);

    return 0;
  }

  CDBError("Something went wrong building element %s", element->getName().c_str());
  return 1;
}


int CSLD::buildRasterSymbolizer(CXMLParserElement *childElement, CServerConfig::XMLE_Style *myOwnStyle) {

  CXMLParser::XMLElement::XMLElementList *childElementsList = childElement->getElements();

  for (size_t i = 0; i < childElementsList->size(); i++) {
    CXMLParserElement element = childElementsList->at(i);

    if (element.getName().equals(RULE_RASTER_SYMBOLIZER_CHILD_COLOR_MAP)) {
      return this->buildColorMap(&element, myOwnStyle);
    } else {
      CDBError("Child %s in %s Element not supported (yet)", element.getName().c_str(),childElement->getName().c_str());
      return 1;
    }
  }

  CDBError("Something went wrong building RasterSymbolizer");
  return 1;
}

int CSLD::buildColorMap(CXMLParserElement *element, CServerConfig::XMLE_Style *myOwnStyle) {
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
      //Get the quantity of the next colorMapEntry.
      max = colorMapEntries.get(i + 1)->getAttrValue("quantity");
      //Set the quantity value as max on ShadeInterval
      shadeInterval->attr.max = max;
    } else {
      if (myOwnStyle->Max.size() == 1) {
        shadeInterval->attr.max = myOwnStyle->Max[0]->value;
      } else {
        CDBError("Missing element %s for setting the last ColorMapEntry Max attribute", RULE_MAX_SCALE_DENOMINATOR);
        return 1;
      }
    }

    //Make sure hex code is uppercase
    CT::string hexColor = colorMapEntry->getAttrValue("color");
    hexColor.toUpperCaseSelf();

    shadeInterval->attr.fillcolor.print(hexColor.c_str());
    myOwnStyle->ShadeInterval.push_back(shadeInterval);

    if (i == (colorMapEntries.size() - 1)) {
      #ifdef CSLD_DEBUG
      CDBDebug("Building ColorMap complete");
      #endif
      return 0;
    }
  }

  CDBError("Something went wrong building ColorMap");
  return 1;
}

bool CSLD::parameterIsSld(CT::string param) {
  if (param.equals(SLD_PARAMETER_NAME)) {
    return true;
  }
  return false;
}


#endif