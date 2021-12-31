#include "CReportMessage.h"

const char *CReportMessage::severity_names[] = {"INFO", "WARNING", "ERROR"};
const char *CReportMessage::category_names[] = {"PROJECTION", "GENERAL"};

CReportMessage::CReportMessage(const CReportMessage &m) : message(m.message), severity(m.severity), category(m.category), documentationLink(m.documentationLink) {
  /* No implementation. */
}

CReportMessage::CReportMessage(CT::string message, Severities severity, Categories category, CT::string documentationLink)
    : message(message), severity(severity), category(category), documentationLink(documentationLink) {
  /* No implementation. */
}

CT::string CReportMessage::getMessage() const { return message; }

CT::string CReportMessage::getSeverity() const { return severity_names[severity]; }

CT::string CReportMessage::getCategory() const { return category_names[category]; }

CT::string CReportMessage::getDocumentationLink() const { return documentationLink; }

const CT::string CReportMessage::to_string() const { return message; }
