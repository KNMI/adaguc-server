#ifndef THREADUTILS_H
#define THREADUTILS_H

// Returns the number of threads to use for parallelizable image rendering work,
// based on available hardware concurrency, clamped between 1 and a sane maximum.
int getNumRenderThreads();

#endif
