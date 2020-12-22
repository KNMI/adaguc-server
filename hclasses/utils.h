#ifndef UTILS_H
#define UTILS_H

static inline int fast_mod(const int input, const int ceil) {
    return input >= ceil ? input % ceil : input;
}

#endif
