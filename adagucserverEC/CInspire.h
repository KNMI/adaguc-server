#ifndef CINSPIRE_H
#define CINSPIRE_H

#ifdef ENABLE_CURL
#define ENABLE_INSPIRE
#endif

#ifdef ENABLE_INSPIRE
#include <cstdio>
#include <string>
#include <vector>
#define CINSPIRE_HTTPGETERROR 1
#define CINSPIRE_XMLPARSEERROR 2
#define CINSPIRE_XMLELEMENTNOTFOUND 3

// Zie http://kademo.nl/gs2/inspire/ows?SERVICE=WMS&REQUEST=GetCapabilities

class CInspire {
public:
  /**
   * INSPIRE metadata structure
   */
  class InspireMetadataFromCSW {
  public:
    std::string title, identifier, abstract, pointOfContact, voiceTelephone, organisationName, email;
    std::vector<std::string> keywords;

    std::string toString();
  };

  std::string static getErrorMessage(int a);

  /** Read from given CSW service and fill in INSPIRE metadata structure
   * @param cswService The CSW service to read
   * @return INSPIRE metadata structure
   * throws character array with error message
   */
  InspireMetadataFromCSW static readInspireMetadataFromCSW(const char *cswService);
};
#endif
#endif
