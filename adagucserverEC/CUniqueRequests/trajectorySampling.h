#ifndef ADAGUC_TRAJECTORY_SAMPLING_H
#define ADAGUC_TRAJECTORY_SAMPLING_H

#include "trajectory.h"
#include <string>

class CDataReader;
class CDataSource;
class CImageWarper;

struct TrajectorySample {
  i4point cell;
  f8point lonlat;
};
struct TrajectoryRun {
  int x, y, count;
};
struct TrajectoryGrid {
  std::string projection;
  // Independent grid axes; their lengths may differ.
  std::vector<double> x, y;
  bool operator==(const TrajectoryGrid &) const = default;
};
struct TrajectorySampling {
  std::vector<TrajectorySample> samples;
  std::vector<TrajectoryRun> runs;
  TrajectoryGrid referenceGrid;
};

// Check source configuration and the request-wide open-file limit.
int validateTrajectorySources(const std::vector<CDataSource *> &dataSources);

// Build path-order samples and unique read runs; retain the grid and enforce the value limit.
TrajectorySampling sampleTrajectoryGrid(CDataSource *source, CImageWarper *warper);

// Restore the file context and verify that its grid matches the sampling plan.
void prepareTrajectoryFile(CDataReader &reader, CDataSource *source, int representativeStep, const TrajectorySampling &sampling);
#endif
