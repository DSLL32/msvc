/* Named native conversion frontiers not yet reconstructed. */
#include "omf.h"
void omf_warning(const char *format,...) { __asm { int 3 } }
void omf_error(const char *format,...) { __asm { int 3 } }
void omf_write_coff(const char *path) { __asm { int 3 } }
void omf_close_input(void) { __asm { int 3 } }
void omf_finish_symbols(void) { __asm { int 3 } }
void omf_convert_symbol_types(void) { __asm { int 3 } }
void omf_release_line_data(void) { __asm { int 3 } }
void omf_second_pass(void) { __asm { int 3 } }
void omf_convert_debug_types(void) { __asm { int 3 } }
void omf_convert_debug_symbols(void) { __asm { int 3 } }
void omf_finish_debug_types(void) { __asm { int 3 } }
void omf_record_lheadr(void) { __asm { int 3 } }
void omf_record_comment(void) { __asm { int 3 } }
void omf_record_modend(void) { __asm { int 3 } }
void omf_record_externals(unsigned char named,unsigned short storage) { __asm { int 3 } }
void omf_record_publics(unsigned short storage) { __asm { int 3 } }
void omf_record_lines(void) { __asm { int 3 } }
void omf_record_segments(void) { __asm { int 3 } }
void omf_record_groups(void) { __asm { int 3 } }
void omf_save_fixup_record(long position) { __asm { int 3 } }
void omf_record_fixups(void) { __asm { int 3 } }
void omf_record_data(void) { __asm { int 3 } }
void omf_record_iterated_data(void) { __asm { int 3 } }
void omf_record_common(unsigned short storage) { __asm { int 3 } }
void omf_record_backpatch(void) { __asm { int 3 } }
void omf_record_comdat(void) { __asm { int 3 } }
void omf_record_symbol_lines(void) { __asm { int 3 } }
void omf_record_named_backpatch(void) { __asm { int 3 } }
void omf_record_local_publics(unsigned short storage) { __asm { int 3 } }
