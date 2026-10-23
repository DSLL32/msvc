#ifndef LINK_SYMBOLS_H
#define LINK_SYMBOLS_H
#include <windows.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef const char *(__stdcall *KeyNameFunction)(const void *,const void *);
typedef struct HashState HashState;
typedef struct HashTable {
    HashState *state;
    DWORD reserved[7];
} HashTable;
typedef struct StringNode StringNode;
typedef struct StringTable {
    HashTable hash;
    StringNode *first;
    StringNode **last;
} StringTable;
StringTable *__stdcall create_string_table(StringTable **,unsigned,unsigned);
void __stdcall initialize_hash_table(HashTable *,unsigned,unsigned,KeyNameFunction,WORD);
const char *__stdcall string_key_name(const void *,const void *);
typedef char StringTableSize[(sizeof(StringTable)==40)?1:-1];

#ifdef __cplusplus
}
#endif
#endif
