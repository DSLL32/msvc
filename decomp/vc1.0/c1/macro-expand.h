#ifndef MACRO_EXPAND_H
#define MACRO_EXPAND_H
#include "macros.h"

/* Macro input frames are saved before nested replacement text is rescanned. */
typedef struct MacroFrame {
    char *source_cursor, *saved_expansion_start, *output_start;
    MacroDefinition *definition;
    signed char argument_count, argument_index;
    unsigned short reserved;
} MacroFrame;
extern MacroFrame macro_frames[256];
extern int macro_expansion_active, macro_rescan_depth;
extern unsigned macro_definition_line;
extern int macro_actual_argument_count;
extern unsigned browser_macro_context;
extern char *macro_input_mark;
/* Include eviction saves the physical source cursor in the first active frame. */
#define macro_saved_input_cursor macro_frames[1].source_cursor

long source_file_timestamp(void);
void __stdcall append_macro_string(int delimiter, char *text);
char *__stdcall quote_macro_filename(char *destination, char *filename);
void __stdcall append_macro_text(char *text);
void __stdcall push_macro_frame(MacroDefinition *definition);
int pop_macro_frame(void);
void begin_macro_replacement(void);
void __stdcall expand_macro_arguments(MacroDefinition *definition, int parameter_count);
void __stdcall browser_macro_reference(MacroDefinition *definition);
char *__stdcall copy_macro_quoted_literal(char *destination, char *limit, int delimiter);
char *read_macro_string(void);
void format_text(char *destination, char *format, ...);
void __stdcall save_macro_argument(char *begin, char *end);
void advance_macro_argument(void);
void browser_macro_end(void);

#endif
