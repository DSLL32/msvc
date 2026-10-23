#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H
#include "frontend.h"
#include <recon.h>
extern unsigned char *debug_record_buffer;
extern unsigned debug_stream_offset;
void write_debug_record(unsigned record, ...);
void begin_line_record_debug_info(void);
#endif
