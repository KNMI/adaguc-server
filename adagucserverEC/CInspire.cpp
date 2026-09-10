#include "CInspire.h"
#include "CTString.h"
#include "CXMLParser.h"
#include "CHTTPTools.h"
#include "CDebugger.h"
#include <string>
#include <vector>
#ifdef ENABLE_INSPIRE

std::string CInspire::InspireMetadataFromCSW::toString() {
  std::string a = CT::printf("title:           \"%s\"\n"
                              "identifier:      \"%s\"\n"
                              "abstract:        \"%s\"\n"
                              "pointOfContact:  \"%s\"\n"
                              "voiceTelephone:  \"%s\"\n"
                              "organisationName:\"%s\"\n"
                              "email:           \"%s\"\n",
                              title.c_str(), identifier.c_str(), abstract.c_str(), pointOfContact.c_str(), voiceTelephone.c_str(), organisationName.c_str(), email.c_str());
  for (size_t j = 0; j < keywords.size(); j++) {
    CT::printfconcat(a, "keyword %zu:       \"%s\"\n", j, keywords[j].c_str());
  }
  return a;
}

std::string CInspire::getErrorMessage(int a) {
  if (a == CINSPIRE_HTTPGETERROR) return "INSPIRE HTTP GET FAILED";
  if (a == CINSPIRE_XMLPARSEERROR) return "INSPIRE XML INVALID";
  if (a == CINSPIRE_XMLELEMENTNOTFOUND) return "INSPIRE XML ELEMENT NOT FOUND";
  return "CINSPIRE_UKNOWN";
}

CInspire::InspireMetadataFromCSW CInspire::readInspireMetadataFromCSW(const char *cswService) {
  std::string xmlData;

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
    std::string message = CXMLParser::getErrorMessage(e);
    CDBError("Inspire CSW parsing failed: %s ", message.c_str());
    throw CINSPIRE_XMLPARSEERROR;

    // throw e;
  }
  CXMLParserElement *MD_DataIdentification = NULL;
  try {
    MD_DataIdentification = element.getThrows("GetRecordByIdResponse")->getThrows("MD_Metadata")->getThrows("identificationInfo")->getThrows("MD_DataIdentification");
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
