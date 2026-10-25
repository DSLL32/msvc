#ifndef C1XX_NAMES_H
#define C1XX_NAMES_H
#include <recon.h>

/* Common header of both actual 16-byte generic and 20-byte keyword allocations.
 * The macro pointer at original+c is shared. Token/category belong only to keyword
 * records at original +10/+11; no such fields exist in this header. */
typedef struct CompilerNameEntry {
    struct CompilerNameEntry *next;
    char *name;
    unsigned flags;
    struct CompilerMacro *macro; /* original+c, owned only when flags include0x80000 */
} CompilerNameEntry;

typedef struct CompilerBuiltinNames {
    CompilerNameEntry *return_udt; /* original 0xcc638 */
    CompilerNameEntry *this_parameter; /* original 0xcc640 */
    CompilerNameEntry *that_parameter; /* original 0xcc63c */
    CompilerNameEntry *vftable; /* original 0xcc680 */
    CompilerNameEntry *local_vftable; /* original 0xcc684 */
    CompilerNameEntry *vbtable; /* original 0xcc64c */
    CompilerNameEntry *vfptr; /* original 0xcc650 */
    CompilerNameEntry *vbptr; /* original 0xcc648 */
    CompilerNameEntry *vdispmap; /* original 0xcc654 */
    CompilerNameEntry *initialize_virtual_bases; /* original 0xcc61c */
    CompilerNameEntry *flags_parameter; /* original 0xcc618 */
    CompilerNameEntry *destructor; /* original 0xcc614 */
    CompilerNameEntry *vbase_destructor; /* original 0xcc644 */
    CompilerNameEntry *deleting_destructor; /* original 0xcc60c */
    CompilerNameEntry *vector_deleting_destructor; /* original 0xcc664 */
    CompilerNameEntry *assignment_operator; /* original 0xcc460 */
    CompilerNameEntry *most_derived_parameter; /* original 0xcc6f0 */
    CompilerNameEntry *operator_new; /* original 0xcc62c */
    CompilerNameEntry *operator_delete; /* original 0xcc610 */
    CompilerNameEntry *operator_new_array; /* original 0xcc630 */
    CompilerNameEntry *operator_delete_array; /* original 0xcc634 */
    CompilerNameEntry *constructor; /* original 0xcc608 */
    CompilerNameEntry *default_constructor_closure; /* original 0xcc6d8 */
    CompilerNameEntry *local_vftable_constructor_closure; /* original 0xcc6dc */
    CompilerNameEntry *copy_constructor_closure; /* original 0xcc6e0 */
    CompilerNameEntry *local_static_destructor; /* original 0xcc620 */
    CompilerNameEntry *template_static_constructor; /* original 0xcc624 */
    CompilerNameEntry *template_static_destructor; /* original 0xcc628 */
    CompilerNameEntry *vector_constructor; /* original 0xcc668 */
    CompilerNameEntry *vector_vbase_constructor; /* original 0xcc66c */
    CompilerNameEntry *vector_destructor; /* original 0xcc670 */
    CompilerNameEntry *eh_vector_constructor; /* original 0xcc674 */
    CompilerNameEntry *eh_vector_vbase_constructor; /* original 0xcc678 */
    CompilerNameEntry *eh_vector_destructor; /* original 0xcc67c */
    CompilerNameEntry *new_target; /* original 0xcc688 */
    CompilerNameEntry *atexit_name; /* original 0xcc68c */
    CompilerNameEntry *tls_destructor_registration; /* original 0xcc690 */
    CompilerNameEntry *global_namespace; /* original 0xcc694 */
    CompilerNameEntry *defined_operator; /* original 0xcc468 */
    CompilerNameEntry *formal_parameter; /* original 0xcc47c */
    CompilerNameEntry *ellipsis; /* original 0xcc478 */
    CompilerNameEntry *abstract_declarator; /* original 0xcc464 */
    CompilerNameEntry *main_entry; /* original 0xcc484 */
    CompilerNameEntry *winmain_entry; /* original 0xcc490 */
    CompilerNameEntry *dllmain_entry; /* original 0xcc494 */
    CompilerNameEntry *alignment_member; /* original 0xcc45c */
    CompilerNameEntry *unnamed; /* original 0xcc498 */
    CompilerNameEntry *dollar; /* original 0xcc46c */
    CompilerNameEntry *directive_section; /* original 0xcc470 */
    CompilerNameEntry *local_static_guard; /* original 0xcc480 */
    CompilerNameEntry *setjmp_name; /* original 0xcc488 */
    CompilerNameEntry *catch_name; /* original 0xcc48c */
    CompilerNameEntry *local_size; /* original 0xcc474 */
    CompilerNameEntry *rtti_dynamic_cast; /* original 0xccad8 */
    CompilerNameEntry *rtti_cast_to_void; /* original 0xccadc */
    CompilerNameEntry *rtti_typeid; /* original 0xccae0 */
    CompilerNameEntry *rtti_typeid_return_type; /* original 0xccae4 */
    CompilerNameEntry *type_info_vtable; /* original 0xccae8 */
    CompilerNameEntry *line_variable; /* original 0xccb08 */
    CompilerNameEntry *uuidof_return_type; /* original 0xccb18 */
    CompilerNameEntry *guid_tag; /* original 0xccb1c */
} CompilerBuiltinNames;
#endif
