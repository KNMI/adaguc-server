#ifndef CINSPIRE_H
#define CINSPIRE_H
#include <stdio.h>
#include "CTypes.h"
#include "CXMLParser.h"
#include "CHTTPTools.h"

#define CINSPIRE_HTTPGETERROR 1
#define CINSPIRE_XMLPARSEERROR 2
#define CINSPIRE_XMLELEMENTNOTFOUND 3


//Zie http://kademo.nl/gs2/inspire/ows?SERVICE=WMS&REQUEST=GetCapabilities

class CInspire{
private:
  DEF_ERRORFUNCTION();
public:
  /**
   * INSPIRE metadata structure
   */
  class InspireMetadataFromCSW{
    public:
    CT::string title,identifier,abstract,pointOfContact,voiceTelephone,organisationName,email;
    std::vector<CT::string> keywords;
    
    CT::string toString(){
      CT::string a;
      a.print(
      "title:           \"%s\"\n"
      "identifier:      \"%s\"\n"
      "abstract:        \"%s\"\n"
      "pointOfContact:  \"%s\"\n"
      "voiceTelephone:  \"%s\"\n"
      "organisationName:\"%s\"\n"
      "email:           \"%s\"\n",title.c_str(),identifier.c_str(),abstract.c_str(),pointOfContact.c_str(),voiceTelephone.c_str(),organisationName.c_str(),email.c_str());
      for(size_t j=0;j<keywords.size();j++){
        a.printconcat("keyword %d:       \"%s\"\n",j,keywords[j].c_str());
      }
      return a;
    }
  };

  CT::string static getErrorMessage(int a){
    if(a == CINSPIRE_HTTPGETERROR)return "INSPIRE HTTP GET FAILED";
    if(a == CINSPIRE_XMLPARSEERROR)return "INSPIRE XML INVALID";
    if(a == CINSPIRE_XMLELEMENTNOTFOUND)return "INSPIRE XML ELEMENT NOT FOUND";
    return "CINSPIRE_UKNOWN";
  }
  
  /** Read from given CSW service and fill in INSPIRE metadata structure
   * @param cswService The CSW service to read
   * @return INSPIRE metadata structure
   * throws character array with error message
   */
  InspireMetadataFromCSW static readInspireMetadataFromCSW(const char * cswService){
    CT::string xmlData;
    
    try{
      xmlData=CHTTPTools::getString(cswService);
    }catch(int e){
      throw CINSPIRE_HTTPGETERROR;
    }
  
    CXMLParserElement element;

    InspireMetadataFromCSW inspireMetadata;
    
    
    try{
      element.parse(xmlData);
    }catch(int e){
      CT::string message=CXMLParser::getErrorMessage(e);
      CDBError("Inspire CSW parsing failed: %s ",message.c_str());
      throw CINSPIRE_XMLPARSEERROR;
      
      //throw e;
    }
      CXMLParserElement *MD_DataIdentification = NULL;
      try{
        MD_DataIdentification = element.get("GetRecordByIdResponse")->get("MD_Metadata")->get("identificationInfo")->get("MD_DataIdentification");
      }catch(int e){
        throw CINSPIRE_XMLELEMENTNOTFOUND;
      }
      //Get title
      try{
        inspireMetadata.title = MD_DataIdentification->get("citation")->get("CI_Citation")->get("title")->get("CharacterString")->getValue().c_str();
      }catch(int e){}
      
      //Get identifier
      try{
        inspireMetadata.identifier = MD_DataIdentification->get("citation")->get("CI_Citation")->get("identifier")->get("MD_Identifier")->get("code")->get("CharacterString")->getValue().c_str();
      }catch(int e){}
      //Get abstract
      try{
        inspireMetadata.abstract = MD_DataIdentification->get("abstract")->get("CharacterString")->getValue().c_str();
      }catch(int e){}
      //Get point of contact
      try{
        inspireMetadata.pointOfContact = MD_DataIdentification->get("pointOfContact")->get("CI_ResponsibleParty")->get("individualName")->get("CharacterString")->getValue().c_str();
      }catch(int e){}
      //Get organisation name
      try{
        inspireMetadata.organisationName = MD_DataIdentification->get("pointOfContact")->get("CI_ResponsibleParty")->get("organisationName")->get("CharacterString")->getValue().c_str();
      }catch(int e){}
      
      //Get mail address
      try{
        inspireMetadata.email = MD_DataIdentification->get("pointOfContact")->get("CI_ResponsibleParty")->get("contactInfo")->get("CI_Contact")->get("address")->get("CI_Address")->get("electronicMailAddress")->get("CharacterString")->getValue().c_str();
      }catch(int e){}
      
      //Get voiceTelephone
      try{
        inspireMetadata.voiceTelephone = MD_DataIdentification->get("pointOfContact")->get("CI_ResponsibleParty")->get("contactInfo")->get("CI_Contact")->get("phone")->get("CI_Telephone")->get("voice")->get("CharacterString")->getValue().c_str();
      }catch(int e){}
      
      //Get keywords
      try{
        CXMLParser::XMLElement::XMLElementPointerList keyWordList = MD_DataIdentification->get("descriptiveKeywords")->get("MD_Keywords")->getList("keyword");
        for(size_t j=0;j<keyWordList.size();j++){
          inspireMetadata.keywords.push_back(keyWordList[j]->get("CharacterString")->getValue().c_str());
        }
      }catch(int e){}
   
    return inspireMetadata;
    
  }
};
#endif
