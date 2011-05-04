#include "CStopWatch.h"

//Stopwatch functions for timing
DEF_ERRORMAIN();
timespec starttime,stoptime,currenttime;
double CSTOPWATCH_H_prevTime = 0;
int content_type_provided = 0;
int firstTime=0;
void StopWatch_Start(){
  clock_gettime(CLOCK_REALTIME, &starttime);
  double start,stop;
  start = double(starttime.tv_nsec)/1000000+starttime.tv_sec*1000;
  stop  = double(stoptime.tv_nsec)/1000000+stoptime.tv_sec*1000;
  CDBDebug("%.3f ms\t%.3f ms\t%s",stop-start,stop-CSTOPWATCH_H_prevTime ,"*** START ***");
  CSTOPWATCH_H_prevTime =stop;
}
void __StopWatch_Stop(const char *msg){
  clock_gettime(CLOCK_REALTIME, &stoptime);
  double start,stop;
  start = double(starttime.tv_nsec)/1000000+starttime.tv_sec*1000;
  stop  = double(stoptime.tv_nsec)/1000000+stoptime.tv_sec*1000;
  if(firstTime==0){
    CSTOPWATCH_H_prevTime =stop;
    firstTime=1;
  }
  _printDebugLine("[T] %.1f/%.3f ms: %s",stop-start,stop-CSTOPWATCH_H_prevTime ,msg);
  CSTOPWATCH_H_prevTime =stop;
  
}


void _StopWatch_Stop(const char *a, ...){
  va_list ap;
  char szTemp[8192+1];
  va_start (ap, a);
  vsnprintf (szTemp, 8192, a, ap);
  va_end (ap);
  szTemp[8192]='\0';
  __StopWatch_Stop(szTemp);
}
