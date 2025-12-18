#include "CReportWriter.h"
#include "CReporter.h"

#include <fstream>

#include "json.hpp"
#include "json_adaguc.h"

using json = nlohmann::json;

namespace CReportWriter {

  bool writeJSONReportToFile() {
    json report;
    CReporter *cReporter = CReporter::getInstance();

    if (cReporter->filename().empty()) return false;
    report["messages"] = json(cReporter->getMessageList());

    std::ofstream reportfile(cReporter->filename().c_str());
    if (!reportfile) {
      return false;
    }

    reportfile << report.dump(4).c_str();
    reportfile << std::endl;
    reportfile.close();

    return true;
  }
} // namespace CReportWriter
