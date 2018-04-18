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
                  << error << std::endl;
        _printErrorStreamPointer(error_str.str().c_str());
    }
    errorList.push_back(error);
}

void CReporter::addWarning(const CT::string warning, const char* file, int line, const char* className) {
    if (this->writelog) {
        std::ostringstream warn_str;
        warn_str << "[W:" << file << ", " << " " << line << " in " << className << "] "
                 << warning << std::endl;
        _printWarningStreamPointer(warn_str.str().c_str());
    }
    warningList.push_back(warning);
}

void CReporter::addInfo(const CT::string infoMessage, const char* file, int line, const char* className) {
    if (this->writelog) {
        std::ostringstream dbg_str;
        dbg_str << "[D:" << file << ", " << " " << line << " in " << className << "] "
                << infoMessage << std::endl;
        _printDebugStreamPointer(dbg_str.str().c_str());
    }
    infoList.push_back(infoMessage);
}

const std::list<CT::string> CReporter::getErrorList() const {
  return errorList;
}

const std::list<CT::string> CReporter::getWarningList() const {
  return warningList;
}

const std::list<CT::string> CReporter::getInfoList() const {
  return infoList;
}
