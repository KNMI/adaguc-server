#include "json_adaguc.h"

namespace CT {
  void to_json(json& j, const string& s) {
    j = json(s.c_str());
  }
}
