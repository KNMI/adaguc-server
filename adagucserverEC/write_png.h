#ifndef WRITE_PNG_H
#define WRITE_PNG_H

int writePng(int width, int height, unsigned char *ARGBByteBuffer, FILE *file, int bitDepth, bool use8bitpalAlpha);

#endif