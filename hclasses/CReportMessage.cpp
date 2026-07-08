#include "CReportMessage.h"

const char *CReportMessage::severity_names[] = {"INFO", "WARNING", "ERROR"};
const char *CReportMessage::category_names[] = {"PROJECTION", "GENERAL"};

CReportMessage::CReportMessage(const CReportMessage &m) : message(m.message), severity(m.severity), category(m.category), documentationLink(m.documentationLink) {
  /* No implementation. */
}

CReportMessage::CReportMessage(std::string message, Severities severity, Categories category, std::string documentationLink)
    : message(message), severity(severity), category(category), documentationLink(documentationLink) {
  /* No implementation. */
}

std::string CReportMessage::getMessage() const { return message; }

std::string CReportMessage::getSeverity() const { return severity_names[severity]; }

std::string CReportMessage::getCategory() const { return category_names[category]; }

std::string CReportMessage::getDocumentationLink() const { return documentationLink; }

const std::string CReportMessage::to_string() const { return message; }
