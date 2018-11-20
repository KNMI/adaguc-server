//
// Created by bennekom on 11/16/18.
//

#ifndef PROJECT_CSLDPOSTREQUEST_H
#define PROJECT_CSLDPOSTREQUEST_H
#endif PROJECT_CSLDPOSTREQUEST_H

#include "CDebugger.h"
#include "CTString.h"

class CSLDPostRequest {

public:
  CSLDPostRequest();

  static int startProcessing(CT::string * post_body);

private:
  DEF_ERRORFUNCTION();
};




