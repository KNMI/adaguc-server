/*
 * CCairoPlotter2.cpp
 *
 *  Created on: Nov 11, 2011
 *      Author: vreedede
 */
#include "CCairoPlotter.h"
#ifdef ADAGUC_USE_CAIRO

const char *CCairoPlotter::className="CCairoPlotter";

cairo_status_t writerFunc(void *closure, const unsigned char *data, unsigned int length) {
  FILE *fp=(FILE *)closure;
  int nrec=fwrite(data, length, 1, fp);
  if (nrec==1) {
    return CAIRO_STATUS_SUCCESS;
  }

  return CAIRO_STATUS_WRITE_ERROR;
}
#endif