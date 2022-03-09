#include "CCreateLegend.h"
#include "CDataReader.h"
#include "CImageDataWriter.h"

int CCreateLegend::renderContinuousLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool, bool estimateMinMax) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("legendtype continous");
#endif
    bool drawUpperTriangle = true;
    bool drawLowerTriangle = true;

   float fontSize=dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.size.toDouble();
    const char *fontLocation = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();

    double scaling = dataSource->getScaling();
    float legendHeight = legendImage->Geo->dHeight - 10*fontSize*scaling;
    float cbH = legendHeight- 13 - 13 * scaling; // 5*legendHeight / scaling;


    if (styleConfiguration->hasLegendValueRange) {
      float minValue = CImageDataWriter::getValueForColorIndex(dataSource, 0);
      float maxValue = CImageDataWriter::getValueForColorIndex(dataSource, 239);
      if (styleConfiguration->legendLowerRange >= minValue) {
        drawLowerTriangle = false;
      }
      if (styleConfiguration->legendUpperRange <= maxValue) {
        drawUpperTriangle = false;
      }
    }

    float triangleShape = 1.1; // 1.1;

    float cbW = 16; // legendWidth/8;
    float triangleHeight = int(((int)cbW ) / triangleShape);
    int dH = 0;

    if (drawUpperTriangle) {
      dH += int(triangleHeight);
      cbH -= triangleHeight;
    }
    if (drawLowerTriangle) {
      cbH -= triangleHeight;
    }

    int minColor;
    int maxColor;
    int legendPositiveUp = 1;
    int pLeft = 4;
    int pTop = (int)(legendImage->Geo->dHeight - legendHeight);
    for (int j = 0; j < cbH; j++) {
      // for(int i=0;i<cbW+3;i++){
      float c = (float(cbH * legendPositiveUp - (j + 1)) / cbH) * 240.0f;
      for (int x = pLeft; x < pLeft + ((int)cbW + 1)*scaling; x++) {
        legendImage->setPixelIndexed(x, j + 7 + dH + pTop, int(c));
        // legendImage->setPixelIndexed(x, j, int(c));

      }
      

      // legendImage->setPixelIndexed(i+pLeft,j+7+dH+pTop,int(c));
      // legendImage->line_noaa(pLeft,j+7+dH+pTop,pLeft+(int)cbW+1,j+7+dH+pTop,int(c));
      if (j == 0) minColor = int(c);
      maxColor = int(c);
      // }
    }
    // legendImage->rectangle(pLeft,7+dH+pTop,(int)cbW+3+pLeft,(int)cbH+7+dH+pTop,248);

    // TODO: Remove scaling, investigate how scaling factor applies
    legendImage->line(pLeft, 6 + dH + pTop, pLeft, (int)cbH + 7 + dH + pTop, 0.8, 248);
    legendImage->line((int)cbW + 1 + pLeft, 6 + dH + pTop, (int)cbW + 1 + pLeft, (int)cbH + 7 + dH + pTop, 0.8, 248);

    int triangleLX = pLeft;
    int triangleRX = pLeft + ((int)cbW + 1)*scaling; // (int)cbW + 1 + pLeft; 
    int triangleMX = (triangleRX + triangleLX)/2; // (int)cbW + pLeft - int(cbW / 2.0);

    int triangle1BY = 7 + dH + pTop - 1;
    int triangle1TY = int(7 + dH + pTop - triangleHeight);

    int triangle2TY = (int)cbH + 7 + dH + pTop;
    int triangle2BY = int(cbH + 7 + dH + pTop + triangleHeight);

    if (drawUpperTriangle) {
      // Draw upper triangle
      for (int j = 0; j < (triangle1BY - triangle1TY) + 1; j++) {
        int dx = int((float(j) / float(triangle1BY - triangle1TY)) * cbW / 2.0);
        // int dx=int((float(j)/float((triangle2BY-triangle2TY)))*cbW/2.0);
        //         legendImage->line(triangleLX+dx,triangle1BY-j,triangleRX-dx-0.5,triangle1BY-j,minColor);
        for (int x = triangleLX + dx; x < triangleRX - dx - 0.5; x++) {
          legendImage->setPixelIndexed(x, triangle1BY - j, int(minColor));
        }
        
      }
      legendImage->line(triangleLX, triangle1BY, triangleMX, triangle1TY - 1, 0.8, 248);
      legendImage->line(triangleRX, triangle1BY, triangleMX, triangle1TY - 1, 0.8, 248);
    } else {
      legendImage->line(triangleLX, triangle1BY + 1, triangleRX, triangle1BY + 1, 0.8, 248);
    }

    if (drawLowerTriangle) {
      // Draw lower triangle
      for (int j = 0; j < (triangle2BY - triangle2TY) + 1; j++) {
        int dx = int((float(j) / float((triangle2BY - triangle2TY))) * cbW / 2.0);
        // legendImage->line(triangleLX+dx+0.5,triangle2TY+j,triangleRX-dx-0.5,triangle2TY+j,maxColor);
        for (int x = triangleLX + dx + 0.5; x < triangleRX - dx - 0.5; x++) {
          legendImage->setPixelIndexed(x, triangle2TY + j, int(maxColor));
        }
      }

      legendImage->line(triangleLX, triangle2TY, triangleMX, triangle2BY + 1, 0.6, 248);
      legendImage->line(triangleRX, triangle2TY, triangleMX, triangle2BY + 1, 0.6, 248);
    } else {
      legendImage->line(triangleLX, triangle2TY, triangleRX, triangle2TY, 0.8, 248);
    }

    double classes = 6;
    int tickRound = 0;
    double min = CImageDataWriter::getValueForColorIndex(dataSource, 0);
    double max = CImageDataWriter::getValueForColorIndex(dataSource, 240); // TODO CHECK 239
    if (max == INFINITY) max = 239;
    if (min == INFINITY) min = 0;
    if (max == min) max = max + 0.000001;
    double increment = (max - min) / classes;
    if (styleConfiguration->legendTickInterval > 0) {
      // classes=(max-min)/styleConfiguration->legendTickInterval;
      // classes=int((max-min)/double(styleConfiguration->legendTickInterval)+0.5);
      increment = double(styleConfiguration->legendTickInterval);
    }
    if (increment <= 0) increment = 1;

    if (styleConfiguration->legendTickRound > 0) {
      tickRound = int(round(log10(styleConfiguration->legendTickRound)) + 3);
    }

    if (increment > max - min) {
      increment = max - min;
    }
    if ((max - min) / increment > 250) increment = (max - min) / 250;
    // CDBDebug("%f %f %f",min,max,increment);
    if (increment <= 0) {
      CDBDebug("Increment is 0, setting to 1");
      increment = 1;
    }
    classes = abs(int((max - min) / increment));

    char szTemp[256];
    if (styleConfiguration->legendLog != 0) {
      for (int j = 0; j <= classes; j++) {
        double c = ((double(classes * legendPositiveUp - j) / classes)) * (cbH);
        double v = ((double(j) / classes)) * (240.0f);
        v -= styleConfiguration->legendOffset;
        if (styleConfiguration->legendScale != 0) v /= styleConfiguration->legendScale;
        if (styleConfiguration->legendLog != 0) {
          v = pow(styleConfiguration->legendLog, v);
        }
        {
          float lineWidth = 0.8;
          legendImage->line((int)cbW - 1 + pLeft, (int)c + 6 + dH + pTop, (int)cbW + 6 + pLeft, (int)c + 6 + dH + pTop, lineWidth, 248);
          if (tickRound == 0) {
            floatToString(szTemp, 255, min, max, v);
          } else {
            floatToString(szTemp, 255, tickRound, v);
          }
          // legendImage->setText(szTemp, strlen(szTemp), (int)cbW + 10 + pLeft, (int)c + dH + pTop + 1, 248, -1);
          legendImage->drawText(((int)cbW + 12 + pLeft) * scaling, (pTop) - ((fontSize * scaling) / 4) + 3, fontLocation, fontSize * scaling, 0, szTemp, 248);
  
        }
      }
    } else {

      // CDBDebug("LEGEND: scale %f offset %f",styleConfiguration->legendScale,styleConfiguration->legendOffset);
      for (double j = min; j < max + increment; j = j + increment) {
        // CDBDebug("%f",j);
        double lineY = cbH - ((j - min) / (max - min)) * cbH;
        // CDBDebug("%f %f %f",j,lineY,cbH);
        // double c=((double(classes*legendPositiveUp-j)/classes))*(cbH);
        double v = j; // pow(j,10);
        // v-=styleConfiguration->legendOffset;

        //       if(styleConfiguration->legendScale != 0)v/=styleConfiguration->legendScale;
        //       if(styleConfiguration->legendLog!=0){v=pow(styleConfiguration->legendLog,v);}

        if (lineY >= -2 && lineY < cbH + 2) {
          float lineWidth = 0.8;
          legendImage->line((int)cbW - 1 + pLeft, (int)lineY + 6 + dH + pTop, (int)cbW + 6 + pLeft, (int)lineY + 6 + dH + pTop, lineWidth, 248);          if (tickRound == 0) {
            floatToString(szTemp, 255, min, max, v);
          } else {
            floatToString(szTemp, 255, tickRound, v);
          }
          // legendImage->setText(szTemp, strlen(szTemp), (int)cbW + 10 + pLeft, (int)lineY + dH + pTop + 1, 248, 0);
          legendImage->drawText(((int)cbW + 12 + pLeft) * scaling, ((int)lineY + dH + pTop) + ((fontSize * scaling) / 4) + 6, fontLocation, fontSize * scaling, 0, szTemp, 248);
        }
      }
    }
    // Get units
    CT::string units;
    if (dataSource->getDataObject(0)->getUnits().length() > 0) {
      units.concat(dataSource->getDataObject(0)->getUnits().c_str());
    }
    if (units.length() == 0) {
      units = "-";
    }

    legendImage->setText(units.c_str(), units.length(), 2 + pLeft, int(legendHeight) - 14 + pTop, 248, 0);
    // legendImage->crop(2,2);
    // return 0;


#ifdef CIMAGEDATAWRITER_DEBUG

  CDBDebug("set units");
#endif
   return 0;
}