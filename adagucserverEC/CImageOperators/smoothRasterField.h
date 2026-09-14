
#ifndef SMOOTHRASTERFIELD_H

#include "GenericDataWarper/GDWDrawFunctionSettings.h"

template <typename T> T smoothingAtLocation(int gridLocationX, int gridLocationY, T *inputGrid, GDWState &warperState, GDWDrawFunctionSettings &settings);

#endif // !SMOOTHRASTERFIELD_H
