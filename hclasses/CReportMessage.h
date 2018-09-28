#ifndef CREPORTMESSAGE_H
#define CREPORTMESSAGE_H

#include "CTString.h"

class CReportMessage {
public:
    enum Severities { INFO, WARNING, ERROR };
    enum Categories { PROJECTION, GENERAL };
    CReportMessage(const CReportMessage &m);
    CReportMessage(CT::string message, Severities severity, Categories category, CT::string documentationLink);
    const CT::string to_string() const;
    CT::string getMessage() const;
    CT::string getSeverity() const;
    CT::string getCategory() const;
    CT::string getDocumentationLink() const;
protected:
private:
    const CT::string message;
    const Severities severity;
    const Categories category; 
    const CT::string documentationLink;
    static const char* severity_names[];
    static const char* category_names[];
};

#endif //CREPORTMESSAGE_H
