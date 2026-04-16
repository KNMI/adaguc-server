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
    element.parseData(xmlData);
  } catch (int e) {
    CT::string message = CXMLParser::getErrorMessage(e);
    CDBError("Inspire CSW parsing failed: %s ", message.c_str());
    throw CINSPIRE_XMLPARSEERROR;

    // throw e;
  }
  CXMLParserElement *MD_DataIdentification = NULL;
  try {
    MD_DataIdentification = element.getThrows("GetRecordByIdResponse")->getThrows("MD_Metadata")->getThrows("identificationInfo")->getThrows("MD_DataIdentification");
    // MD_DataIdentification = element.get("GetRecordByIdResponse")->getThrows("MD_Metadata")->getThrows("identificationInfo")->getThrows("SV_ServiceIdentification");
  } catch (int e) {
    throw CINSPIRE_XMLELEMENTNOTFOUND;
  }
  // Get title
  try {
    inspireMetadata.title = MD_DataIdentification->getThrows("citation")->getThrows("CI_Citation")->getThrows("title")->getThrows("CharacterString")->value;
  } catch (int e) {
  }

  // Get identifier
  try {
    inspireMetadata.identifier =
        MD_DataIdentification->getThrows("citation")->getThrows("CI_Citation")->getThrows("identifier")->getThrows("MD_Identifier")->getThrows("code")->getThrows("CharacterString")->value;
  } catch (int e) {
  }
  // Get abstract
  try {
    inspireMetadata.abstract = MD_DataIdentification->getThrows("abstract")->getThrows("CharacterString")->value;
  } catch (int e) {
  }
  // Get point of contact
  try {
    inspireMetadata.pointOfContact = MD_DataIdentification->getThrows("pointOfContact")->getThrows("CI_ResponsibleParty")->getThrows("individualName")->getThrows("CharacterString")->value;
  } catch (int e) {
  }
  // Get organisation name
  try {
    inspireMetadata.organisationName = MD_DataIdentification->getThrows("pointOfContact")->getThrows("CI_ResponsibleParty")->getThrows("organisationName")->getThrows("CharacterString")->value;
  } catch (int e) {
  }

  // Get mail address
  try {
    inspireMetadata.email = MD_DataIdentification->getThrows("pointOfContact")
                                ->getThrows("CI_ResponsibleParty")
                                ->getThrows("contactInfo")
                                ->getThrows("CI_Contact")
                                ->getThrows("address")
                                ->getThrows("CI_Address")
                                ->getThrows("electronicMailAddress")
                                ->getThrows("CharacterString")
                                ->value.c_str();
  } catch (int e) {
  }

  // Get voiceTelephone
  try {
    inspireMetadata.voiceTelephone = MD_DataIdentification->getThrows("pointOfContact")
                                         ->getThrows("CI_ResponsibleParty")
                                         ->getThrows("contactInfo")
                                         ->getThrows("CI_Contact")
                                         ->getThrows("phone")
                                         ->getThrows("CI_Telephone")
                                         ->getThrows("voice")
                                         ->getThrows("CharacterString")
                                         ->value.c_str();
  } catch (int e) {
  }

  // Get keywords
  try {
    auto keyWordList = MD_DataIdentification->getThrows("descriptiveKeywords")->getThrows("MD_Keywords")->getList("keyword");
    for (size_t j = 0; j < keyWordList.size(); j++) {
      inspireMetadata.keywords.push_back(keyWordList[j].getThrows("CharacterString")->value);
    }
  } catch (int e) {
  }

  return inspireMetadata;
}
#endif
