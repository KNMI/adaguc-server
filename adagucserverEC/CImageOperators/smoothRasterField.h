
#ifndef SMOOTHRASTERFIELD_H
#define SMOOTHRASTERFIELD_H

#include "GenericDataWarper/GDWDrawFunctionSettings.h"
#include "GenericDataWarper/GDWState.h"

template <typename T> T smoothingAtLocation(int gridLocationX, int gridLocationY, T *inputGrid, GDWState &warperState, GDWDrawFunctionSettings &settings);

#endif // !SMOOTHRASTERFIELD_H
