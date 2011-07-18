#ifndef CSTOPWATCH_H
#define CSTOPWATCH_H
#include <stdlib.h>
#include <time.h>
#include "CDebugger.h"
#include <stdio.h>
#include <stdarg.h>

void StopWatch_Start();
void _StopWatch_Stop(const char *a, ...);
#define StopWatch_Stop  _printDebug("[D: %s, %d in %s] ",__FILE__,__LINE__,className);_StopWatch_Stop
#endif
