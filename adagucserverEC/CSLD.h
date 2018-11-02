#include "CDebugger.h"
#include "CTString.h"
#include "CServerConfig_CPPXSD.h"
#include "CServerParams.h"
#include "../hclasses/CXMLParser.h"

#define MAX_RULE_ELEMENTS 1


class CSLD {

  static CT::string STYLE_NAME_TEMPLATE;

  struct XML_RULES {
    struct MIN_SCALE_DENOMINATOR {
      static CT::string NAME;
    };

    struct MAX_SCALE_DENOMINATOR {
      static CT::string NAME;
    };

    struct RASTER_SYMBOLIZER {
      static CT::string NAME;

      struct CHILDS {
        static CT::string COLOR_MAP;
      };
    };
  };


public:
  CSLD();

private:
  CT::string parameterName;
  CServerConfig::XMLE_Configuration *serverConfig;
  CServerParams *serverParams;
  CServerConfig::XMLE_Style *myOwnStyle;
  CServerConfig::XMLE_Legend *myOwnLegend;

  bool sldBuildComplete;

  DEF_ERRORFUNCTION();

  int buildRasterSymbolizer(CXMLParserElement *childElement);
  int buildColorMap(CXMLParserElement *element);
  int buildScaleDenominator(CXMLParserElement *element);
  int validateSLDElements(CXMLParserElement *element);

public:
  bool parameterIsSld(CT::string param);
  bool serverConfigCheck(CServerConfig::XMLE_Configuration *serverConfig);

  void setServerParams(CServerParams *serverParams);

  int processSLD(CT::string fileName);
};

#define XML_RULES_MIN_SCALE_DENOMINATOR_NAME XML_RULES::MIN_SCALE_DENOMINATOR::NAME
#define XML_RULES_MAX_SCALE_DENOMINATOR_NAME XML_RULES::MAX_SCALE_DENOMINATOR::NAME


#define XML_RULES_RASTER_SYMBOLIZER_NAME XML_RULES::RASTER_SYMBOLIZER::NAME
#define XML_RULES_RASTER_SYMBOLIZER_CHILD_COLOR_MAP XML_RULES::RASTER_SYMBOLIZER::CHILDS::COLOR_MAP
