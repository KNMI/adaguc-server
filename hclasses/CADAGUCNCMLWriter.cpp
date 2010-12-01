#include "CADAGUCNCMLWriter.h"
const char *CADAGUCNCMLWriter::className="CADAGUCNCMLWriter";
CADAGUCNCMLWriter::CADAGUCNCMLWriter(){
 // debug("[CADAGUCNCMLWriter]",1);
  NCMLSet     = 0;
  ErrorRaised = 0;
  destncid    = -1;
  dNrOfMetadataAttributes = 0;
}

CADAGUCNCMLWriter::~CADAGUCNCMLWriter(){
//  debug("[/CADAGUCNCMLWriter]",1);
}

int CADAGUCNCMLWriter::SetMetaDataAttribute(const char* pszVariable,
                                            const char* pszAttributeName,
                                            const char* pszAttributeValue,
                                            const char *pszAttributeType){
  if(dNrOfMetadataAttributes>=MAX_METADATA_ATTRIBUTES-1){
    CDBError("SetMetaDataAttribute: Maximumun MetadataAttributes reached: define a larger number in 'MAX_METADATA_ATTRIBUTES'");
    return -1;
  }
  MetaDataAttribute_Variable[dNrOfMetadataAttributes].copy(pszVariable);
  MetaDataAttribute_Name[dNrOfMetadataAttributes].copy(pszAttributeName);
  MetaDataAttribute_Value[dNrOfMetadataAttributes].copy(pszAttributeValue);
  MetaDataAttribute_Type[dNrOfMetadataAttributes].copy(pszAttributeType);
  dNrOfMetadataAttributes++;
  return 0;
}


void CADAGUCNCMLWriter::SetNCMLFile(const char *_ncmlname){
  snprintf(NCMLFileName,MAX_STR_LEN,"%s",_ncmlname);
  NCMLSet=1;
}

void CADAGUCNCMLWriter::_SetAttribute(const char * pszVariable,
                                      const char * pszAttributeName,
                                      const char* pszAttributeValue,
                                      const char * pszAttributeType)
{
  int status;
  int varid=0;
  status = nc_redef(destncid);
  if(strncmp(pszVariable,"NC_GLOBAL",9)==0){
    varid=NC_GLOBAL;
  }

  if(varid==0){
    status = nc_inq_varid(destncid,pszVariable , &varid);
    if(status!=NC_NOERR){
      status = nc_def_var(destncid,pszVariable ,NC_CHAR ,0, NULL,&varid);
    }
  }

  int AtrrOK=NC_EBADTYPE;
  
  if(strncmp(pszAttributeType,"char",4)==0){
    signed char val[2];
    val[0] = atoi(pszAttributeValue);
    AtrrOK = nc_put_att_schar(destncid, 
                              varid, pszAttributeName,
                              NC_CHAR,1,
                              (const signed char*)val);
  }
  
  if(strncmp(pszAttributeType,"byte",4)==0){
    signed char val[2];
    val[0] = atoi(pszAttributeValue);
    AtrrOK = nc_put_att_schar(destncid, 
                              varid, pszAttributeName,
                              NC_BYTE,1,
			      (const signed char*)val);
  }
  if(strncmp(pszAttributeType,"ubyte",4)==0){
	  unsigned char val[2];
	  val[0] = atoi(pszAttributeValue);
	  AtrrOK = nc_put_att_ubyte(destncid, 
				    varid, pszAttributeName,
				    NC_UBYTE,1,
				    (const unsigned char*)val);
  }
  if(strncmp(pszAttributeType,"ushort",6)==0){
    unsigned short val=atoi(pszAttributeValue);
    AtrrOK = nc_put_att_ushort(destncid, 
                               varid, pszAttributeName,
                               NC_USHORT,1,
                               &val);
  }

  if(strncmp(pszAttributeType,"short",5)==0){
    short val=atoi(pszAttributeValue);
    AtrrOK = nc_put_att_short(destncid, 
                              varid, pszAttributeName,
                              NC_SHORT,1,
                              &val);
  }

  if(strncmp(pszAttributeType,"ushort",6)==0){
    unsigned short val=atoi(pszAttributeValue);
    AtrrOK = nc_put_att_ushort(destncid, 
                             varid, pszAttributeName,
                             NC_USHORT,1,
                             &val);
  }

  if(strncmp(pszAttributeType,"int",3)==0){
    int val=atoi(pszAttributeValue);
    AtrrOK = nc_put_att_int(destncid, 
                            varid, pszAttributeName,
                            NC_INT,1,
                            &val);
  }

  if(strncmp(pszAttributeType,"uint",4)==0){
    unsigned int val=atoi(pszAttributeValue);
    AtrrOK = nc_put_att_uint(destncid, 
                            varid, pszAttributeName,
                            NC_UINT,1,
                            &val);
  }

  if(strncmp(pszAttributeType,"float",5)==0){
    float val=atof(pszAttributeValue);
    AtrrOK = nc_put_att_float(destncid, 
                              varid, pszAttributeName,
                              NC_FLOAT,1,
                              &val);
  }
  if(strncmp(pszAttributeType,"double",6)==0){
    double val=atof(pszAttributeValue);
    AtrrOK = nc_put_att_double(destncid, 
                               varid, pszAttributeName,
                               NC_DOUBLE,1,
                               &val);
  }
  if(strncmp(pszAttributeType,"String",6)==0||
     strncmp(pszAttributeType,"string",6)==0){
    AtrrOK = nc_put_att_text(destncid, 
                             varid, pszAttributeName,
                             strlen(pszAttributeValue),
                             pszAttributeValue);
  }
  if(AtrrOK !=NC_NOERR){
    snprintf(szDebug,MAX_STR_LEN,"Unable to write attribute\n\t'%s#%s=%s'\n\twith type '%s'\n\t%s\n\tNote that Type should be one of 'char,byte,ubyte,short,int,float,double,String or string'",
             pszVariable,pszAttributeName,pszAttributeValue,pszAttributeType,nc_strerror(AtrrOK));
    CDBError(szDebug);
    ErrorRaised=1;
    return;
  }
  status = nc_enddef(destncid);
  if(status != NC_NOERR){
    CDBError("_SetAttribute: enddef");
  }

}

void CADAGUCNCMLWriter::GetNCMLAttributes(xmlNode * a_node)
{
  xmlNode *cur_node = NULL;

  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if(cur_node->name!=NULL)
        if(strncmp("variable",(char*)cur_node->name,8)==0){
        NCMLVarName=NULL;
        if(cur_node->properties->name!=NULL)
          if(cur_node->properties->children->content!=NULL)
            if(strncmp("name",(char*)cur_node->properties->name,4)==0)
              NCMLVarName=(char*)cur_node->properties->children->content;
        }
        if(strncmp("attribute",(char*)cur_node->name,9)==0){
          if(NCMLVarName==NULL)NCMLVarName=(char*)"NC_GLOBAL";
          if(NCMLVarName!=NULL){
            if(cur_node->properties->name!=NULL){
              xmlAttr*attributes=cur_node->properties;
              char * pszAttributeType=NULL,*pszAttributeName=NULL,*pszAttributeValue=NULL;
              while(attributes!=NULL){
                if(strncmp("name",(char*)attributes->name,4)==0)
                  pszAttributeName=(char*)attributes->children->content;
                if(strncmp("type",(char*)attributes->name,4)==0)
                  pszAttributeType=(char*)attributes->children->content;
                if(strncmp("value",(char*)attributes->name,5)==0)
                  pszAttributeValue=(char*)attributes->children->content;
                attributes=attributes->next;
              }
              if(pszAttributeType!=NULL&&pszAttributeName!=NULL&&pszAttributeValue!=NULL){
                //printf("%s#%s=%s\n",NCMLVarName,pszAttributeName,pszAttributeValue);
                _SetAttribute(NCMLVarName,pszAttributeName,pszAttributeValue,pszAttributeType);
              }
            }
          }
        }
    }
    GetNCMLAttributes(cur_node->children);
  }
}


int CADAGUCNCMLWriter::AttachNCML(){
  ErrorRaised=0;
  CDBDebug("[AttachNCML]",1);  
  if(NCMLSet!=0&&strlen(NCMLFileName)>1){
    if(destncid==-1) {
      CDBError("destncid not defined, use SetNetCDFID(int id);");
      return 1;
    }

    // Read the XML file and attach it to the netcdf file
    xmlDoc *doc = NULL;
    NCMLVarName =NULL;
    xmlNode *root_element = NULL;
    LIBXML_TEST_VERSION
        doc = xmlReadFile(NCMLFileName, NULL, 0);
    if (doc == NULL) {
      snprintf(szDebug,MAX_STR_LEN,"Could not parse file \"%s\"", NCMLFileName);
      CDBError(szDebug);
      return 1;
    }
    root_element = xmlDocGetRootElement(doc);
    GetNCMLAttributes(root_element);
    xmlFreeDoc(doc);
    xmlCleanupParser();
  }
  
  // Attach the provided metadata attributes
  for(int j=0;j<dNrOfMetadataAttributes;j++){
    _SetAttribute(MetaDataAttribute_Variable[j].c_str(),
                  MetaDataAttribute_Name[j].c_str(),
                  MetaDataAttribute_Value[j].c_str(),
                  MetaDataAttribute_Type[j].c_str());
  }
 
  if(ErrorRaised==1)return 1;
  CDBDebug("[/AttachNCML]",1);
  return 0;
}

int CADAGUCNCMLWriter::GetMetaDataAttribute(const char* pszVariable,const char* pszAttributeName,CT::string *AttributeValue,CT::string *AttributeType){
  for(int j=0;j<dNrOfMetadataAttributes;j++){
    if(MetaDataAttribute_Variable[j].match(pszVariable)==0){
      if(MetaDataAttribute_Name[j].match(pszAttributeName)==0){
        AttributeValue->copy(&MetaDataAttribute_Value[j]);
        AttributeType->copy(&MetaDataAttribute_Type[j]);
        return 0;
      }
    }
  }
  return 1;
}

