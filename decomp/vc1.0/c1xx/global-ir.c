/* Global intermediate records. The header is rewritten at completion. */
#include "global-ir.h"

unsigned char global_record_kind; /* original 0x9135c */

/* Original 0x56d00..0x56d1f. */
void write_global_header(void)
{
    rewind(global_file);
    global_record_kind = 0;
    write_global_record(17, 0);
}
