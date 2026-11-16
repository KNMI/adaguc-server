#ifndef WRITE_WEBP_H
#define WRITE_WEBP_H

#include <cstdio>

int writeWebP(int width, int height, const unsigned char *ARGBByteBuffer, FILE *file, int quality);

#endif
