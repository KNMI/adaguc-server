

#ifndef GDW_DRAWTRIANGLE_UTILS_H
#define GDW_DRAWTRIANGLE_UTILS_H

template <typename T>
int gdwDrawTriangle(double *xP, double *yP, T value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, T, void *settings, void *genericDataWarper), void *genericDataWarper,
                    bool aOrB);
#endif