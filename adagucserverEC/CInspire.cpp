#include "CInspire.h"
#ifdef ENABLE_INSPIRE

CInspire::InspireMetadataFromCSW CInspire::readInspireMetadataFromCSW(const char *cswService) {
  CT::string xmlData;

  try {
    xmlData = CHTTPTools::getString(cswService);
  } catch (int e) {
    throw CINSPIRE_HTTPGETERROR;
  }

  CXMLParserElement element;

  InspireMetadataFromCSW inspireMetadata;

  try {
    element.parse(xmlData);
  } catch (int e) {
    CT::string message = CXMLParser::getErrorMessage(e);
    CDBError("Inspire CSW parsing failed: %s ", message.c_str());
    throw CINSPIRE_XMLPARSEERROR;

    // throw e;
  }
  CXMLParserElement *MD_DataIdentification = NULL;
  try {
    MD_DataIdentification = element.get("GetRecordByIdResponse")->get("MD_Metadata")->get("identificationInfo")->get("MD_DataIdentification");
    // MD_DataIdentification = element.get("GetRecordByIdResponse")->get("MD_Metadata")->get("identificationInfo")->get("SV_ServiceIdentification");
  } catch (int e) {
    throw CINSPIRE_XMLELEMENTNOTFOUND;
  }
  // Get title
  try {
    inspireMetadata.title = MD_DataIdentification->get("citation")->get("CI_Citation")->get("title")->get("CharacterString")->getValue().c_str();
  } catch (int e) {
  }

  // Get identifier
  try {
    inspireMetadata.identifier = MD_DataIdentification->get("citation")->get("CI_Citation")->get("identifier")->get("MD_Identifier")->get("code")->get("CharacterString")->getValue().c_str();
  } catch (int e) {
  }
  // Get abstract
  try {
    inspireMetadata.abstract = MD_DataIdentification->get("abstract")->get("CharacterString")->getValue().c_str();
  } catch (int e) {
  }
  // Get point of contact
  try {
    inspireMetadata.pointOfContact = MD_DataIdentification->get("pointOfContact")->get("CI_ResponsibleParty")->get("individualName")->get("CharacterString")->getValue().c_str();
  } catch (int e) {
  }
  // Get organisation name
  try {
    inspireMetadata.organisationName = MD_DataIdentification->get("pointOfContact")->get("CI_ResponsibleParty")->get("organisationName")->get("CharacterString")->getValue().c_str();
  } catch (int e) {
  }

  // Get mail address
  try {
    inspireMetadata.email = MD_DataIdentification->get("pointOfContact")
                                ->get("CI_ResponsibleParty")
                                ->get("contactInfo")
                                ->get("CI_Contact")
                                ->get("address")
                                ->get("CI_Address")
                                ->get("electronicMailAddress")
                                ->get("CharacterString")
                                ->getValue()
                                .c_str();
  } catch (int e) {
  }

  // Get voiceTelephone
  try {
    inspireMetadata.voiceTelephone = MD_DataIdentification->get("pointOfContact")
                                         ->get("CI_ResponsibleParty")
                                         ->get("contactInfo")
                                         ->get("CI_Contact")
                                         ->get("phone")
                                         ->get("CI_Telephone")
                                         ->get("voice")
                                         ->get("CharacterString")
                                         ->getValue()
                                         .c_str();
  } catch (int e) {
  }

  // Get keywords
  try {
    CXMLParser::XMLElement::XMLElementPointerList keyWordList = MD_DataIdentification->get("descriptiveKeywords")->get("MD_Keywords")->getList("keyword");
    for (size_t j = 0; j < keyWordList.size(); j++) {
      inspireMetadata.keywords.push_back(keyWordList[j]->get("CharacterString")->getValue().c_str());
    }
  } catch (int e) {
  }

  return inspireMetadata;
}
#endif
