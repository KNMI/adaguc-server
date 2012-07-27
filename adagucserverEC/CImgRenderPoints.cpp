#include "CImgRenderPoints.h"
const char *CImgRenderPoints::className="CImgRenderPoints";

void CImgRenderPoints::render(CImageWarper*warper, CDataSource*dataSource, CDrawImage*drawImage){
  if(dataSource->dataObject.size()==2){
    CT::string varName1=dataSource->dataObject[0]->cdfVariable->name.c_str();
    CT::string varName2=dataSource->dataObject[1]->cdfVariable->name.c_str();
    CDBDebug("varName1 = %s",varName1.c_str());
    CDBDebug("varName2 = %s",varName2.c_str());
    std::vector<PointDV> *p1=&dataSource->dataObject[0]->points;
    std::vector<PointDV> *p2=&dataSource->dataObject[1]->points;
    size_t l=p1->size();
    size_t s=1;
    while(l/s>(80*32)){
      s=s+s;
    };
    l=p1->size();
    for(size_t j=0;j<l;j=j+s){
      
      
      float strength = (*p1)[j].v;
      float direction = (*p2)[j].v;
     // direction=360-45;
      drawImage->drawVector((*p1)[j].x, dataSource->dHeight-(*p1)[j].y, ((90-direction)/360)*3.141592654*2, strength*2, 240);
    }
  }
}
int CImgRenderPoints::set(const char*){
}