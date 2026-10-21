/* Link driver state referenced by linker_main (00017ea0). */
struct _iobuf;
extern unsigned char _iob[];
void *incremental_file, *image_output;
unsigned long link_phase, output_file_size;
int incremental_requested;
unsigned char vxd_image, small_alignment, incremental_explicit;
unsigned long debug_options;
char link_environment[] = "LINK";
char empty_argument[] = "";
char default_output_name[] = "coff.out";
char *output_name = default_output_name;
struct _iobuf *map_stream = (struct _iobuf *)(_iob + 32);
#include "linker-asm.inc"
