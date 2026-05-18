#include "CDebugger.h"
#include "CTString.h"
#include "CServerConfig_CPPXSD.h"
#include "CServerParams.h"
#include "../hclasses/CXMLParser.h"
#include <string>

#define MAX_RULE_ELEMENTS 1
#define MAX_FILE_SIZE_ALLOWED 10000
#define SLD_PARAMETER_NAME "SLD"

#define RULE_RASTER_SYMBOLIZER "RasterSymbolizer"
#define RULE_RASTER_SYMBOLIZER_CHILD_COLOR_MAP "ColorMap"

#define RULE_MIN_SCALE_DENOMINATOR "MinScaleDenominator"
#define RULE_MAX_SCALE_DENOMINATOR "MaxScaleDenominator"

#define STYLE_NAME_TEMPLATE "AdagucSldStyle_"
#define LEGEND_NAME_TEMPLATE "AdagucSldLegend_"

class CSLD {

public:
  bool parameterIsSld(const std::string &param);

  void setServerParams(CServerParams *serverParams);

  int processSLDUrl(const std::string &sldUrl);

private:
  CServerConfig::XMLE_Configuration *serverConfig;
  CServerParams *serverParams;

  DEF_ERRORFUNCTION();

  int buildRasterSymbolizer(CXMLParserElement *childElement, CServerConfig::XMLE_Style *myOwnStyle);
  int buildColorMap(CXMLParserElement *element, CServerConfig::XMLE_Style *myOwnStyle);
  int buildScaleDenominator(CXMLParserElement *element, CServerConfig::XMLE_Style *myOwnStyle);
  int validateAndParseSLDElements(CXMLParserElement *element, CServerConfig::XMLE_Style *myOwnStyle);
};
