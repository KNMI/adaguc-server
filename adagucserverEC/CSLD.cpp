#include "CDirReader.h"
#include "CDebugger.h"
#include "CSLD.h"
#include "../hclasses/CReadFile.h"
#include "../hclasses/CDirReader.h"
#include "../hclasses/CReadFile.h"

#include "../hclasses/CXMLParser.h"


const char *CSLD::className="CSLD";
CT::string CSLD::XML_RULE_FEATURES::RASTER_SYMBOLIZER_NAME = "RasterSymbolizer";

CSLD::CSLD() {
    parameterName = "SLD";

}

void CSLD::setServerConfig(CServerConfig::XMLE_Configuration *serverConfig){
    this->serverConfig = serverConfig;
}


int CSLD::validateSLDFile(CT::string sldName) {
    CDirReader cDirReader;
    CReadFile cReadFile;
    CT::string sldFile;

    CT::string fileName = this->serverConfig->SLD[0]->attr.sldlocation + "/" + sldName;

    try {
        sldFile = cReadFile.open(fileName.c_str());
    }catch (int e) {
        if (e == CREADFILE_FILENOTFOUND){
            CDBError("File [%s] not found", fileName.c_str());
            return 1;
        }
        if (e == CREADFILE_FILENOTREAD){
            CDBError("Unable to read file [%s]", fileName.c_str());
            return 2;
        }
    }

    //CDBDebug("%s", sldFile.c_str());

    this->extractSLD(sldFile);
    //TODO Build ADAGUC Server Compatible styling configuration


//    CT::string fillcolor = "#0000FF";
//
//    CServerConfig::XMLE_Style * myOwnStyle = new CServerConfig::XMLE_Style();
//    this->serverConfig->Style.push_back(myOwnStyle);
//    myOwnStyle->attr.name = "radarstyle";
//
//    CServerConfig::XMLE_Legend* myOwnLegend = new CServerConfig::XMLE_Legend();
//    myOwnLegend->attr.name ="radarlegend";
//    myOwnLegend->attr.type.print("colorRange");
//    this->serverConfig->Legend.push_back(myOwnLegend);
//
//
//    //Create a palette
//    CDBDebug("My fillcolor %s", fillColor.c_str());
//    CServerConfig::XMLE_palette* myPalette = new CServerConfig::XMLE_palette();
//    myPalette->attr.index = 0;
//    myPalette->addAttribute("color", fillColor.c_str());
//    //Add palette to legend
//    myOwnLegend->palette.push_back(myPalette);
//
//    CServerConfig::XMLE_palette* myPalette2 = new CServerConfig::XMLE_palette();
//    myPalette2->attr.index = 125;
//    myPalette2->addAttribute("color", "#FF0000");
//    //Add palette to legend
//    myOwnLegend->palette.push_back(myPalette2);
//
//    CServerConfig::XMLE_Legend* styleLegend = new CServerConfig::XMLE_Legend();
//    styleLegend->value = myOwnLegend->attr.name;
//
//    myOwnStyle->Legend.push_back(styleLegend);
//
//
//    CServerConfig::XMLE_Min * min = new CServerConfig::XMLE_Min();
//    min->value = "0.1";
//    myOwnStyle->Min.push_back(min);
//
//    CServerConfig::XMLE_Max * max = new CServerConfig::XMLE_Max();
//    max->value = "100";
//    myOwnStyle->Max.push_back(max);
//
//    CServerConfig::XMLE_Log * log = new CServerConfig::XMLE_Log();
//    log->value = "100";
//    myOwnStyle->Log.push_back(log);
//
//    CServerConfig::XMLE_RenderMethod * renderMethod = new CServerConfig::XMLE_RenderMethod();
//    renderMethod->value = "nearest";
//    myOwnStyle->RenderMethod.push_back(renderMethod);

//    CServerConfig::XMLE_ShadeInterval *shadeInterval = new CServerConfig::XMLE_ShadeInterval();
//    shadeInterval->attr.min.print("%f", "0.0");
//    shadeInterval->attr.max.print("%f", "100.0");
//    shadeInterval->attr.fillcolor.print(fillColor.c_str());
//    myOwnStyle->ShadeInterval.push_back(shadeInterval);

//    for (int j=0;j<100;j++) {
//        CServerConfig::XMLE_ShadeInterval *shadeInterval = new CServerConfig::XMLE_ShadeInterval();
//        shadeInterval->attr.min.print("%f", (float(j) / 2));
//        shadeInterval->attr.max.print("%f", (float(j) / 2) + 0.5);
//        shadeInterval->attr.fillcolor.print(fillColor.c_str());
//        myOwnStyle->ShadeInterval.push_back(shadeInterval);
//    }

    return 0;
}

void CSLD::extractSLD(CT::string sld) {

    CXMLParserElement element;
    try {
        element.parse(sld);
        //Extract UserStyle
        CT::string userStyle = element.get("StyledLayerDescriptor")->get("NamedLayer")->get("UserStyle")->toString().c_str();

        //Extract Rules from UserStyle
        element.parse(userStyle);
        CXMLParser::XMLElement::XMLElementPointerList rules = element.get("UserStyle")->get("FeatureTypeStyle")->getList("Rule");


        //Foreach rules, Can be multiple Rule
        for (size_t i = 0; i < rules.size(); ++i) {

            //Get the feature element in Rule. This element is always the first element in in the Rule root.
            CXMLParserElement *featureElement = rules.get(i)->getFirst();

            //Check if rule Feature is supported
            if(featureElement->getName().equals(FEATURE_RASTER_SYMBOLIZER_NAME)){
                //Build in RasterSymbolizer support
                //RasterSymbolizer * have multiple supported properies, ColorMap for this one.
                this->buildRasterSymbolizer(featureElement);
            } else {
                CDBError("Feature %s in Rule Element not supported (yet)", featureElement->getName().c_str());
            }
        }

    }catch(int e){
        CT::string message=CXMLParser::getErrorMessage(e);
        CDBError("%s\n",message.c_str());
    }
}


void CSLD::buildRasterSymbolizer(CXMLParserElement *element) {
    CDBDebug("HALOOOTJES %s", element->getName().c_str());

    CDBDebug(element->toString().c_str());

    //Loop trough elements, if element is found, send the element (with childs) to function.
    //TODO find: Colormap -> go ColorMap function, Test build in Opacity. Build classes for RasterSymbolizer etc. with Build specific functions

    //TODO Loop vector list?
    CXMLParser::XMLElement::XMLElementList *list = element->getElements();

    for (size_t i = 0; i < list->size(); i++) {
        CXMLParserElement element = list->at(i);
        CDBDebug("Element %s", element.getName().c_str());
    }
}




bool CSLD::parameterIsSld(CT::string param) {
    if(param.equals(this->parameterName)){
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



