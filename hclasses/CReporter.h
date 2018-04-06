//
// Created by wagenaar on 3/7/18.
//

#ifndef ADAGUC_SERVER_EUNADICS_CREPORTER_H
#define ADAGUC_SERVER_EUNADICS_CREPORTER_H
#include <list>
#include "CTString.h"

// Set this to false if you don't want report messages in the log file also.
#define REPORT_AND_LOG true 

#define CREPORT_WARN(message) CReporter::getInstance()->addWarning(message, __FILE__, __LINE__, className)
#define CREPORT_ERROR(message) CReporter::getInstance()->addError(message, __FILE__, __LINE__, className)
#define CREPORT_INFO(message) CReporter::getInstance()->addInfo(message, __FILE__, __LINE__, className)

class CReporter {

private:
  static CReporter *instance;
  std::list<CT::string> errorList;
  std::list<CT::string> warningList;
  std::list<CT::string> infoList;
  bool writelog = false;

protected:
  CReporter(bool report_and_log=false);
  
public:
  static CReporter *getInstance();
  void addError(const CT::string error, const char* file="", int line=-1, const char* className="");
  void addWarning(const CT::string warning, const char* file="", int line=-1, const char* className="");
  void addInfo(const CT::string infoMessage, const char* file="", int line=-1, const char* className="");
  CT::string generateReport();

  int writeReport(const CT::string reportfilename);
};


#endif //ADAGUC_SERVER_EUNADICS_CREPORTER_H
