#ifndef COGCDIMS_H
#define COGCDIMS_H

class COGCDims{
  public:
    COGCDims(){
    }
    COGCDims(const char *name,const char *value){
      this->name.copy(name);
      this->value.copy(value);
    }
    CT::string name;
    CT::string value;
    std::vector<CT::string>allValues;
    CT::string netCDFDimName;
};
class CCDFDims {
  public:
    class NetCDFDim{
      public:
      CT::string name;
      CT::string value;

      size_t index;
    };
    ~CCDFDims (){
      for(size_t j=0;j<dimensions.size();j++){
        delete dimensions[j];
        dimensions[j]=NULL;
      }
    }
    std::vector <NetCDFDim*> dimensions;  
    void addDimension(const char *name,const char *value,size_t index){
      for(size_t j=0;j<dimensions.size();j++){
        if(dimensions[j]->name.equals(name)){
          dimensions[j]->index=index;
          dimensions[j]->value=value;
          return;
        }
      }
      NetCDFDim *dim = new NetCDFDim;
      dim->name.copy(name);
      dim->value.copy(value);
      dim->index=index;
      dimensions.push_back(dim);
    }
    size_t getDimensionIndex(const char *name){
      for(size_t j=0;j<dimensions.size();j++){
        if(dimensions[j]->name.equals(name)){
          return dimensions[j]->index;
        }
      }
      return 0;
    }
    size_t getDimensionIndex(int j){
      if(j<0)return 0;
      if(size_t(j)>dimensions.size())return 0;
      return dimensions[j]->index;
    }
    const char *getDimensionValue(int j){
      if(j<0)return 0;
      if(size_t(j)>dimensions.size())return 0;
      return dimensions[j]->value.c_str();
    }
    const char *getDimensionName(int j){
      if(j<0)return 0;
      if(size_t(j)>dimensions.size())return 0;
      return dimensions[j]->name.c_str();
    }
};
#endif
