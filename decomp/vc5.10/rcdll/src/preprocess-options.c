#include "command.h"
#include "preprocess.h"
#include <recon.h>

/* The embedded preprocessor's option descriptors address mutable settings.
 * Letter suffixes retain their option spelling until their use is recovered. */
WCHAR *rc_cpp_path_characters=L"/",*rc_cpp_include_listing=L"";
WCHAR *rc_cpp_input,*rc_cpp_output,*rc_cpp_debug,*rc_cpp_version;
WCHAR *rc_cpp_A,*rc_cpp_Q;
int rc_cpp_pf,rc_cpp_C,rc_cpp_E,rc_cpp_P,rc_cpp_J,rc_cpp_Oi,rc_cpp_Ol;
int rc_cpp_xc,rc_cpp_H,rc_cpp_Gc,rc_cpp_Fs,rc_cpp_R;
int rc_cpp_extensions=1,rc_cpp_pack=2,rc_cpp_char=1;
int rc_cpp_stack_checks=1;
int rc_cpp_ZE,rc_cpp_Zi,rc_cpp_Zg,rc_cpp_ZI,rc_cpp_Zc;
int rc_cpp_nesting_level=-1;
RcOptionList rc_cpp_defines={100},rc_cpp_undefines={100},rc_cpp_includes={100};

RcLetterOption rc_cpp_Z_options[]={
    {'a',5,&rc_cpp_extensions},
    {'e',1,&rc_cpp_extensions},
    {'E',1,&rc_cpp_ZE},
    {'i',1,&rc_cpp_Zi},
    {'g',1,&rc_cpp_Zg},
    {'p',1,&rc_cpp_pack},
    {'I',1,&rc_cpp_ZI},
    {'c',1,&rc_cpp_Zc},
    {0,0,NULL}
};

RcOption rc_cpp_options[]={
    {L"-pc#",&rc_cpp_path_characters,1,0x22},
    {L"-pf",&rc_cpp_pf,1,1},
    {L"-C",&rc_cpp_C,1,1},
    {L"-CP#",&rc_codepage,1,0x24},
    {L"-D#",&rc_cpp_defines,1,0x26},
    {L"-U#",&rc_cpp_undefines,1,0x26},
    {L"-E",&rc_cpp_E,1,1},
    {L"-I#",&rc_cpp_includes,1,0x26},
    {L"-P",&rc_cpp_P,1,1},
    {L"-f",&rc_cpp_input,1,0x22},
    {L"-g",&rc_cpp_output,1,0x22},
    {L"-J",&rc_cpp_J,1,1},
    {L"-Zp",&rc_cpp_pack,1,1},
    {L"-Zp#",&rc_cpp_pack,1,0x24},
    {L"-Z*",rc_cpp_Z_options,1,0x23},
    {L"-Oi",&rc_cpp_Oi,1,1},
    {L"-Ol",&rc_cpp_Ol,1,1},
    {L"-db#",&rc_cpp_debug,1,0x22},
    {L"-il#",&rc_cpp_include_listing,1,0x22},
    {L"-xc",&rc_cpp_xc,1,1},
    {L"-H",&rc_cpp_H,1,1},
    {L"-V#",&rc_cpp_version,1,0x22},
    {L"-Gs",&rc_cpp_stack_checks,1,5},
    {L"-Gc",&rc_cpp_Gc,1,1},
    {L"-char#",&rc_cpp_char,1,0x24},
    {L"-A#",&rc_cpp_A,1,0x22},
    {L"-Q#",&rc_cpp_Q,1,0x22},
    {L"-Fs",&rc_cpp_Fs,1,1},
    {L"-R",&rc_cpp_R,1,1},
    {L"*",&rc_cpp_bad_argument,0,0x22},
    {NULL,NULL,0,0}
};
