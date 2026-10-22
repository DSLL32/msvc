#include "inline.h"
extern "C" int (__cdecl *alpha_address)(int) = shared_increment;
extern "C" int alpha(int value) { return alpha_address(value); }
