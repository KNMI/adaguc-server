#include "CReporter.h"
#include "CDebugger_H2.h"

#include <fstream>
#include <sstream>
#include <string>

/*
 * These function pointers are declared, defined and initialized
 * in CDebugger.cpp.
 */
extern void (*_printErrorStreamPointer)(const char*);
extern void (*_printWarningStreamPointer)(const char*);
extern void (*_printDebugStreamPointer)(const char*);

CReporter *CReporter::instance = NULL;

CReporter::CReporter(bool report_and_log) : errorList(), warningList(), infoList(),
                                            writelog(report_and_log)
{
    // Empty at this point
}

CReporter *CReporter::getInstance() {
  if (instance == NULL) {
      if (REPORT_AND_LOG == true)
          instance = new CReporter(REPORT_AND_LOG);
      else
          instance = new CReporter();
  }

  return instance;
}

void CReporter::addError(const CT::string error, const char* file, int line, const char* className) {
    if (this->writelog) {
        std::ostringstream error_str;
        error_str << "[E:" << file << ", " << " " << line << " in " << className << "] "
                  << error.c_str() << std::endl;
        _printErrorStreamPointer(error_str.str().c_str());
    }
    errorList.push_back(error);
}

void CReporter::addWarning(const CT::string warning, const char* file, int line, const char* className) {
    if (this->writelog) {
        std::ostringstream warn_str;
        warn_str << "[W:" << file << ", " << " " << line << " in " << className << "] "
                 << warning.c_str() << std::endl;
        _printWarningStreamPointer(warn_str.str().c_str());
    }
    warningList.push_back(warning);
}

void CReporter::addInfo(const CT::string infoMessage, const char* file, int line, const char* className) {
    if (this->writelog) {
        std::ostringstream dbg_str;
        dbg_str << "[D:" << file << ", " << " " << line << " in " << className << "] "
                << infoMessage.c_str() << std::endl;
        _printDebugStreamPointer(dbg_str.str().c_str());
    }
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

int CReporter::writeReport(const CT::string reportfilename) {
    std::ofstream reportfile(reportfilename.c_str());
    if (!reportfile) {
        return -1;
    }
    reportfile << this->generateReport().c_str();
    reportfile.close();
    return 0;
}
