#include "inline.h"
extern "C" int (__cdecl *beta_address)(int) = shared_increment;
extern "C" int beta(int value) { return beta_address(value); }
