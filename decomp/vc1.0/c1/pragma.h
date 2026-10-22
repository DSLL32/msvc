#ifndef PRAGMA_H
#define PRAGMA_H
#include "builtins.h"

typedef union PragmaArgument {
    int integer;
    char *text;
} PragmaArgument;
typedef struct PragmaOptimizationRule {
    unsigned char letter;
    signed char enabled;
    int *target;
} PragmaOptimizationRule;
extern PragmaArgument pragma_argument;
extern short listing_number;
extern char *listing_subtitle, *listing_title;
extern char *optimization_pragma_text;
extern int optimization_pragma_on, native_caller_pragma_seen;
extern int default_string_pooling;
extern FrontendSymbol *default_code_segment, *default_data_segment;
extern FrontendSymbol *default_bss_segment, *default_const_segment;
extern unsigned pragma_optimization_restore_mask, pragma_optimization_disable_mask;
extern PragmaOptimizationRule pragma_optimization_rules[];
unsigned char read_pragma_token(void);
int __stdcall classify_pragma(char *name);
int __stdcall read_pragma_argument(unsigned char kind);
int __stdcall read_parenthesized_pragma_argument(unsigned char kind);
int pragma_open_parenthesis(void);
void __stdcall visit_pragma_names(void (__stdcall *callback)(char *));
void __stdcall enable_intrinsic_function(char *name);
void __stdcall disable_intrinsic_function(char *name);
void __stdcall assign_allocated_text(char *name);
void warn_expected_identifier(void);
void process_pack_pragma(void);
int __stdcall validate_packing(int alignment);
void __stdcall push_pragma_packing(int alignment, char *label);
void __stdcall pop_pragma_packing(int alignment, char *label);
void __stdcall emit_listing_directive(unsigned char mode);
void __stdcall pragma_message(char *text);
void unpack_option_flags(unsigned flags);
void __stdcall set_warning_level(int number, int level);
void warn_bad_pragma(void);
void synchronize_pch_pragma(void);
#endif
