#include "CADAGUC_time.h"
const char *CADAGUC_time::className="CADAGUC_time";
char *CADAGUC_time::stpszDateSince=NULL;
static utUnit  dataunits;
int  CADAGUC_time_nrInits=0;
void safestrcpy(char *s1,const char*s2,size_t size_s1){
  strncpy(s1,s2,size_s1);
  s1[size_s1]='\0';
  //printf("s1 %s, %d\n",s1,size_s1);
}
CADAGUC_time::CADAGUC_time(const char * DateSince){
  //setenv("UDUNITS_PATH","udunits.dat",1);
  CADAGUC_time_nrInits++;
  //CDBDebug("CADAGUC_time");
  if(stpszDateSince==NULL){
    stpszDateSince=(char*)DateSince;
  }else{
    if(strlen(stpszDateSince)!=strlen(DateSince)){
      printf("Multiple initializations of UDUNITS: [%s] ! = [%s]\n",stpszDateSince,DateSince);
      return;
    }
    if(strncmp(DateSince,stpszDateSince,strlen(stpszDateSince))!=0){
      printf("Multiple initializations of UDUNITS: [%s] ! = [%s]\n",stpszDateSince,DateSince);
      return;
    }
    //printf("UDUNITS already initialized with '%s'\n",stpszDateSince);
    return;
  }
 // strncpy(stpszDateSince,DateSince,ADAGUC_TIME_MAX_STR_LEN);stpszDateSince[ADAGUC_TIME_MAX_STR_LEN]='\0';
  if (utInit("") != 0) {
    CDBError("Couldn't initialize Unidata units library, try setting UDUNITS_PATH to udunits.dat or try setting UDUNITS2_XML_PATH to udunits2.xml");throw(1);return;
  }
  
  size_t l=strlen(stpszDateSince);
  for(size_t j=0;j<l;j++){
    if(stpszDateSince[j]=='U')stpszDateSince[j]=32;
    if(stpszDateSince[j]=='T')stpszDateSince[j]=32;
    if(stpszDateSince[j]=='C')stpszDateSince[j]=32;
    if(stpszDateSince[j]=='Z')stpszDateSince[j]=32;
  }
  if(utScan(stpszDateSince,&dataunits) != 0)  {
    CDBError("internal error: udu_fmt_time can't parse data unit string: \"%s\"",stpszDateSince);
  }
  //char szTemp[ADAGUC_TIME_MAX_STR_LEN+1]; snprintf(szTemp,ADAGUC_TIME_MAX_STR_LEN,"UDUNITS initialized with [%s]",DateSince);CDBDebug(szTemp);
}

CADAGUC_time::~CADAGUC_time(){
  //CDBDebug("~CADAGUC_time");
  CADAGUC_time_nrInits--;
  if(CADAGUC_time_nrInits<=0){
    CADAGUC_time_nrInits=0;
    stpszDateSince=NULL;
    utTerm();
    //CDBDebug("UDUNITS Terminated");
  }
}

int CADAGUC_time::HDF4DateTimeToADAGUC(stADAGUC_time  &ADtime, int date_in,int time_in){
  snprintf(szTime,ADAGUC_TIME_MAX_STR_LEN,"%08dT%06d",date_in,time_in);
  //printf("TIME:[%d][%d]%s\n",date_in,time_in,szTime);
  return YYYYMMDDTHHMMSSToADAGUC(ADtime,szTime);
}

int CADAGUC_time::OffsetToAdaguc(stADAGUC_time  &ADtime, double offset){
  ADtime.offset=offset;
  if(utCalendar(ADtime.offset,&dataunits,&ADtime.year,&ADtime.month,&ADtime.day,&ADtime.hour,&ADtime.minute,&ADtime.second)!=0) {
    CDBError("OffsetToAdaguc: Internal error: utCalendar");return 1 ;
  } 
  return 0;
}

int CADAGUC_time::TimeToOffset(double &offset,int year,int month,int day,int hour,int minute,float second){
  if(utInvCalendar(year,month+1, day+1,hour,minute,(int)second,&dataunits,&offset) != 0){
    CDBError("ITimeToOffset: nternal error: utInvCalendar");return 1;
  }
  return 0;
}
int CADAGUC_time::AdagucToOffset( double &offset,stADAGUC_time  ADtime){
  if(utInvCalendar(ADtime.year,ADtime.month,
     ADtime.day,ADtime.hour,ADtime.minute,
     (int)ADtime.second,&dataunits,&offset) != 0){
       CDBError("AdagucToOffset: Internal error: utInvCalendar");return 1;
     }
     return 0;
}

void CADAGUC_time::PrintISOTime(char* output,int strlen,stADAGUC_time  ADtime){
  snprintf(output,strlen,"%04d-%02d-%02dT%02d:%02d:%09f",ADtime.year,ADtime.month,ADtime.day,ADtime.hour,ADtime.minute,ADtime.second);
  output[strlen]='\0';
}
void CADAGUC_time::PrintYYYYMMDDTHHMMSSTime(char* output,int strlen,double offset){
  stADAGUC_time  ADtime;
  OffsetToAdaguc(ADtime, offset);
  snprintf(output,strlen,"%04d%02d%02dT%02d%02d%02d",ADtime.year,ADtime.month,ADtime.day,ADtime.hour,ADtime.minute,(int)ADtime.second);
  output[strlen]='\0';
}
void CADAGUC_time::PrintISOTime(char* output,int strlen,double offset){
  stADAGUC_time  ADtime;
  OffsetToAdaguc(ADtime, offset);
  PrintISOTime(output,strlen,ADtime);
}

void CADAGUC_time::PrintISOTime(char *out,size_t maxlen,int year,int month,int day,int hour,int minute,float second)
{
  stADAGUC_time  ADtime;
  ADtime.year=year;
  ADtime.month=month;
  ADtime.day=day;
  ADtime.hour=hour;
  ADtime.minute=minute;
  ADtime.second=second;
  PrintISOTime(out,maxlen,ADtime);
}

int CADAGUC_time::ISOTimeToOffset(double &offset,const char *szTime){
  stADAGUC_time  ADtime;
  int status =   ISOTimeToADAGUC(ADtime,szTime);
  if(status!=0)return 1;
  offset=ADtime.offset;
  return 0;
}

int CADAGUC_time::YYYYMMDDTHHMMSSTimeToOffset(double &offset,const char *szTime){
  stADAGUC_time  ADtime;
  int status =   YYYYMMDDTHHMMSSToADAGUC(ADtime,szTime);
  
  if(status!=0)return 1;
  
  offset=ADtime.offset;
  return 0;
}


int CADAGUC_time::ISOTimeToADAGUC(stADAGUC_time  &ADtime,const char *szTime){
  stADAGUC_time  check;
  safestrcpy(szTemp,szTime,4)  ;ADtime.year=atoi(szTemp);
  safestrcpy(szTemp,szTime+5,2);ADtime.month=atoi(szTemp);
  safestrcpy(szTemp,szTime+8,2);ADtime.day=atoi(szTemp);
  safestrcpy(szTemp,szTime+11,2);ADtime.hour=atoi(szTemp);
  safestrcpy(szTemp,szTime+14,2);ADtime.minute=atoi(szTemp);
  safestrcpy(szTemp,szTime+17,2);ADtime.second=(float)atoi(szTemp);
  if(utInvCalendar(ADtime.year,ADtime.month,
     ADtime.day,ADtime.hour,ADtime.minute,
     (int)ADtime.second,&dataunits,&ADtime.offset) != 0){
       CDBError("ISOTimeToADAGUC: Internal error: utInvCalendar");return 1;
  }
  
  if(utCalendar(ADtime.offset,&dataunits,&check.year,&check.month,&check.day,&check.hour,&check.minute,&check.second)!=0) {
    CDBError("ISOTimeToADAGUC: Internal error: utCalendar");return 1;
  }
  snprintf(szTime_test_2,ADAGUC_TIME_MAX_STR_LEN,"%04d-%02d-%02dT%02d:%02d:%02d",check.year,check.month,check.day,check.hour,check.minute,(int)check.second);
  if(strncmp(szTime,szTime_test_2,15)!=0){
    snprintf(szTemp,ADAGUC_TIME_MAX_STR_LEN,"Time Error in ISOTimeToADAGUC: inTime != testTime: \"%s\" != \"%s\"",szTime,szTime_test_2);
    CDBError(szTemp);
    return 1; 
  }
  return 0;
}

int CADAGUC_time::YYYYMMDDTHHMMSSToADAGUC(stADAGUC_time  &ADtime,const char * szTime){
  stADAGUC_time  check;
  safestrcpy(szTemp,szTime,4)  ;ADtime.year=atoi(szTemp);
  safestrcpy(szTemp,szTime+4,2);ADtime.month=atoi(szTemp);
  safestrcpy(szTemp,szTime+6,2);ADtime.day=atoi(szTemp);
  safestrcpy(szTemp,szTime+9,2);ADtime.hour=atoi(szTemp);
  safestrcpy(szTemp,szTime+11,2);ADtime.minute=atoi(szTemp);
  safestrcpy(szTemp,szTime+13,2);ADtime.second=(float)atoi(szTemp);
  /*printf("\n*** %d-%d-%d  T %d:%d:%d\n%s\n",ADtime.year,ADtime.month,
         ADtime.day,ADtime.hour,ADtime.minute,
  (int)ADtime.second,stpszDateSince);*/
//  double offset;
  if(utInvCalendar(ADtime.year,ADtime.month,
     ADtime.day,ADtime.hour,ADtime.minute,
     (int)ADtime.second,&dataunits,&ADtime.offset) != 0){
       snprintf(szTemp,ADAGUC_TIME_MAX_STR_LEN,"YYYYMMDDTHHMMSSToADAGUC: Internal error: utInvCalendar : [%s]",szTime);
       CDBError(szTemp);return 1;
     }
     //printf("************%f\n",ADtime.offset);
     if(utCalendar(ADtime.offset,&dataunits,&check.year,&check.month,&check.day,&check.hour,&check.minute,&check.second)!=0) {
       CDBError("YYYYMMDDTHHMMSSToADAGUC: Internal error: utCalendar");return 1;
     }
     snprintf(szTime_test_2,ADAGUC_TIME_MAX_STR_LEN,"%04d%02d%02dT%02d%02d%02d",check.year,check.month,check.day,check.hour,check.minute,(int)check.second);
     if(strncmp(szTime,szTime_test_2,15)!=0){
       snprintf(szTemp,ADAGUC_TIME_MAX_STR_LEN,"Time Error in YYYYMMDDTHHMMSSToADAGUC: inTime != testTime: \"%s\" != \"%s\"",szTime,szTime_test_2);
       CDBError(szTemp);
       return 1; 
     }
     return 0;
}

void CADAGUC_time::GetCurrentDate(char *szDate,size_t max_str_len){
  time_t rawtime;
  tm * ptm;
  time ( &rawtime );
  ptm = gmtime ( &rawtime );
  PrintISOTime(szDate,max_str_len,
               ptm->tm_year+1900,
               ptm->tm_mon+1,
               ptm->tm_mday,
               ptm->tm_hour,
               ptm->tm_min,
               (float)ptm->tm_sec);
}


