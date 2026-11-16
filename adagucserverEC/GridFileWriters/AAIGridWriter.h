#ifndef AAIGRIDWRITER_H
#define AAIGRIDWRITER_H

#include <string>
#include "GridFileData.h"

/**
 * Formats a single band grid as an ESRI ASCII grid (AAIGrid), in the same layout as GDAL's AAIGrid driver with FORCE_CELLSIZE=FALSE.
 * Non-square cells are written with dx and dy. Rows are written north first.
 * Returns 0 on success, nonzero (with a logged error) if the grid cannot be written as AAIGrid.
 */
int formatAAIGrid(const GridFileData &grid, std::string &output);

/**
 * Writes the grid as AAIGrid file, see formatAAIGrid
 */
int writeAAIGrid(const GridFileData &grid, const std::string &fileName);

#endif
