/* Per-module output helpers waiting for matching frontiers. */
#include "pass-two.h"






External * find_mac_contribution_external(Contribution *contribution) { __asm { int 3 } return 0; }

void update_debug_symbol(Image *image,PIMAGE_SYMBOL symbol) { __asm { int 3 }  }







void update_function_auxiliary(PIMAGE_SYMBOL symbol,short section,PIMAGE_AUX_SYMBOL auxiliary,unsigned long *previous,unsigned long *function,FunctionSymbolMap *maps,unsigned long index,Image *image) { __asm { int 3 }  }

void update_block_auxiliary(PIMAGE_SYMBOL symbol,PIMAGE_AUX_SYMBOL auxiliary,unsigned long *previous,unsigned long *function,unsigned long *block,unsigned long remaining) { __asm { int 3 }  }

void record_map_symbol(const char *name,Contribution *contribution,unsigned long value,unsigned char function) { __asm { int 3 } }

void write_mac_loader_prologue(Contribution *contribution) { __asm { int 3 }  }



unsigned read_coff_lines(int file,PIMAGE_LINENUMBER lines,unsigned long count) { __asm { int 3 } return 0; }

unsigned write_coff_lines(int file,PIMAGE_LINENUMBER lines,unsigned long count,ImageOptions *options) { __asm { int 3 } return 0; }

unsigned char lookup_function_symbol(FunctionSymbolMap *maps,unsigned short section,unsigned long input,unsigned long *offset,unsigned long *output) { __asm { int 3 } return 0; }
void record_debug_fixup(unsigned short type,unsigned long rva,unsigned long target) { __asm { int 3 } }
void record_map_relocation(unsigned long rva) { __asm { int 3 } }
unsigned char is_big_endian(void) { __asm { int 3 } return 0; }
