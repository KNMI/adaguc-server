#include <tuple>
#include "BBOX.h"

bool operator<(BBOX a, BBOX b) { return std::make_tuple(a.bbox[0], a.bbox[1], a.bbox[2], a.bbox[3]) < std::make_tuple(b.bbox[0], b.bbox[1], b.bbox[2], b.bbox[3]); }

BBOX makeBBOX(double *bbox) {
  BBOX box;
  box.bbox[0] = bbox[0];
  box.bbox[1] = bbox[1];
  box.bbox[2] = bbox[2];
  box.bbox[3] = bbox[3];
  return box;
}