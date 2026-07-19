#ifndef CREPORTMESSAGE_H
#define CREPORTMESSAGE_H

#include <string>

class CReportMessage {
public:
  enum Severities { INFO, WARNING, ERROR };
  enum Categories { PROJECTION, GENERAL };
  CReportMessage(const CReportMessage &m);
  CReportMessage(const std::string &message, Severities severity, Categories category, const std::string &documentationLink);
  const std::string to_string() const;
  std::string getMessage() const;
  std::string getSeverity() const;
  std::string getCategory() const;
  std::string getDocumentationLink() const;

protected:
private:
  const std::string message;
  const Severities severity;
  const Categories category;
  const std::string documentationLink;
  static const char *severity_names[];
  static const char *category_names[];
};

#endif
