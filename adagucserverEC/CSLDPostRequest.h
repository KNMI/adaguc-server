//
// Created by bennekom on 11/16/18.
//

#include "CDebugger.h"
#include "CTString.h"

class CSLDPostRequest {

public:
  CSLDPostRequest();

  static int startProcessing(CT::string * post_body);

private:
  DEF_ERRORFUNCTION();
};




