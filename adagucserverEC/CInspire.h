#ifndef CINSPIRE_H
#define CINSPIRE_H

#ifdef ENABLE_CURL
#define ENABLE_INSPIRE
#endif

#ifdef ENABLE_INSPIRE
#include <stdio.h>
#include "CTString.h"
#include "CXMLParser.h"
#include "CHTTPTools.h"
#include "CDebugger.h"
#define CINSPIRE_HTTPGETERROR 1
#define CINSPIRE_XMLPARSEERROR 2
#define CINSPIRE_XMLELEMENTNOTFOUND 3

// Zie http://kademo.nl/gs2/inspire/ows?SERVICE=WMS&REQUEST=GetCapabilities

class CInspire {
private:
public:
  /**
   * INSPIRE metadata structure
   */
  class InspireMetadataFromCSW {
  public:
    CT::string title, identifier, abstract, pointOfContact, voiceTelephone, organisationName, email;
    std::vector<CT::string> keywords;

    CT::string toString() {
      CT::string a;
      a.print("title:           \"%s\"\n"
              "identifier:      \"%s\"\n"
              "abstract:        \"%s\"\n"
              "pointOfContact:  \"%s\"\n"
              "voiceTelephone:  \"%s\"\n"
              "organisationName:\"%s\"\n"
              "email:           \"%s\"\n",
              title.c_str(), identifier.c_str(), abstract.c_str(), pointOfContact.c_str(), voiceTelephone.c_str(), organisationName.c_str(), email.c_str());
      for (size_t j = 0; j < keywords.size(); j++) {
        a.printconcat("keyword %d:       \"%s\"\n", j, keywords[j].c_str());
      }
      return a;
    }
  };

  CT::string static getErrorMessage(int a) {
    if (a == CINSPIRE_HTTPGETERROR) return "INSPIRE HTTP GET FAILED";
    if (a == CINSPIRE_XMLPARSEERROR) return "INSPIRE XML INVALID";
    if (a == CINSPIRE_XMLELEMENTNOTFOUND) return "INSPIRE XML ELEMENT NOT FOUND";
    return "CINSPIRE_UKNOWN";
  }

  /** Read from given CSW service and fill in INSPIRE metadata structure
   * @param cswService The CSW service to read
   * @return INSPIRE metadata structure
   * throws character array with error message
   */
  InspireMetadataFromCSW static readInspireMetadataFromCSW(const char *cswService);
};
#endif
#endif
