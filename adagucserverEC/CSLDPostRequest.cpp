//
// Created by bennekom on 11/16/18.
//

#include "CSLDPostRequest.h"

const char *CSLDPostRequest::className = "CSLDPostRequest";

CSLDPostRequest::CSLDPostRequest(){

}

int CSLDPostRequest::startProcessing(CT::string *post_body) {
  CDBDebug("HOI %s", post_body->c_str());

  return 0;
}
