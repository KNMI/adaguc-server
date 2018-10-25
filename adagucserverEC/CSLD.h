#include "CDebugger.h"
#include "CTString.h"
#include "CServerConfig_CPPXSD.h"
#include "../hclasses/CXMLParser.h"

class CSLD {

    struct XML_RULE_FEATURES {
        static CT::string RASTER_SYMBOLIZER_NAME;
    };

    public:
        CSLD();

    private:
        CT::string parameterName;
        CServerConfig::XMLE_Configuration *serverConfig;
        CXMLParserElement *xmlParserElement;
        DEF_ERRORFUNCTION();

    public:
        bool parameterIsSld(CT::string param);
        bool serverConfigCheck(CServerConfig::XMLE_Configuration *serverConfig);

        void setServerConfig(CServerConfig::XMLE_Configuration *serverConfig);
        void extractSLD(CT::string prop);
        void buildRasterSymbolizer(CXMLParserElement *element);

        int validateSLDFile(CT::string fileName);
};

#define FEATURE_RASTER_SYMBOLIZER_NAME XML_RULE_FEATURES::RASTER_SYMBOLIZER_NAME
