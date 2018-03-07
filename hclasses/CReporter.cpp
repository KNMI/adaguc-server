#include "CReporter.h"

CReporter *CReporter::instance = NULL;

CReporter *CReporter::getInstance() {

  if (instance == NULL) {
    instance = new CReporter();
  }

  return instance;
}

void CReporter::addError(CT::string error) {
  errorList.push_back(error);
}

void CReporter::addWarning(CT::string warning) {
  warningList.push_back(warning);
}

void CReporter::addInfo(CT::string infoMessage) {
  infoList.push_back(infoMessage);
}

CT::string CReporter::generateReport() {

  CT::string report = CT::string("Errors: \n");

  for (std::list<CT::string>::iterator it=errorList.begin(); it != errorList.end(); ++it) {
     report.concat(*it);
     report.concat(CT::string("\n"));
  }

  report.concat("\nWarnings: \n");

  for (std::list<CT::string>::iterator it=warningList.begin(); it != warningList.end(); ++it) {
    report.concat(*it);
    report.concat(CT::string("\n"));
  }


  report.concat("\nInfo: \n");

  for (std::list<CT::string>::iterator it=infoList.begin(); it != infoList.end(); ++it) {
    report.concat(*it);
    report.concat(CT::string("\n"));
  }

  return report;
}
