/* Logical control-record field schemas, native descriptors at 0xa3380. */
#include "control-ir.h"
static unsigned char ignored18[] = { CONTROL_IGNORED_18, CONTROL_END };
static unsigned char ignored19[] = { CONTROL_IGNORED_19, CONTROL_END };
static unsigned char ignored11[] = { CONTROL_IGNORED_11, CONTROL_END };
static unsigned char ignored98[] = { CONTROL_IGNORED_98, CONTROL_END };
static unsigned char ignored113[] = { CONTROL_IGNORED_113, CONTROL_END };
static unsigned char line_control[] = { CONTROL_LINE, CONTROL_END };
static unsigned char source_control[] = { CONTROL_SOURCE_FILE, CONTROL_END };
static unsigned char optimization_control[] = { CONTROL_OPTIMIZATION, CONTROL_END };
static unsigned char context_control[] = { CONTROL_CONTEXT, CONTROL_END };
static unsigned char plmn_control[] = { CONTROL_PLMN, CONTROL_END };
static unsigned char plmf_control[] = { CONTROL_PLMF, CONTROL_END };
static unsigned char data_control[] = { CONTROL_COUNTED_DATA, CONTROL_END };
static unsigned char function_control[] = { CONTROL_FUNCTION_FLAGS, CONTROL_END };
static unsigned char word_control[] = { CONTROL_WORD, CONTROL_END };
static unsigned char options_control[] = { CONTROL_OPTION_BITS, CONTROL_END };
static unsigned char alignment_control[] = { CONTROL_ALIGNMENT, CONTROL_END };
static unsigned char warnings_control[] = { CONTROL_WARNINGS, CONTROL_END };
unsigned char *control_record_formats[36] = {
    ignored18, line_control, source_control, ignored19,
    ignored19, ignored19, ignored19, 0, 0, 0,
    optimization_control, ignored11, ignored19, ignored19,
    ignored19, 0, 0, 0, 0, context_control,
    plmn_control, plmf_control, data_control, function_control,
    word_control, 0, 0, 0, 0, 0, ignored98, options_control,
    alignment_control, ignored113, 0, warnings_control
};
