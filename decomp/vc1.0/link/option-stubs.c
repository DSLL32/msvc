/* Option helpers not yet entered by an accepted frontier. */
#include "options.h"
unsigned char read_switch_number(ParsedSwitch *option,unsigned short index,unsigned long *value) { __asm { int 3 } return 0; }
long crt_filelength(int file) { __asm { int 3 } return 0; }
unsigned crt_fread(void *buffer,unsigned size,unsigned count,FILE *stream) { __asm { int 3 } return 0; }
