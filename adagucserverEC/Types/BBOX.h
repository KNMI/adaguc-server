#ifndef BBOX_H
#define BBOX_H

struct BBOX {
  double bbox[4];
};

/**
 * Custom comparison operator for the BBOX
 */
bool operator<(BBOX a, BBOX b);

/**
 * Makes a BBOX from a array of doubles
 */
BBOX makeBBOX(double *bbox);

#endif // !  BBOX_H
