/* Logical schemas used by record_line_information (original 0x3a690). */
#include "control-ir.h"
#include <recon.h>
static unsigned char empty_control[] = { CONTROL_END };
static unsigned char line_control[] = { CONTROL_LINE, CONTROL_END };
static unsigned char source_control[] = { CONTROL_SOURCE_FILE, CONTROL_END };
static unsigned char optimization_control[] = { CONTROL_OPTIMIZATION, CONTROL_END };
static unsigned char context_control[] = { CONTROL_CONTEXT, CONTROL_END };
static unsigned char plmn_control[] = { CONTROL_PLMN, CONTROL_END };
static unsigned char plmf_control[] = { CONTROL_PLMF, CONTROL_END };
static unsigned char name_control[] = { CONTROL_COUNTED_NAME, CONTROL_END };
static unsigned char function_control[] = { CONTROL_FUNCTION_FLAGS, CONTROL_END };
static unsigned char word_control[] = { CONTROL_WORD, CONTROL_END };
static unsigned char options_control[] = { CONTROL_OPTION_BITS, CONTROL_END };
static unsigned char alignment_control[] = { CONTROL_ALIGNMENT, CONTROL_END };
static unsigned char warnings_control[] = { CONTROL_WARNINGS, CONTROL_END };
unsigned char *control_record_formats[36] = {
    empty_control, line_control, source_control, empty_control,
    empty_control, empty_control, empty_control, 0, 0, 0,
    optimization_control, empty_control, empty_control, empty_control,
    empty_control, 0, 0, 0, 0, context_control, plmn_control, plmf_control,
    name_control, function_control, word_control, 0, 0, 0, 0, 0,
    empty_control, options_control, alignment_control, empty_control, 0,
    warnings_control
};
