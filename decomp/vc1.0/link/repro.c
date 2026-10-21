/* LINK_REPRO response-file capture, used by initialize_log (0001fc60). */
struct _iobuf *repro_stream;
char *repro_directory;
char repro_environment[] = "LINK_REPRO";
char current_directory[] = ".";
char repro_filename[] = "\\link.rsp";
char repro_open_mode[] = "wt";
#include "repro-asm.inc"
