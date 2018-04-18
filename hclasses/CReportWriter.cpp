#include "CReportWriter.h"
#include "CReporter.h"

#include <fstream>

#include "json.hpp"
#include "json_adaguc.h"

using json = nlohmann::json;

namespace CReportWriter {

  bool writeJSONReportToFile(const CT::string reportFilename) {
    json report;
    CReporter *cReporter = CReporter::getInstance();

    report["error"] = json(cReporter->getErrorList());
    report["info"] = json(cReporter->getInfoList());
    report["warning"] = json(cReporter->getWarningList());

    std::ofstream reportfile(reportFilename.c_str());
    if(!reportfile) {
      return false;
    }

    reportfile << report.dump(4).c_str();
    reportfile << std::endl;
    reportfile.close();

    return true;
  }
}