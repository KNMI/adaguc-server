#include <stdio.h>
#include "CTypes.h"
/*class Fred{
  public:
  Fred& operator= (Fred const& f){
    if (this == &f) return *this;   // Gracefully handle self assignment
    // Put the normal assignment duties here...
    printf("AAAAAAAAAA\n");
    return *this;
}
    CT::string v;
    Fred(){
      printf("BBBB\n");
    }
    
    

    Fred(const char *data){
      v.copy(data);
    }

  Fred& operator= (const char*const &f){
    //if (this == &f) return *this;   // Gracefully handle self assignment
    // Put the normal assignment duties here...
    printf("AAFFFFFFFFFFF %s\n",f);
    return *this;
  }
};

void test(Fred a){
  printf("1:%s\n",a.v.c_str());
}
void test(Fred *a){
  printf("2:%s\n",a->v.c_str());
}*/
int t(){
    
  CT::string a="file_http://opendap.deltares.nl/thredds/dodsC/opendap/deltares/FEWS-IPCC/SRESA1B_BCCR-BCM2_2081-2100.nc";
  
  a.replace("/","_");
  a.concat("_time=227.nccache");
  printf("%s\n",a.c_str());
  return 0;
}
int main(){
  t();
  return 0;
}
