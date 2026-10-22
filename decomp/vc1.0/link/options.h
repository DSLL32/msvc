#ifndef LINK_OPTIONS_H
#define LINK_OPTIONS_H
#include "image.h"
#include "arguments.h"
typedef struct SwitchValue { char *key,*value; } SwitchValue;
typedef struct ParsedSwitch {
    char *name;
    unsigned short count;
    SwitchValue values[1];
} ParsedSwitch;
typedef struct IncludedExternal {
    External *external;
    struct IncludedExternal *next;
} IncludedExternal;
ParsedSwitch *parse_switch(char *);
extern ArgumentList switches, section_merges, section_overrides, exports;
extern TextBuffer image_comments;
extern IncludedExternal *included_externals;
extern char *option_delimiters,*map_filename,*def_filename,*absolute_symbols_file;
extern char *import_library_name,*cvpack_program,*order_filename;
extern unsigned char base_pair,no_entry,verbose_link,mapped_debug,split_image,nt_beta1;
extern unsigned char old_library_format;
extern unsigned short warning_level;
void add_default_library(const char *,LibraryList *);
unsigned char matches_default_library(const char *,DefaultLibrary *);
void exclude_default_library(const char *,LibraryList *);
void exclude_all_default_libraries(const char *,LibraryList *);
char *find_library_file(const char *,const char *,ArgumentList *);
unsigned char read_switch_number(ParsedSwitch *,unsigned short,unsigned long *);
typedef void (*FatalRoutine)(const char *,int,...);
typedef void (*WarningRoutine)(const char *,unsigned,...);
void initialize_order(FatalRoutine,WarningRoutine);
long crt_filelength(int);
#endif
