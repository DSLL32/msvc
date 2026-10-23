/* Named native conversion frontiers not yet reconstructed. */
#include "omf.h"
#include <recon.h>
void omf_warning(const char *format,...) { __asm { int 3 } }
void omf_error(const char *format,...) { __asm { int 3 } }
void omf_convert_debug_types(void) { __asm { int 3 } }
void omf_convert_debug_symbols(void) { __asm { int 3 } }
void omf_finish_debug_types(void) { __asm { int 3 } }
void omf_record_lheadr(void) { __asm { int 3 } }
void omf_record_symbol_lines(void) { __asm { int 3 } }
void omf_extend_section(short section,unsigned long offset,const void *data,unsigned long length) { __asm { int 3 } }
void omf_write_debug_type_data(unsigned long offset,const void *data,unsigned long length) { __asm { int 3 } }
void omf_write_debug_symbol_data(unsigned long offset,const void *data,unsigned long length) { __asm { int 3 } }
void omf_insert_relocation_symbol(unsigned long symbol,unsigned long offset) { __asm { int 3 } }
unsigned long omf_lookup_relocation_symbol(unsigned long symbol,unsigned long offset) { __asm { int 3 } }
void omf_save_debug_fixups(void) { __asm { int 3 } }
unsigned short omf_debug_type_auxiliary(unsigned short type,unsigned short *coff_type,IMAGE_AUX_SYMBOL *auxiliary) { __asm { int 3 } }
