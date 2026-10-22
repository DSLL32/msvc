#ifndef BITSETS_H
#define BITSETS_H
#include "machine.h"
/* A sentinel with base -1 precedes sorted 32-member chunks. */
struct BitSet {
    BitSet *next;
    int base;
    unsigned bits;
};
typedef char BitSetSizeCheck[sizeof(BitSet) == 12 ? 1 : -1];
BitSet *new_bitset(void);
BitSet *new_bitset_node(void);
BitSet *__fastcall bitset_include(BitSet *set, unsigned member);
int __fastcall bitset_contains(BitSet *set, unsigned member);
#endif
