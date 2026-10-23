#include "case.h"
int __cdecl transform(int value, const char *text) { return value + (text != 0); }
Callback __cdecl select_callback(int which) { return which ? transform : 0; }
int __stdcall apply_callback(Callback callback, int value) { return callback(value, "item"); }
int (*__cdecl choose_table(int which))[3] { static int values[2][3]; return &values[which != 0]; }
struct Pair __cdecl combine(struct Pair left, const struct Pair *right) { left.right += right->right; return left; }
