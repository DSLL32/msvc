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
extern BitSet *free_bitset_nodes;
BitSet *new_bitset(void);
BitSet *new_bitset_node(void);
BitSet *__fastcall bitset_include(BitSet *set, unsigned member);
int __fastcall bitset_contains(BitSet *set, unsigned member);
void __fastcall release_bitset(BitSet *set);
BitSet *__fastcall clear_bitset(BitSet *set);
BitSet *__fastcall bitset_union(BitSet *destination, BitSet *source);
BitSet *__fastcall bitset_remove(BitSet *set, unsigned member);
BitSet *__fastcall copy_bitset(BitSet *source);
BitSet *__fastcall ensure_bitset(BitSet *set, int capacity);
BitSet *__fastcall fill_bitset(BitSet *set, int count);
BitSet *__fastcall intersect_bitset(BitSet *destination, BitSet *source);
int __fastcall equal_bitsets(BitSet *left, BitSet *right);
BitSet *__fastcall assign_bitset(BitSet *destination, BitSet *source);
BitSet *__fastcall subtract_bitset(BitSet *destination, BitSet *source);
void __fastcall transfer_bitset(BitSet *destination, BitSet *source, BitSet *removed, BitSet *added);
void __fastcall assign_union_bitset(BitSet *destination, BitSet *left, BitSet *right);
void __fastcall assign_difference_bitset(BitSet *destination, BitSet *source, BitSet *removed);
int __fastcall next_bitset_member(BitSet *set);
int __fastcall bitsets_overlap(BitSet *left, BitSet *right);
#endif
