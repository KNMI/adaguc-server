#ifndef WRITE_PNG32_H
#define WRITE_PNG32_H

#include <cstdio>
#include <cairo.h>

// 32-bit RGBA via Cairo's own PNG writer; expects a premultiplied ARGB32 surface.
int writePng32(cairo_surface_t *surface, FILE *file);

#endif
