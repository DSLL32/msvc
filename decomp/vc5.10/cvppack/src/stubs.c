/* Unreconstructed application boundaries retain their original call slots. */
#include "files.h"
#include "types.h"
#include "image.h"
#include "pack.h"
#include "widen.h"
#include "source-index.h"
#include "writer.h"
#include <recon.h>


void open_stream_file(CachedFile *file, int flags, int mode, int created) { __asm { int 3 } }
int release_cached_file(CachedFile *file) { __asm { int 3 } return 0; }
int mark_closed_file(CachedFile *file) { __asm { int 3 } return 0; }


int seek_stream_file(CachedFile *file, int offset, int origin) { __asm { int 3 } return 0; }
int grow_mapping(CachedFile *file, int size) { __asm { int 3 } return 0; }
unsigned stream_read(CachedFile *file, void *buffer, unsigned size) { __asm { int 3 } return 0; }


void decode_legacy_lines(void *lines, unsigned size) { __asm { int 3 } }
void rewrite_legacy_lines(void *lines, CodeViewDirectoryEntry *entry, void *module_buffer, ModuleInfo *info) { __asm { int 3 } }
void read_type_database(const char *path) { __asm { int 3 } }
void rewrite_register16_symbol(void) { __asm { int 3 } }
void rewrite_data16_symbol(void) { __asm { int 3 } }
void rewrite_global_data16_symbol(void) { __asm { int 3 } }
void rewrite_procedure16_symbol(void) { __asm { int 3 } }
void rewrite_mips_procedure_symbol(void) { __asm { int 3 } }

const char *module_name(ModuleInfo *info) { __asm { int 3 } return NULL; }





void convert_legacy_record(unsigned index) { __asm { int 3 } }
void append_type_field_list(unsigned char *field, TypeRecord *record) { __asm { int 3 } }
void release_type_payload(TypeRecord *record) { __asm { int 3 } }




void read_legacy_module_types(void *types, unsigned size) { __asm { int 3 } }



int record_widen(WidenTi *object, void *output, const void *input, unsigned *size) { __asm { int 3 } return 0; }
void register_type_server(void *leaf) { __asm { int 3 } }

unsigned stream_write(CachedFile *file, const void *buffer, unsigned size) { __asm { int 3 } return 0; }
int resize_stream_file(CachedFile *file, int size) { __asm { int 3 } return 0; }
