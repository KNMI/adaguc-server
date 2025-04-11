#include "CDebugger.h"
#include "GenericDataWarper/gdwDrawTriangle.h"
#include "CImgWarpHillShaded.h"
#include <fstream>

int imageWidth = 1000;
int imageHeight = 800;

struct WarperSettings {
  double dx;
};
struct TestSettings : WarperSettings {

  double dfNodataValue;
  double legendValueRange;
  double legendLowerRange;
  double legendUpperRange;
  bool hasNodataValue;
  float *dataField;
  int width, height;
  CDrawImage *image;
};

void drawFunction(int x, int y, double val, void *_settings, void *_warper) {
  auto settings = (TestSettings *)_settings;
  auto warper = (CGenericDataWarper *)_warper;
  double dx = warper->warperState.tileDx;

  int colorG = dx * 255;
  CColor c = CColor(colorG, colorG, 255 - colorG, 200);
  if (x < 0 || y < 0 || x >= imageWidth || y >= imageHeight) {
    CDBDebug("%d %d", x, y);
  }
  settings->image->setPixel(x, y, c);
}

#define PI 3.141592654

int main() {
  CDBDebug("HELLO");

  // gdwDrawTriangle<T>(xCornersA, yCornersA, value, imageWidth, imageHeight, drawFunctionSettings, drawFunction, (void *)this, false);

  CDrawImage image;
  image.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
  image.setRenderer(CDRAWIMAGERENDERER_CAIRO);
  image.enableTransparency(false);
  image.setBGColor(0, 0, 0);

  image.createImage(imageWidth, imageHeight);
  image.create685Palette();

  image.rectangle(100, 100, 900, 1000, 20);

  TestSettings drawFunctionSettings;
  CGenericDataWarper warper;

  drawFunctionSettings.image = &image;

  CColor c(255, 255, 255, 255);
  CColor b(0, 0, 255, 255);
  CColor y(255, 255, 0, 255);

  double numtriangles = 10000;

  for (double r = 0; r < numtriangles; r += 1) {

    double angle = (r / numtriangles) * PI * 2;

    double px1 = cos(angle) * 100 + 500;
    double px3 = cos(angle + 0.1) * 300 + 500;
    double px4 = cos(angle - 0.1) * 250 + 500;

    double py1 = sin(angle) * 100 + 500;
    double py3 = sin(angle + 0.1) * 300 + 500;
    double py4 = sin(angle - 0.1) * 250 + 500;

    double xCornersB[3] = {px3, px1, px4};
    double yCornersB[3] = {py3, py1, py4};

    gdwDrawTriangle<double>(xCornersB, yCornersB, 5, imageWidth, imageHeight, &drawFunctionSettings, drawFunction, &warper, true);

    // image.moveTo(px1, py1);
    // image.lineTo(px3, py3, 1, c);
    // image.lineTo(px4, py4, 1, c);
    // image.lineTo(px1, py1, 1, c);
    // image.endLine();

    // image.setPixel(px1, py1, c);
    // image.setPixel(px4, py4, b);
    // image.setPixel(px3, py3, y);
  }

  for (double r = 0; r < numtriangles; r += 1) {

    double angle = (r / numtriangles) * PI * 2;

    double px1 = cos(angle) * 400 + 500;
    double px3 = cos(angle + 0.1) * 650 + 500;
    double px4 = cos(angle - 0.1) * 425 + 500;

    double py1 = sin(angle) * 404 + 500;
    double py3 = sin(angle + 0.1) * 610 + 500;
    double py4 = sin(angle - 0.1) * 350 + 500;

    double xCornersB[3] = {px3, px1, px4};
    double yCornersB[3] = {py3, py1, py4};

    gdwDrawTriangle<double>(xCornersB, yCornersB, 5, imageWidth, imageHeight, &drawFunctionSettings, drawFunction, &warper, true);

    // image.moveTo(px1, py1);
    // image.lineTo(px3, py3, 1, c);
    // image.lineTo(px4, py4, 1, c);
    // image.lineTo(px1, py1, 1, c);
    // image.endLine();

    // image.setPixel(px1, py1, c);
    // image.setPixel(px4, py4, b);
    // image.setPixel(px3, py3, y);
  }

  // double xCornersB[3] = {500, 10, 920};
  // double yCornersB[3] = {20, 500, 701};
  // gdwDrawTriangle<double>(xCornersB, yCornersB, 5, imageWidth, imageHeight, &drawFunctionSettings, drawFunction, &warper, true);

  // double xCornersC[3] = {500, 10, 80};
  // double yCornersC[3] = {20, 500, 121};
  // gdwDrawTriangle<double>(xCornersC, yCornersC, 5, imageWidth, imageHeight, &drawFunctionSettings, drawFunction, &warper, true);

  // double xCornersD[3] = {500, 920, 980};
  // double yCornersD[3] = {20, 701, 221};
  // gdwDrawTriangle<double>(xCornersD, yCornersD, 5, imageWidth, imageHeight, &drawFunctionSettings, drawFunction, &warper, true);

  double xCornersE[3] = {0, 0, 1000};
  double yCornersE[3] = {500, 500, 250};
  gdwDrawTriangle<double>(xCornersE, yCornersE, 5, imageWidth, imageHeight, &drawFunctionSettings, drawFunction, &warper, true);

  FILE *myfile;
  myfile = fopen("test.png", "wb");
  image.cairo->writeToPng32Stream(myfile, 255);
  fclose(myfile);
}