#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H
#include "initialization.h"

#pragma pack(1)
typedef struct PreprocessorSymbol {
    struct PreprocessorSymbol *next;
    unsigned char flags;
    char name[1];
} PreprocessorSymbol;
typedef struct PreprocessorKeyword {
    unsigned char token;
    PreprocessorSymbol symbol;
} PreprocessorKeyword;
#pragma pack()
#define SYMBOL_FROM_NAME(name) ((PreprocessorSymbol *)((char *)(name) - 5))
#define KEYWORD_FROM_NAME(name) ((PreprocessorKeyword *)((char *)(name) - 6))
extern PreprocessorSymbol **preprocessor_buckets;
extern unsigned char *macro_text_begin, *macro_text_next;
void initialize_macro_text(void);
void scan_continued_identifier(char *start, unsigned hash);
#endif
