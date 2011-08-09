#ifndef CADAGUC_time_H
#define CADAGUC_time_H
#include <udunits.h>

#include "CTypes.h"

#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include "CDebugger.h"

#define ADAGUC_TIME_MAX_STR_LEN 8192
struct stADAGUC_time {
  int     year, month, day, hour, minute;
  float   second;
  double  offset;
};

class CADAGUC_time{
  private:
    DEF_ERRORFUNCTION();
    int     year, month, day, hour, minute;
    float   second;
    char szTime[ADAGUC_TIME_MAX_STR_LEN+1],szTemp[ADAGUC_TIME_MAX_STR_LEN+1];
    char szTime_test_2[ADAGUC_TIME_MAX_STR_LEN+1];
  public:
    CADAGUC_time(const char * DateSince);
    ~CADAGUC_time();
    int HDF4DateTimeToADAGUC(stADAGUC_time  &ADtime, int date_in,int time_in);
    int YYYYMMDDTHHMMSSToADAGUC(stADAGUC_time  &ADtime,const char * szTime);
    // time format MUST BE YYYYMMDDTHHMMSS.ms
    int ISOTimeToADAGUC(stADAGUC_time  &ADtime,const char *szTime);
    int OffsetToAdaguc(stADAGUC_time  &ADtime, double offset);
    int AdagucToOffset( double &offset,stADAGUC_time  ADtime);
    
    int TimeToOffset(double &offset,int year,int month,int day,int hour,int minute,float second);
    int ISOTimeToOffset(double &offset,const char *szTime);
    int YYYYMMDDTHHMMSSTimeToOffset(double &offset,const char *szTime);
    void PrintISOTime(char *output,int strlen,stADAGUC_time  ADtime);
    void PrintISOTime(char *output,int strlen,double offset);
    void PrintISOTime(char *out,size_t maxlen,int year,int month,int day,int hour,int minute,float second);
    void PrintYYYYMMDDTHHMMSSTime(char *output,int strlen,double offset);
    void GetCurrentDate(char *szDate,size_t max_str_len);
    static char *stpszDateSince;
};

//char *CADAGUC_time::stpszDateSince=NULL;
#endif
