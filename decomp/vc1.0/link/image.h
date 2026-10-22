#ifndef LINK_IMAGE_H
#define LINK_IMAGE_H
#include "modules.h"
#include "symbols.h"
/* Options consumed by the reconstructed driver and output passes. */
typedef struct ImageOptions {
    unsigned char portable_image, force, fixed, rom;
    unsigned char output_specified, base_specified, heap_specified, stack_specified;
    unsigned char order_specified, no_pack, no_default_libraries, eliminate_unreferenced;
    unsigned char checksum;
    unsigned long global_pointer_size, map_format, debug_level, debug_types;
    char *default_extension;
    unsigned long optimize;
    unsigned char library_list,library_definition;
    char *library_image_name;
} ImageOptions;
typedef struct Image {
    IMAGE_FILE_HEADER file;
    IMAGE_OPTIONAL_HEADER optional;
    ImageOptions options;
    SectionList sections;
    LibraryList libraries;
    SymbolTable *symbols;
} Image;
void create_image(Image **);
void process_options(Image *,unsigned char,unsigned char,const char *);
int link_passes(Image *,unsigned char *);
void initialize_machine(Image *);
void select_machine(struct Argument *,unsigned short,Image *);
void validate_machine(const char *,unsigned short,Image *);
void select_entry_point(Image *,unsigned char);
void set_subsystem_version(IMAGE_OPTIONAL_HEADER *);
int run_cvpack(ImageOptions *);
void close_all_files(void);
void remove_temporary_inputs(void);
void close_trace_file(void);
void write_map(int,Image *,const char *);
#endif
