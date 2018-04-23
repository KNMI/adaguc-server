#include "json_adaguc.h"

namespace CT {
  void to_json(json& j, const string& s) {
    j = json(s.c_str());
  }
}

void to_json(json& j, const CReportMessage& m) {
  j = {
    {"message", m.getMessage()},
    {"severity", m.getSeverity()},
    {"category", m.getCategory()},
    {"documentationLink", m.getDocumentationLink()}
  };
}
