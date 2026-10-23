/* Frontend debug intermediate initialization and CodeView record output. */
#include "debug-info.h"
#include <recon.h>

unsigned char *debug_record_buffer; /* original 0x8ccb4 */
unsigned debug_stream_offset; /* original 0x9f45c */

/* Original 0x6d660..0x6d6b8. The ordinary CodeView stream begins
   with its record formats and a four-byte version signature. Line-record
   mode retains its separate original initialization entry. */
void begin_debug_info(void)
{
    if (options.configuration.line_records) {
        begin_line_record_debug_info();
        return;
    }
    debug_record_buffer = checked_malloc(4096);
    write_debug_record(12);
    write_debug_record(10);
    *(unsigned *)debug_record_buffer = 1;
    write_debug_record(3, 4, debug_record_buffer);
    debug_stream_offset += 4;
}
