#include "CReporter.h"
#include "CDebugger_H2.h"

#include <fstream>
#include <sstream>
#include <string>

using Severities = CReportMessage::Severities;
using Categories = CReportMessage::Categories;

extern unsigned int logMessageNumber;
extern unsigned long logProcessIdentifier;

CReporter *CReporter::instance = NULL;

CReporter::CReporter(bool report_and_log)
    : messageList(), writelog(report_and_log), _filename()
{
  // Empty at this point
}

CReporter *CReporter::getInstance()
{
  if (instance == NULL)
  {
    if (REPORT_AND_LOG == true)
      instance = new CReporter(REPORT_AND_LOG);
    else
      instance = new CReporter();
  }

  return instance;
}

void CReporter::addMessage(const CT::string message, CReportMessage::Severities severity,
                           CReportMessage::Categories category, CT::string documentationLink, const char *file,
                           int line, const char *className)
{

  if (this->writelog)
  {
    writeMessageToLog(message, severity, file, line, className);
  }

  CReportMessage _message(message, severity, category, documentationLink);
  messageList.push_back(_message);
}

void CReporter::writeMessageToLog(const CT::string message, CReportMessage::Severities severity, const char *file,
                                  int line, const char *className) const
{

  if (severity == Severities::INFO)
  {
    _printDebug("[D:%03d:pid%lu: %s, %d in %s] ", logMessageNumber, logProcessIdentifier, file, line, className);
    _printDebugLine(message);
  }

  if (severity == Severities::WARNING)
  {
    _printWarning("[W:%03d:pid%lu: %s, %d in %s] ", logMessageNumber, logProcessIdentifier, file, line, className);
    _printWarningLine(message);
  }

  if (severity == Severities::ERROR)
  {
    _printError("[E:%03d:pid%lu: %s, %d in %s] ", logMessageNumber, logProcessIdentifier, file, line, className);
    _printErrorLine(message);
  }
}

const std::list<CReportMessage> CReporter::getMessageList() const
{
  return messageList;
}

void CReporter::filename(const CT::string filename)
{
  this->_filename = filename;
}

CT::string CReporter::filename() const
{
  return this->_filename;
}
