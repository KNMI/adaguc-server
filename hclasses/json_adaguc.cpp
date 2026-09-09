#include "json_adaguc.h"

void to_json(json &j, const CReportMessage &m) { j = {{"message", m.getMessage()}, {"severity", m.getSeverity()}, {"category", m.getCategory()}, {"documentationLink", m.getDocumentationLink()}}; }
