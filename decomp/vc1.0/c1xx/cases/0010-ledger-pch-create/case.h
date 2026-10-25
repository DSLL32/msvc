#ifndef LEDGER_PCH_CASE_H
#define LEDGER_PCH_CASE_H
#include <stddef.h>
#include <stdint.h>

/* User-approved finite comparison for this frozen Ledger writer only.
   Every row is an actual allocation owner; fields selects its proven pointer
   DWORDs. Original/native allocation ownership and the reader's actual
   relocation map were checked independently. This is fixture metadata,
   never a serializer or a general rule for unmarked arena bytes. */
typedef struct LedgerPchOwner {
    uint16_t offset, bytes, fields;
} LedgerPchOwner;
static const LedgerPchOwner ledger_pch_owners[] = {
    { 0x9190, 20, 0x0006 }, /* define_macro */
    { 0xa0bc, 12, 0x0004 }, /* new_vector */
    { 0xa254, 52, 0x0200 }, /* new_declarator */
    { 0xa468, 20, 0x0005 }, /* enter_class_scope */
    { 0xa47c, 4, 0x0001 }, /* enter_class_scope */
    { 0xa484, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xa48c, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xa498, 16, 0x000c }, /* lookup_class_name */
    { 0xa4a8, 16, 0x000c }, /* lookup_class_name */
    { 0xa4b8, 16, 0x000c }, /* new_qualified_declarator */
    { 0xa4c8, 52, 0x000e }, /* new_declarator */
    { 0xa54c, 8, 0x0002 }, /* new_inline_vector */
    { 0xa558, 8, 0x0002 }, /* resize_permanent */
    { 0xa7a4, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xa7ac, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xa7b8, 16, 0x000c }, /* lookup_class_name */
    { 0xa7c8, 16, 0x000c }, /* lookup_class_name */
    { 0xa7d8, 16, 0x000c }, /* new_qualified_declarator */
    { 0xa7e8, 52, 0x000e }, /* new_declarator */
    { 0xa840, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xa848, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xa854, 16, 0x000c }, /* lookup_class_name */
    { 0xa864, 16, 0x000c }, /* lookup_class_name */
    { 0xa874, 16, 0x000c }, /* new_qualified_declarator */
    { 0xa884, 52, 0x000e }, /* new_declarator */
    { 0xaa50, 20, 0x000a }, /* new_ir_expression */
    { 0xaec0, 12, 0x0004 }, /* new_vector */
    { 0xaf5c, 52, 0x0200 }, /* new_declarator */
    { 0xb0c8, 20, 0x0005 }, /* enter_class_scope */
    { 0xb0dc, 4, 0x0001 }, /* enter_class_scope */
    { 0xb0e4, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xb0ec, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xb0f8, 16, 0x000c }, /* lookup_class_name */
    { 0xb108, 16, 0x000c }, /* lookup_class_name */
    { 0xb118, 16, 0x000c }, /* new_qualified_declarator */
    { 0xb128, 52, 0x000e }, /* new_declarator */
    { 0xb278, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xb280, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xb28c, 16, 0x000c }, /* lookup_class_name */
    { 0xb29c, 16, 0x000c }, /* lookup_class_name */
    { 0xb2ac, 16, 0x000c }, /* new_qualified_declarator */
    { 0xb2bc, 52, 0x000e }, /* new_declarator */
    { 0xb2f0, 16, 0x000c }, /* new_qualified_declarator */
    { 0xb370, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xb378, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xb384, 16, 0x000c }, /* lookup_class_name */
    { 0xb394, 16, 0x000c }, /* lookup_class_name */
    { 0xb3a4, 16, 0x000c }, /* new_qualified_declarator */
    { 0xb3b4, 52, 0x000e }, /* new_declarator */
    { 0xb40c, 8, 0x0002 }, /* resize_permanent */
    { 0xb414, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xb41c, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xb428, 16, 0x000c }, /* lookup_class_name */
    { 0xb438, 16, 0x000c }, /* lookup_class_name */
    { 0xb448, 16, 0x000c }, /* new_qualified_declarator */
    { 0xb458, 52, 0x000e }, /* new_declarator */
    { 0xb4ac, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xb4b4, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xb4c0, 16, 0x000c }, /* lookup_class_name */
    { 0xb4d0, 16, 0x000c }, /* lookup_class_name */
    { 0xb4e0, 16, 0x000c }, /* new_qualified_declarator */
    { 0xb4f0, 52, 0x000e }, /* new_declarator */
    { 0xb524, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xb52c, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xb538, 16, 0x000c }, /* lookup_class_name */
    { 0xb548, 16, 0x000c }, /* lookup_class_name */
    { 0xb558, 16, 0x000c }, /* new_qualified_declarator */
    { 0xb568, 52, 0x000e }, /* new_declarator */
    { 0xb5ac, 8, 0x0003 }, /* append_qualifier_reference */
    { 0xb5b4, 12, 0x0006 }, /* append_qualifier_reference */
    { 0xb5c0, 16, 0x000c }, /* lookup_class_name */
    { 0xb5d0, 16, 0x000c }, /* lookup_class_name */
    { 0xb5e0, 16, 0x000c }, /* new_qualified_declarator */
    { 0xb5f0, 52, 0x000e }, /* new_declarator */
    { 0xb644, 52, 0x000c }, /* new_declarator */
    { 0xb6c0, 20, 0x000a }, /* new_ir_expression */
    { 0xb72c, 16, 0x0008 }, /* new_qualified_declarator */
};
#define LEDGER_PCH_BYTES 53932u
#define LEDGER_PAGE_OFFSET 0x19d4u
#define LEDGER_PAGE_BYTES 0xb8d8u
#define LEDGER_ORIGINAL_PAGE 0x7e1000u

int ledger_pch_compare(const unsigned char *actual, const unsigned char *oracle,
    size_t bytes);
unsigned char *ledger_pch_oracle(const char *assets);
extern const ReconTest ledger_pch_tests[1];
#endif
