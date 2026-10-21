#ifndef LINK_SYMBOLS_H
#define LINK_SYMBOLS_H
#include "linker-types.h"

typedef struct HashNode HashNode;
typedef struct HashCursor HashCursor;
typedef struct External External;
typedef struct StringNode StringNode;
typedef const char *(*HashKey)(const void *,void *);
typedef struct HashSegment { HashNode **buckets; } HashSegment;
typedef struct HashTable {
    unsigned long next_split, base_buckets, bucket_count, element_count;
    unsigned long split_count, directory_capacity, buckets_per_segment;
    unsigned long flags;
    HashKey key;
    HashCursor *enumerators;
    HashSegment **directory;
} HashTable;
typedef struct SymbolTable {
    HashTable *hash;
    TextBuffer strings;
    StringNode *string_tree;
    External **sorted_by_name, **sorted_by_address, **sorted_by_symbol;
    External *undefined;
    External **undefined_tail;
} SymbolTable;

const char *symbol_key(const void *,void *);
void initialize_hash(HashTable **,unsigned long,unsigned long,HashKey,unsigned short);
void initialize_symbols(SymbolTable **);
#endif
