#include "CDebugger.h"
#include "write_png32.h"

static cairo_status_t pngWriterFunc(void *closure, const unsigned char *data, unsigned int length) {
  FILE *fp = (FILE *)closure;
  int nrec = fwrite(data, length, 1, fp);
  if (nrec == 1) {
    return CAIRO_STATUS_SUCCESS;
  }
  return CAIRO_STATUS_WRITE_ERROR;
}

int writePng32(cairo_surface_t *surface, FILE *file) {
  cairo_surface_flush(surface);
  if (cairo_surface_write_to_png_stream(surface, pngWriterFunc, (void *)file) != CAIRO_STATUS_SUCCESS) {
    CDBError("cairo_surface_write_to_png_stream failed");
    return 1;
  }
  return 0;
}
