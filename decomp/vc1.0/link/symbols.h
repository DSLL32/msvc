#ifndef LINK_SYMBOLS_H
#define LINK_SYMBOLS_H
#include "modules.h"

typedef struct SymbolTable SymbolTable;
typedef struct HashNode { void *value; struct HashNode *next; } HashNode;
typedef struct HashCursor HashCursor;
typedef struct HashTable HashTable;
typedef struct External External;
typedef struct StringNode StringNode;
typedef struct ModuleReference { Module *module; struct ModuleReference *next; } ModuleReference;
#define EXTERNAL_DEFINED 1
#define EXTERNAL_COMMON 2
#define EXTERNAL_EMITTED 4
#define EXTERNAL_COMDAT 0x20
#define EXTERNAL_NO_OUTPUT 0x200
#define EXTERNAL_MULTIPLE_REFERENCES 0x1000
#define EXTERNAL_WEAK 0x40
#define EXTERNAL_LAZY 0x80
#define EXTERNAL_ALIAS 0x4000
#define EXTERNAL_IGNORED 0x100
#define LINK_SYM_CLASS_FAR_EXTERNAL 0x44
struct External {
    IMAGE_SYMBOL symbol;
    union { Contribution *contribution; Module *module; ModuleReference *references; } owner;
    union { unsigned long final_value; External **previous; } value;
    union { External *undefined_next; unsigned long symbol_index; } state;
    unsigned long flags;
    unsigned short archive_member;
    External *weak_default;
};
struct ExternalLink { External *external; ExternalLink *next; };
typedef struct UndefinedEnumerator {
    unsigned char (*next)(struct UndefinedEnumerator *);
    void (*end)(struct UndefinedEnumerator *);
    External *remaining,*current;
} UndefinedEnumerator;
void initialize_undefined_enumerator(UndefinedEnumerator *,SymbolTable *);
unsigned char next_undefined(UndefinedEnumerator *);
void end_undefined_enumerator(UndefinedEnumerator *);
int compare_string_pointers(const void *,const void *);
typedef struct ReferenceEnumerator {
    unsigned char (*next)(struct ReferenceEnumerator *);
    void (*end)(struct ReferenceEnumerator *);
    External *external;
    ModuleReference *remaining;
    Module *current;
} ReferenceEnumerator;
void initialize_reference_enumerator(ReferenceEnumerator *,External *);
unsigned char next_reference(ReferenceEnumerator *);
void end_reference_enumerator(ReferenceEnumerator *);
unsigned char external_referenced_by(External *,Module *);
extern char short_symbol_name[9];
typedef const char *(*HashKey)(const void *,void *);
struct HashCursor {
    unsigned long bucket,returned;
    HashNode *next;
    HashCursor *previous;
};
void set_hash_flags(HashTable *,unsigned short);
unsigned long count_hash_entries(HashTable *);
void begin_hash_enumeration(HashTable *);
HashNode *next_hash_entry(HashTable *);
void end_hash_enumeration(HashTable *);
void begin_external_enumeration(SymbolTable *);
External *next_external(SymbolTable *);
void end_external_enumeration(SymbolTable *);

typedef struct HashSegment { HashNode **buckets; } HashSegment;
struct HashTable {
    unsigned long next_split, base_buckets, bucket_count, element_count;
    unsigned long split_count, directory_capacity, buckets_per_segment;
    unsigned short flags;
    HashKey key;
    HashCursor *enumerators;
    HashSegment **directory;
};
struct SymbolTable {
    HashTable *hash;
    TextBuffer strings;
    StringNode *string_tree;
    External **sorted_by_name, **sorted_by_address, **sorted_by_symbol;
    External *undefined;
    External **undefined_tail;
};

extern TextBuffer sort_string_buffer;
typedef int (*ExternalCompare)(const void *,const void *);
External **sort_external_table(SymbolTable *,ExternalCompare);
int compare_external_names(const void *,const void *);
External **get_externals_by_name(SymbolTable *);
External **get_externals_by_address(SymbolTable *);
char *undecorate_symbol_name(const char *,unsigned char);
void format_undefined_references(TextBuffer *,External *);
unsigned long intern_symbol_name(SymbolTable *,const char *);
unsigned long hash_bucket(const char *,HashTable *);
void expand_hash(HashTable *,void *);
void **find_hash_entry(const char *,HashTable *,unsigned char,void *,unsigned char *);
void set_external_defined(External *,unsigned char,SymbolTable *);
External *lookup_external(SymbolTable *,unsigned short,const char *,unsigned char *,unsigned char *);
const char *symbol_key(const void *,void *);
void initialize_hash(HashTable **,unsigned long,unsigned long,HashKey,unsigned short);
void initialize_symbols(SymbolTable **);
#endif
