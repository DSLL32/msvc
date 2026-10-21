#ifndef LINK_ARGUMENTS_H
#define LINK_ARGUMENTS_H
#include "linker-types.h"
struct ParsedSwitch;
typedef struct Argument {
    char *text, *original;
    struct ParsedSwitch *parsed;
    struct Argument *next;
} Argument;
typedef struct ArgumentList {
    Argument *first, *last;
    unsigned short count;
} ArgumentList;
extern Argument *first_argument;
extern ArgumentList search_paths, input_files, library_files, other_inputs;
char *duplicate_string(const char *);
void append_argument_node(ArgumentList *,char *,char *);
void append_argument(ArgumentList *,char *);
void collect_search_path(const char *,ArgumentList *);
char *tokenize_response(char *,const char *,unsigned char *);
void parse_response_line(char *);
void collect_arguments(int,char **,const char *);
void classify_argument(char *,unsigned char);
void expand_wildcards(const char *);
unsigned char is_library_file(const char *,int);
#endif
