#ifndef COGCDIMS_H
#define COGCDIMS_H
#include <stdio.h>
#include "CTypes.h"

class COGCDims{
  public:
    COGCDims(){
    }
    /**
     * OGC name
     */
    
    CT::string name;
    /** 
     * Value, are all values as given in the KVP request string, can contain / and , tokens
     */
    CT::string value;

    /**
     * NetCDF name
     */
    CT::string netCDFDimName;
    
    /**
     * Unique values, similar to values except that they are filtered and selected as available from the file (distinct/grouped), 
     */
    std::vector<CT::string> uniqueValues;
    
    void addValue(const char *value);
    
};
class CCDFDims {
  private:
  DEF_ERRORFUNCTION();
  public:
    class NetCDFDim{
      public:
      CT::string name;
      CT::string value;
      size_t index;
    };
    std::vector <NetCDFDim*> dimensions;  
    ~CCDFDims ();
    void addDimension(const char *name,const char *value,size_t index);
    size_t getDimensionIndex(const char *name);
    size_t getDimensionIndex(int j);
    const char *getDimensionValue(int j);
    const char *getDimensionName(int j);
};
#endif
