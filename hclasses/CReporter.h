//
// Created by wagenaar on 3/7/18.
//

#ifndef CREPORTER_H
#define CREPORTER_H
#include <list>
#include "CTString.h"
#include "CReportMessage.h"

// Set this to false if you don't want report messages in the log file also.
#define REPORT_AND_LOG true

#define REPORT_DEFAULT_FILE "./checker_report.txt"

#define CREPORT_INFO(message, category, documentationLink)                                                                                                                                             \
  CReporter::getInstance()->addMessage(message, CReportMessage::Severities::INFO, category, documentationLink, __FILENAME__, __LINE__, className)
#define CREPORT_INFO_NODOC(message, category) CReporter::getInstance()->addMessage(message, CReportMessage::Severities::INFO, category, "", __FILENAME__, __LINE__, className)
#define CREPORT_WARN(message, category, documentationLink)                                                                                                                                             \
  CReporter::getInstance()->addMessage(message, CReportMessage::Severities::WARNING, category, documentationLink, __FILENAME__, __LINE__, className)
#define CREPORT_WARN_NODOC(message, category) CReporter::getInstance()->addMessage(message, CReportMessage::Severities::WARNING, category, "", __FILENAME__, __LINE__, className)
#define CREPORT_ERROR(message, category, documentationLink)                                                                                                                                            \
  CReporter::getInstance()->addMessage(message, CReportMessage::Severities::ERROR, category, documentationLink, __FILENAME__, __LINE__, className)
#define CREPORT_ERROR_NODOC(message, category) CReporter::getInstance()->addMessage(message, CReportMessage::Severities::ERROR, category, "", __FILENAME__, __LINE__, className)

class CReporter {

private:
  static CReporter *instance;
  std::list<CReportMessage> messageList;
  void writeMessageToLog(const CT::string message, CReportMessage::Severities severity, const char *file, int line, const char *className) const;
  bool writelog = false;
  CT::string _filename;

protected:
  CReporter(bool report_and_log = false);

public:
  static CReporter *getInstance();
  const std::list<CReportMessage> getMessageList() const;
  void addMessage(const CT::string message, CReportMessage::Severities severity, CReportMessage::Categories category, CT::string documentationLink, const char *file = "", int line = -1,
                  const char *className = "");
  void filename(const CT::string filename);
  CT::string filename() const;
};

#endif // CREPORTER_H
