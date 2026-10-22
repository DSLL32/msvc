/* Image checksum entry point shared by linking and editing. */
#include "pass-two.h"
#include <recon.h>
PIMAGE_NT_HEADERS checksum_mapped_image(void *,unsigned long,unsigned long *,unsigned long *);

/* Original [00025c60,00025cd0). This operation requires the mapped file backend. */

void update_checksum(int file)
{
    unsigned long size=file_size(file),previous;
    void *view=mapped_output_pointer(file,0,size);
    if (view) checksum_mapped_image(view,size,&previous,NULL);
    else warning_message(program_name,0x81,"","");
}
