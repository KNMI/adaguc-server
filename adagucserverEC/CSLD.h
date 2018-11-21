#include "CDebugger.h"
#include "CTString.h"
#include "CServerConfig_CPPXSD.h"
#include "CServerParams.h"
#include "../hclasses/CXMLParser.h"

#define MAX_RULE_ELEMENTS 1
#define SLD_PARAMETER_NAME "SLD"

#define RULE_RASTER_SYMBOLIZER  "RasterSymbolizer"
#define RULE_RASTER_SYMBOLIZER_CHILD_COLOR_MAP "ColorMap"

#define RULE_MIN_SCALE_DENOMINATOR "MinScaleDenominator"
#define RULE_MAX_SCALE_DENOMINATOR "MaxScaleDenominator"

#define STYLE_NAME_TEMPLATE "AdagucSld_"

class CSLD {

public:
  CSLD();

  bool parameterIsSld(CT::string param);

  void setServerParams(CServerParams *serverParams);

  int processSLDUrl(CT::string sldUrl);

private:
  CServerConfig::XMLE_Configuration *serverConfig;
  CServerParams *serverParams;
  CServerConfig::XMLE_Style *myOwnStyle;
  CServerConfig::XMLE_Legend *myOwnLegend;

  DEF_ERRORFUNCTION();

  int buildRasterSymbolizer(CXMLParserElement *childElement);
  int buildColorMap(CXMLParserElement *element);
  int buildScaleDenominator(CXMLParserElement *element);
  int validateSLDElements(CXMLParserElement *element);
};