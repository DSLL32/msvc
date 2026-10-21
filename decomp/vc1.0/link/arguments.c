/* Command and library search-path lists; append nodes have original ABI. */
typedef struct Argument {
    char *text, *original;
    void *parsed;
    struct Argument *next;
} Argument;
typedef struct ArgumentList {
    Argument *first, *last;
    unsigned short count;
} ArgumentList;
Argument *first_argument;
ArgumentList search_paths, input_files, library_files, other_inputs;
char wildcard_characters[] = "?*";
char object_extension[] = ".obj";
char library_environment[] = "LIB";
char response_open_mode[] = "rt";
char path_separator[] = ";";
char next_path_separator[] = ";";
#include "arguments-asm.inc"
