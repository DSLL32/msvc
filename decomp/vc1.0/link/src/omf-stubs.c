/* Named native conversion frontiers not yet reconstructed. */
#include "omf.h"
#include <recon.h>
void omf_warning(const char *format,...) { __asm { int 3 } }
void omf_error(const char *format,...) { __asm { int 3 } }
void omf_convert_debug_types(void) { __asm { int 3 } }
void omf_convert_debug_symbols(void) { __asm { int 3 } }
void omf_finish_debug_types(void) { __asm { int 3 } }
void omf_record_lheadr(void) { __asm { int 3 } }
void omf_record_lines(void) { __asm { int 3 } }
void omf_record_iterated_data(void) { __asm { int 3 } }
void omf_record_common(unsigned short storage) { __asm { int 3 } }
void omf_record_backpatch(void) { __asm { int 3 } }
void omf_record_symbol_lines(void) { __asm { int 3 } }
void omf_record_named_backpatch(void) { __asm { int 3 } }
void omf_record_local_publics(unsigned short storage) { __asm { int 3 } }
void omf_extend_section(short section,unsigned long offset,const void *data,unsigned long length) { __asm { int 3 } }
void omf_write_debug_type_data(unsigned long offset,const void *data,unsigned long length) { __asm { int 3 } }
void omf_write_debug_symbol_data(unsigned long offset,const void *data,unsigned long length) { __asm { int 3 } }
void omf_insert_relocation_symbol(unsigned long symbol,unsigned long offset) { __asm { int 3 } }
unsigned long omf_lookup_relocation_symbol(unsigned long symbol,unsigned long offset) { __asm { int 3 } }
void omf_save_debug_fixups(void) { __asm { int 3 } }
void omf_write_comment_data(short section,unsigned long offset,char *const *comments,short count) { __asm { int 3 } }
unsigned short omf_debug_type_auxiliary(unsigned short type,unsigned short *coff_type,IMAGE_AUX_SYMBOL *auxiliary) { __asm { int 3 } }
void omf_append_function_auxiliary(unsigned long count,unsigned short section,unsigned long start,unsigned short first_line,unsigned long length,unsigned short last_line) { __asm { int 3 } }
void omf_append_line(short section,unsigned long offset,unsigned short line) { __asm { int 3 } }
