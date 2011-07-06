#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "netcdf.h"
#include "CDefinitions.h"
#include "CTypes.h"
#define MAX_METADATA_ATTRIBUTES 512
#include "CDebugger.h"

class CADAGUCNCMLWriter{
  private:
    DEF_ERRORFUNCTION();
    int destncid;
    char *NCMLVarName;
    char NCMLFileName[MAX_STR_LEN];
    char szDebug[MAX_STR_LEN];
    int ErrorRaised;
    int NCMLSet;
    void GetNCMLAttributes(xmlNode * a_node);
    void _SetAttribute(const char * pszVariable,
                      const char * pszAttributeName,
                      const char* pszAttributeValue,
                      const char * pszAttributeType);

    int dNrOfMetadataAttributes;
    CT::string MetaDataAttribute_Variable [MAX_METADATA_ATTRIBUTES];
    CT::string MetaDataAttribute_Name[MAX_METADATA_ATTRIBUTES];
    CT::string MetaDataAttribute_Value    [MAX_METADATA_ATTRIBUTES];
    CT::string MetaDataAttribute_Type    [MAX_METADATA_ATTRIBUTES];
  public:

    CADAGUCNCMLWriter();
    ~CADAGUCNCMLWriter();
    void SetAttributeDirectly(const char * pszVariable,
                              const char * pszAttributeName,
                              const char* pszAttributeValue,
                              const char * pszAttributeType){
                                _SetAttribute(pszVariable,
                                  pszAttributeName,
                                  pszAttributeValue,
                                  pszAttributeType);
                              }
    void SetNCMLFile(const char *_ncmlname);
    void SetNetCDFId(int _id){destncid=_id;};
    int SetMetaDataAttribute(const char* pszVariable,const char* pszAttributeName,const char* pszAttributeValue,const char *pszAttributeType);
    int AttachNCML();
    int GetMetaDataAttribute(const char* pszVariable,const char* pszAttributeName,CT::string *AttributeValue,CT::string *AttributeType);
};

