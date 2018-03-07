//
// Created by wagenaar on 3/7/18.
//

#ifndef ADAGUC_SERVER_EUNADICS_CREPORTER_H
#define ADAGUC_SERVER_EUNADICS_CREPORTER_H
#include <list>
#include "CTString.h"

class CReporter {

private:
  static CReporter *instance;
  std::list<CT::string> errorList;
  std::list<CT::string> warningList;
  std::list<CT::string> infoList;

public:
  static CReporter *getInstance();
  void addError(CT::string error);
  void addWarning(CT::string warning);
  void addInfo(CT::string infoMessage);
  CT::string generateReport();
};


#endif //ADAGUC_SERVER_EUNADICS_CREPORTER_H
