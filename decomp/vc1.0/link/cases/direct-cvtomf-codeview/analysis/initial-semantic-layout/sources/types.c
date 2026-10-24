/* Semantic description of fixture data/debug declarations, not compiler output.
 * The independent Python writer emits an ordinary OMF object with C6 records.
 * This file is not reconstruction implementation and is not linked into LINK.
 */
typedef union Value { long integer; double real; } Value;
typedef struct Node {
    Value value;
    unsigned reserved:1;
    unsigned flags:3;
    struct Node *next;
} Node;
typedef Node Nodes4[4];
typedef long Longs4[4];
typedef long (__cdecl *LongProcedure)(void);
typedef LongProcedure Callbacks2[2];
typedef Node *NodePtrs2[2];
Node *__cdecl legacy_identity(register Node *head) { __asm nop; return head; }
long __cdecl legacy_value(void) { long result=42; return result; }
/* The assembler supplies four linked Node values, a head pointer, two callbacks,
 * four numbers and two Node pointers. Their complete data/relocations are emitted
 * by make-c6-omf.py and described by inputs/layout.json.
 */
