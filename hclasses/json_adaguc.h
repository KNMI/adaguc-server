#ifndef JSON_ADAGUC_H
#define JSON_ADAGUC_H

#include "CTString.h"
#include "json.hpp"
#include "CReportMessage.h"

using json = nlohmann::json;


/**
 * This file includes all ADAGUC specific to_json methods used by the json.hpp library.
 * Note that the to_json methods need to live in the same namespace as the namespace in which the datatypes are defined.
 * Documentation: https://github.com/nlohmann/json
 */

namespace CT {
  //CT::string
  void to_json(json& j, const string& s);
};

void to_json(json& j, const CReportMessage& m);

#endif //JSON_ADAGUC_H
