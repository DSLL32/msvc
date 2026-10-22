/* Named frontiers of the dump-mode graph. */
#include "dump.h"

void initialize_disassembler(FatalRoutine fatal,WarningRoutine warning) { __asm { int 3 } }

void dump_symbol_map(void) { __asm { int 3 } }


void disassemble_section(IMAGE_SECTION_HEADER *section,unsigned short index,PIMAGE_SYMBOL symbols,unsigned long count,unsigned char image,DumpStringTable *strings,int file,FILE *stream) { __asm { int 3 } }

void dump_fpo(IMAGE_SECTION_HEADER *section) { __asm { int 3 } }

void dump_debug_directory(IMAGE_SECTION_HEADER *section) { __asm { int 3 } }

void dump_imports(IMAGE_SECTION_HEADER *section) { __asm { int 3 } }

void dump_exports(IMAGE_SECTION_HEADER *section) { __asm { int 3 } }


void dump_base_relocation_block(IMAGE_BASE_RELOCATION *block,unsigned short *fixups) { __asm { int 3 } }
