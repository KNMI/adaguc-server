#ifndef GenericDataWarper_H
#define GenericDataWarper_H

#include <cmath>
#include <cstdlib>
#include <cfloat>
#include <algorithm>
#include <vector>
#include "CImageWarper.h"
#include "Types/GeoParameters.h"
#include "CDebugger.h"
#include "CStopWatch.h"
#include "utils/projectionUtils.h"
#include "GenericDataWarper/GDWState.h"
#include "GenericDataWarper/gdwDrawTriangle.h"

typedef unsigned char uchar;
typedef unsigned char ubyte;
struct GDWArgs {
  CImageWarper *warper;
  void *sourceData;
  GeoParameters sourceGeoParams;
  GeoParameters destGeoParams;
};

class ProjectionGrid {
public:
  double *px = nullptr;
  double *py = nullptr;
  bool *skip = nullptr;
  void initSize(size_t dataSize);
  ~ProjectionGrid();
};

// Reproj back and forth boundingbox in GeoParameters to make valid proj coordinates which always have the same range.
f8box reprojBBox(GeoParameters &input, CImageWarper *warper);

// Make a projection grid. The coordinates are all converted at once from source to destination
ProjectionGrid *makeProjection(double halfCell, CImageWarper *warper, i4box &pixelExtentBox, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams, GDWState &warperState);
ProjectionGrid *makeStridedProjection(double halfCell, CImageWarper *warper, i4box &pixelExtentBox, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams, GDWState &warperState);

class GenericDataWarper {
private:
  ProjectionGrid *projectionGrid = nullptr;

public:
  GenericDataWarper() = default;
  ~GenericDataWarper();
  bool useHalfCellOffset = false;
  // The drawFunction is a template parameter (instead of std::function), so that it can be inlined in the pixel loops.
  // It is called with the signature void(int x, int y, T value, GDWState &warperState).
  template <typename T, typename DrawFn> int render(CImageWarper *warper, void *_sourceData, GeoParameters sourceGeoParams, GeoParameters destGeoParams, const DrawFn &drawFunction);

  template <typename T, typename DrawFn> int render(GDWArgs &args, const DrawFn &drawFunction) {
    return render<T>(args.warper, args.sourceData, args.sourceGeoParams, args.destGeoParams, drawFunction);
  }
};

// Transform the grid linearly. This is used in case projection are the same and is much efficienter then warping the grid.
template <typename T, typename DrawFn>
void linearTransformGrid(GDWState &warperState, bool useHalfCellOffset, CImageWarper *, void *, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams, const DrawFn &drawFunction) {
  StopWatch_Measure("[linearTransformGrid] source %dx%d dest %dx%d", warperState.sourceGridWidth, warperState.sourceGridHeight, warperState.destGridWidth, warperState.destGridHeight);
  double halfCell = useHalfCellOffset ? 0.5 : 0;
  double dfSourceExtW = sourceGeoParams.bbox.span().x;
  double dfSourceExtH = sourceGeoParams.bbox.span().y;
  double dfSourceW = warperState.sourceGridWidth;
  double dfSourceH = warperState.sourceGridHeight;
  double dfDestW = warperState.destGridWidth;
  double dfDestH = warperState.destGridHeight;
  double dfSourceOrigX = sourceGeoParams.bbox.left;
  double dfSourceOrigY = sourceGeoParams.bbox.bottom;
  double dfDestExtW = destGeoParams.bbox.span().x;
  double dfDestExtH = -destGeoParams.bbox.span().y;
  double dfDestOrigX = destGeoParams.bbox.left;
  double dfDestOrigY = destGeoParams.bbox.top;
  int PXExtentBasedOnSource[4] = {0, 0, warperState.sourceGridWidth, warperState.sourceGridHeight};

  if (PXExtentBasedOnSource[2] - PXExtentBasedOnSource[0] <= 0 || PXExtentBasedOnSource[3] - PXExtentBasedOnSource[1] <= 0) {
    StopWatch_Measure("[/linearTransformGrid] empty source grid");
    return;
  }

  // Obtain pixelextent to avoid looping over all source grid cells which will never be used in the destination grid
  i4box pixelspan;
  pixelspan = PXExtentBasedOnSource;
  auto source = sourceGeoParams.bbox;
  auto dest = destGeoParams.bbox;

  f8point span = source.span();
  i4point wh = {.x = sourceGeoParams.width, .y = sourceGeoParams.height};
  f8box newbox = {
      .left = (dest.left - source.left) / span.x, .bottom = (dest.bottom - source.bottom) / span.y, .right = (dest.right - source.left) / span.x, .top = (dest.top - source.bottom) / span.y};
  // The source bbox can be upside down (for example when the latitudes ascend), which inverts newbox. Sort it before rounding, so that floor
  // rounds the lower end down and ceil the upper end up. Rounding first and sorting afterwards shrinks the range by up to a cell on each side.
  newbox.sort();
  i4box newpixelspan = {.left = (int)floor(newbox.left * wh.x), .bottom = (int)floor(newbox.bottom * wh.y), .right = (int)ceil(newbox.right * wh.x), .top = (int)ceil(newbox.top * wh.y)};
  newpixelspan = i4box{
      .left = newpixelspan.left - 1,
      .bottom = newpixelspan.bottom - 1,
      .right = newpixelspan.right + 1,
      .top = newpixelspan.top + 1,

  };
  newpixelspan.clip({.left = 0, .bottom = 0, .right = wh.x, .top = wh.y});

  pixelspan = newpixelspan;

  double xOffset = (dfSourceOrigX - dfDestOrigX) * (dfDestW / dfDestExtW);
  double xScale = (dfSourceExtW / dfSourceW) * (dfDestW / dfDestExtW);

  double yOffset = (dfSourceOrigY - dfDestOrigY) * (dfDestH / dfDestExtH);
  double yScale = (dfSourceExtH / dfSourceH) * (dfDestH / dfDestExtH);

  int sxw = floor(fabs(xScale)) + 1;
  int syh = floor(fabs(yScale)) + 1;
  bool yDirPositive = span.y > 0;
  bool xDirPositive = span.x > 0;
  StopWatch_Measure("linearTransformGrid: start drawing pixelspan %dx%d", pixelspan.span().x, pixelspan.span().y);

  // The destination pixels of a source cell are a range in x that only depends on its column, and a range in y that only depends
  // on its row. These ranges are calculated once per column and row, instead of for every cell: with many more source cells than
  // destination pixels (like a WCS request for every 4th cell) most of the time went into calculating them.
  struct DestRange {
    int start, end;         // The destination pixels start..end-1 of the cell
    int drawStart, drawEnd; // The part of them inside the destination grid, which is drawn
    bool skip;              // Nothing to draw
  };
  auto destRange = [](double round, double nextRound, double scale, int margin, int destSize, int destGridSize) {
    if (std::fabs(scale) < 1) {
      // A source cell smaller than a destination pixel is drawn only in the pixel whose centre it contains: every pixel then
      // gets the source cell at its centre (nearest neighbour), whatever the order in which the cells are stored and drawn.
      // Rounding the edges of the cell instead puts all cells near an edge of a pixel in it, and the last one drawn wins:
      // for an axis stored in descending order that is another cell than the one at the centre of the pixel.
      double low = std::min(round, nextRound), high = std::max(round, nextRound);
      int pixel = (int)std::ceil(low - 0.5);
      DestRange range = {.start = pixel, .end = pixel + 1, .drawStart = std::max(pixel, 0), .drawEnd = std::min(pixel + 1, destGridSize), .skip = !(pixel + 0.5 < high)};
      if (range.drawStart >= range.drawEnd) range.skip = true;
      return range;
    }
    int s1 = std::floor(round + 0.5);
    int s2 = std::floor(round + scale + 0.5);
    bool skip = (s1 < -margin && s2 < -margin) || (s1 >= destSize + margin && s2 >= destSize + margin);
    if (s1 > s2) {
      std::swap(s1, s2);
    }
    if (s2 == s1) s2++;
    DestRange range = {.start = s1, .end = s2, .drawStart = std::max(s1, 0), .drawEnd = std::min(s2, destGridSize), .skip = skip};
    if (range.drawStart >= range.drawEnd) range.skip = true;
    return range;
  };

  // The edges of the cells, calculated once: the upper edge of a cell is the same number as the lower edge of the next
  std::vector<DestRange> columns(std::max(0, pixelspan.right - pixelspan.left));
  std::vector<double> columnEdges(columns.size() + 1);
  for (size_t j = 0; j < columnEdges.size(); j++) columnEdges[j] = (pixelspan.left + (int)j + halfCell) * xScale + xOffset;
  for (int x = pixelspan.left; x < pixelspan.right; x++) {
    DestRange &column = columns[x - pixelspan.left];
    column = destRange(columnEdges[x - pixelspan.left], columnEdges[x - pixelspan.left + 1], xScale, sxw, destGeoParams.width, warperState.destGridWidth);
    if (x < 0 || x >= warperState.sourceGridWidth) column.skip = true;
  }
  std::vector<DestRange> rows(std::max(0, pixelspan.top - pixelspan.bottom));
  std::vector<double> rowEdges(rows.size() + 1);
  for (size_t j = 0; j < rowEdges.size(); j++) rowEdges[j] = (pixelspan.bottom + (int)j - halfCell) * yScale + yOffset; // Y is inverted
  for (int y = pixelspan.bottom; y < pixelspan.top; y++) {
    DestRange &row = rows[y - pixelspan.bottom];
    row = destRange(rowEdges[y - pixelspan.bottom], rowEdges[y - pixelspan.bottom + 1], yScale, syh, destGeoParams.height, warperState.destGridHeight);
    int sourceIndexY = sourceGeoParams.height - 1 - y;
    if (sourceIndexY < 0 || sourceIndexY >= warperState.sourceGridHeight) row.skip = true;
  }
  // The source columns to draw, in the order they are drawn
  std::vector<int> drawnColumns;
  drawnColumns.reserve(columns.size());
  for (int x = pixelspan.left; x < pixelspan.right; x++) {
    if (!columns[x - pixelspan.left].skip) drawnColumns.push_back(x);
  }

  for (int y = pixelspan.bottom; y < pixelspan.top; y++) {
    const DestRange &row = rows[y - pixelspan.bottom];
    if (row.skip) {
      continue;
    }
    double h = double(row.end - row.start);
    warperState.sourceIndexY = sourceGeoParams.height - 1 - y;
    const T *sourceRow = ((T *)warperState.sourceGrid) + (size_t)warperState.sourceIndexY * sourceGeoParams.width;

    for (int x: drawnColumns) {
      const DestRange &column = columns[x - pixelspan.left];
      double w = double(column.end - column.start);
      warperState.sourceIndexX = x;
      T value = sourceRow[x];

      for (int sjy = row.drawStart; sjy < row.drawEnd; sjy++) {
        for (int sjx = column.drawStart; sjx < column.drawEnd; sjx++) {
          warperState.sourceTileDy = yDirPositive ? (sjy - row.start) / h : ((row.end - sjy) / h);
          warperState.sourceTileDx = xDirPositive ? (sjx - column.start) / w : (column.end - sjx) / w;
          drawFunction(sjx, sjy, value, warperState);
        }
      }
    }
  }
  StopWatch_Measure("linearTransformGrid: done drawing");
  StopWatch_Measure("[/linearTransformGrid]");
}

// Warp the grid from the source projection to the destination projection.
template <typename T, typename DrawFn>
void warpTransformGrid(GDWState &warperState, ProjectionGrid *&projectionGrid, bool useHalfCellOffset, CImageWarper *warper, void *, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams,
                       const DrawFn &drawFunction) {

  StopWatch_Measure("[warpTransformGrid] source %dx%d dest %dx%d", warperState.sourceGridWidth, warperState.sourceGridHeight, warperState.destGridWidth, warperState.destGridHeight);
  bool debug = false;
  if (debug) {
    CDBDebug("warpTransformGrid");
  }
  double halfCell = useHalfCellOffset ? 0.5 : 0;

  double dfDestW = warperState.destGridWidth;
  double dfDestH = warperState.destGridHeight;
  double dfDestExtW = destGeoParams.bbox.span().x;
  double dfDestExtH = -destGeoParams.bbox.span().y;
  double multiDestX = dfDestW / dfDestExtW;
  double multiDestY = dfDestH / dfDestExtH;
  double dfDestOrigX = destGeoParams.bbox.left;
  double dfDestOrigY = destGeoParams.bbox.top;
  i4box pixelExtentBox = {0, 0, warperState.sourceGridWidth, warperState.sourceGridHeight};

  if (pixelExtentBox.span().x <= 0 || pixelExtentBox.span().y <= 0) {
    StopWatch_Measure("[/warpTransformGrid] empty source grid");
    return;
  }

  int dataWidth = pixelExtentBox.span().x;
  int dataHeight = pixelExtentBox.span().y;

  if (debug) {
    CDBDebug("warp is required");
  }

  size_t dataSize = (dataWidth + 1) * (dataHeight + 1);

  if (projectionGrid == nullptr) {
    // TODO: Make strided projection work in all cases
    bool useStridingProjection = false;
    if (dataWidth * dataHeight > 1000 * 1000) {
      useStridingProjection = true;
    }

    if (!useStridingProjection) {
      if (debug) {
        CDBDebug("makeProjection");
      }
      projectionGrid = makeProjection(halfCell, warper, pixelExtentBox, sourceGeoParams, destGeoParams, warperState);
    } else {
      if (debug) {
        CDBDebug("makeStridedProjection");
      }
      projectionGrid = makeStridedProjection(halfCell, warper, pixelExtentBox, sourceGeoParams, destGeoParams, warperState);
    }
  }
  auto px = projectionGrid->px;
  auto py = projectionGrid->py;
  auto skip = projectionGrid->skip;
  if (debug) {
    CDBDebug("Reprojection done");
  }

  StopWatch_Measure("warpTransformGrid: start marking invalid projected points");
  for (size_t j = 0; j < dataSize; j++) {
    if (!(px[j] > -DBL_MAX && px[j] < DBL_MAX)) skip[j] = true;
  }
  StopWatch_Measure("warpTransformGrid: done marking invalid projected points");

  bool isMercator = isMercatorProjection(destGeoParams.crs);
  bool isLonLatOrMercatorProjection = isLonLatProjection(destGeoParams.crs) == true || isMercator;
  double sphereWidth = isMercator ? 40000000 : 360;
  int offs1 = 0;
  int offs2 = 1;
  int offs3 = dataWidth + 1 + 1;
  int offs4 = dataWidth + 1 + 0;

  if (debug) {
    CDBDebug("start looping");
  }

  // Check if left is same as right
  size_t numElements = dataHeight * dataWidth;
  warperState.hasSharedBoundaryLR = false;
  size_t offsetY = (dataWidth + 1) * (dataHeight / 2);
  size_t indexLeftXMiddleY = 0 + offsetY;
  size_t indexRightXMiddleY = dataWidth + offsetY;
  size_t indexLeftPlusOneXMiddleY = 1 + offsetY;
  size_t indexLeftPluTwoXMiddleY = 2 + offsetY;
  if (indexLeftXMiddleY < numElements && indexRightXMiddleY < numElements && indexLeftPlusOneXMiddleY < numElements && indexLeftPluTwoXMiddleY < numElements) {

    double fracClosestLeftRight = fabs(px[indexLeftXMiddleY] - px[indexRightXMiddleY]) / fabs(px[indexLeftPlusOneXMiddleY] - px[indexLeftPluTwoXMiddleY]);
    if (debug) {
      CDBDebug("%f %f %f", fabs(px[indexLeftXMiddleY] - px[indexRightXMiddleY]), fabs(px[indexLeftPlusOneXMiddleY] - px[indexLeftPluTwoXMiddleY]), fracClosestLeftRight);
    }
    if (fracClosestLeftRight < 0.01) {
      warperState.hasSharedBoundaryLR = true;
    }
  }

  auto drawQuads = [&]() {
    double avgDX = 0;
    double avgDY = 0;
    double pLengthD = 0;
    for (int y = 0; y < dataHeight; y = y + 1) {
      for (int x = 0; x < dataWidth; x = x + 1) {
        size_t p = x + y * (dataWidth + 1);
        if (skip[p + offs1] == false && skip[p + offs2] == false && skip[p + offs3] == false && skip[p + offs4] == false) {
          bool doDraw = true;
          // Order for the quad corners is:
          //  quadX[0] -- quadX[1]
          //   |      |
          //  quadX[3] -- quadX[2]

          double quadX[4] = {px[p + offs1], px[p + offs2], px[p + offs3], px[p + offs4]};
          double quadY[4] = {py[p + offs1], py[p + offs2], py[p + offs3], py[p + offs4]};

          if (isLonLatOrMercatorProjection) {
            double lonMin = std::min(quadX[0], std::min(quadX[1], std::min(quadX[2], quadX[3])));
            double lonMax = std::max(quadX[0], std::max(quadX[1], std::max(quadX[2], quadX[3])));
            if (lonMax - lonMin >= sphereWidth * 0.9) {
              double lonMiddle = (lonMin + lonMax) / 2.0;
              if (lonMiddle > 0) {
                quadX[0] += sphereWidth;
                quadX[1] += sphereWidth;
                quadX[2] += sphereWidth;
                quadX[3] += sphereWidth;
              } else {
                quadX[0] -= sphereWidth;
                quadX[1] -= sphereWidth;
                quadX[2] -= sphereWidth;
                quadX[3] -= sphereWidth;
              }
            }
          }

          quadX[0] = (quadX[0] - dfDestOrigX) * multiDestX;
          quadX[1] = (quadX[1] - dfDestOrigX) * multiDestX;
          quadX[2] = (quadX[2] - dfDestOrigX) * multiDestX;
          quadX[3] = (quadX[3] - dfDestOrigX) * multiDestX;

          quadY[0] = (quadY[0] - dfDestOrigY) * multiDestY;
          quadY[1] = (quadY[1] - dfDestOrigY) * multiDestY;
          quadY[2] = (quadY[2] - dfDestOrigY) * multiDestY;
          quadY[3] = (quadY[3] - dfDestOrigY) * multiDestY;

          // If suddenly the length of the quad is 10 times bigger, we probably have an anomaly and we should not draw it.
          // Calculate the diagonal length of the quad.
          double lengthD = (quadX[2] - quadX[0]) * (quadX[2] - quadX[0]) + (quadY[2] - quadY[0]) * (quadY[2] - quadY[0]);
          if (x == 0 && y == 0) {
            pLengthD = lengthD;
          }
          if (lengthD > pLengthD * 10) {
            doDraw = false;
          }
          pLengthD = lengthD;

          // Check the right side of the grid
          if (x == 0) avgDX = quadX[1];
          if (y == 0) avgDY = quadY[3];
          if (x == dataWidth - 1) {
            if (fabs(avgDX - quadX[0]) < fabs(quadX[0] - quadX[1]) / 2) {
              doDraw = false;
            }
            if (fabs(avgDX - quadX[1]) < fabs(quadX[0] - quadX[1]) / 2) {
              doDraw = false;
            }
          }
          // Check the bottom side of the grid
          if (y == dataHeight - 1) {
            if (fabs(avgDY - quadY[0]) < fabs(quadY[0] - quadY[3]) / 2) {
              doDraw = false;
            }
          }

          if (doDraw) {
            warperState.sourceIndexX = x + pixelExtentBox.left;
            warperState.sourceIndexY = (warperState.sourceGridHeight - 1 - (y + pixelExtentBox.bottom));
            T value = ((T *)warperState.sourceGrid)[warperState.sourceIndexX + warperState.sourceIndexY * warperState.sourceGridWidth];
            gdwDrawQuad(quadX, quadY, value, warperState, drawFunction);
          }
        }
      }
    }
  };

  StopWatch_Measure("warpTransformGrid: start drawing %dx%d quads", dataWidth, dataHeight);
  drawQuads();
  StopWatch_Measure("warpTransformGrid: done drawing quads");

  StopWatch_Measure("[/warpTransformGrid]");
}

template <typename T, typename DrawFn> int GenericDataWarper::render(CImageWarper *warper, void *_sourceData, GeoParameters sourceGeoParams, GeoParameters destGeoParams, const DrawFn &drawFunction) {

  StopWatch_Measure("[GenericDataWarper::render]");
  bool verbose = false;
  // This structure is passed to drawfunctions and contains info about the current state of the warper.
  // The drawfunction will be called numerous times for each destination pixel.
  GDWState warperState = {.sourceGrid = _sourceData,                  // The source datagrid, has the same datatype as the template T
                          .hasNodataValue = false,                    // Wether the source data grid has a nodata value
                          .dfNodataValue = 0,                         // No data value of the source grid, in double type. Can be casted to T
                          .sourceIndexX = 0,                          // Which X index is sampled from the source grid
                          .sourceIndexY = 0,                          // Which Y index is sampled for the source grid.
                          .sourceGridWidth = sourceGeoParams.width,   // The width of the sourcedata grid
                          .sourceGridHeight = sourceGeoParams.height, // The height of the source data grid
                          .destGridWidth = destGeoParams.width,       // The width of the destination grid
                          .destGridHeight = destGeoParams.height,     // The height of the destination grid
                          .sourceTileDx = 0,                          // The relative X sample position from the source grid cell from 0 to 1. Can be used for bilinear interpolation
                          .sourceTileDy = 0};                         // The relative y sample position

  sourceGeoParams.bbox = reprojBBox(sourceGeoParams, warper);

  /* When geographical map projections are equal, just do a simple linear transformation */
  if (warper->isProjectionRequired() == false) {
    if (verbose) {
      CDBDebug("No reprojection required, doing linear transformation");
    }
    linearTransformGrid<T>(warperState, useHalfCellOffset, warper, _sourceData, sourceGeoParams, destGeoParams, drawFunction);
    if (verbose) {
      CDBDebug("Done");
    }
  } else {
    if (verbose) {
      CDBDebug("Reprojection required, doing warp transformation");
    }
    /* If geographical map projection is different, we have to transform the grid */
    warpTransformGrid<T>(warperState, projectionGrid, useHalfCellOffset, warper, _sourceData, sourceGeoParams, destGeoParams, drawFunction);
  }

  StopWatch_Measure("[/GenericDataWarper::render]");
  return 0;
}
#endif
