#ifndef GDW_BILINEAR_H
#define GDW_BILINEAR_H

// Keep the source type: double grids multiply in double, but horizontal blends
// and the final result are rounded to float, exactly as in the scalar renderer.
template <typename T> inline float gdwBilinearValue(float dx, float dy, T v00, T v10, T v01, T v11) {
  float gx1 = (1 - dx) * v00 + dx * v10;
  float gx2 = (1 - dx) * v01 + dx * v11;
  return (1 - dy) * gx1 + dy * gx2;
}

template <typename T> void gdwBilinearBatch(float *output, const float *dx, const float *dy, const T *v00, const T *v10, const T *v01, const T *v11, int count) {
#ifdef ADAGUC_OPENMP_SIMD
#pragma omp simd
#endif
  for (int i = 0; i < count; ++i) output[i] = gdwBilinearValue(dx[i], dy[i], v00[i], v10[i], v01[i], v11[i]);
}

#endif
