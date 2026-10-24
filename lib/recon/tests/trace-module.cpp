#include "../include/recon-trace.h"
static int __attribute__((noinline)) hidden(int value) { return value+1; }
int __attribute__((noinline)) cpp_leaf(int value) { return hidden(value); }
extern "C" int __attribute__((noinline)) app_alpha(int value) { return cpp_leaf(value); }
extern "C" int __attribute__((noinline)) app_beta(int value) { return value+2; }
/* Instrumented glue deliberately stays outside the registered filter. */
extern "C" int module_glue(int beta) { return beta?app_beta(3):app_alpha(3); }
extern "C" void module_entries(ReconTraceEntry *out)
{
    out[0].address=(const void *)&app_alpha;out[0].name="app_alpha";
    out[1].address=(const void *)&cpp_leaf;out[1].name="_Z8cpp_leafi";
    out[2].address=(const void *)&hidden;out[2].name="_ZL6hiddeni";
    out[3].address=(const void *)&app_beta;out[3].name="app_beta";
}
