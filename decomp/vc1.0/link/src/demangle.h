#ifndef LINK_DEMANGLE_H
#define LINK_DEMANGLE_H
#include "symbols.h"
#include <recon.h>
typedef void *(__cdecl *DemangleAllocate)(unsigned);
typedef void (__cdecl *DemangleRelease)(void *);
char *demangle_symbol(char *,const char *,int,DemangleAllocate,DemangleRelease,unsigned short);
/* The1.0 C parser rejects __thiscall. A fastcall method reserves EDX as an
 * ignored argument, preserving native ECX-this, stack arguments and RET cleanup.
 * No adapter or extra graph vertex is emitted. See SOURCE-NOTES.txt. */
typedef enum DemangleNodeKind {
    DEMANGLE_CHARACTER,DEMANGLE_TEXT,DEMANGLE_REFERENCE,DEMANGLE_STATUS
} DemangleNodeKind;
#pragma pack(2)
typedef struct DemangleNode {
    DemangleNodeKind kind;
    struct DemangleNode *next;
} DemangleNode;
typedef struct DemangleStatusNode {
    DemangleNode base;
    int status,length;
} DemangleStatusNode;
typedef struct DemangleCharNode {
    DemangleNode base;
    char character;
} DemangleCharNode;
typedef struct DemangleTextNode {
    DemangleNode base;
    char *text;
    int length;
} DemangleTextNode;
#pragma pack()
typedef char DemangleNodeSize[(sizeof(DemangleNode)==8)?1:-1];
typedef char DemangleStatusNodeSize[(sizeof(DemangleStatusNode)==16)?1:-1];
typedef char DemangleCharNodeSize[(sizeof(DemangleCharNode)==10)?1:-1];
typedef char DemangleTextNodeSize[(sizeof(DemangleTextNode)==16)?1:-1];
typedef struct DemangleName {
    DemangleNode *head;
    unsigned long flags;
} DemangleName;
typedef struct DemangleReferenceNode {
    DemangleNode base;
    DemangleName *name;
} DemangleReferenceNode;
typedef char DemangleReferenceNodeSize[(sizeof(DemangleReferenceNode)==12)?1:-1];
typedef struct DemangleReplicator {
    int index;
    DemangleName *values[10];
    DemangleName error,missing;
} DemangleReplicator;
typedef struct DemangleBlock {
    struct DemangleBlock *next;
    unsigned char bytes[508];
} DemangleBlock;
typedef struct DemangleHeap {
    DemangleAllocate allocate;
    DemangleRelease release;
    DemangleBlock *head,*tail;
    unsigned remaining;
} DemangleHeap;
typedef char DemangleNameSize[(sizeof(DemangleName)==8)?1:-1];
typedef char DemangleReplicatorSize[(sizeof(DemangleReplicator)==60)?1:-1];
typedef char DemangleHeapSize[(sizeof(DemangleHeap)==20)?1:-1];
extern DemangleHeap demangle_heap;
extern DemangleReplicator *demangle_type_replicator,*demangle_name_replicator;
extern const char *demangle_original,*demangle_input;
extern char *demangle_output;
extern int demangle_capacity;
extern unsigned short demangle_flags;
DemangleName * __fastcall demangle_name_status(DemangleName *self,void *unused,int status);
DemangleName * __fastcall demangle_name_empty(DemangleName *self,void *unused);
DemangleName * __fastcall demangle_name_assign(DemangleName *self,void *unused,const DemangleName *source);
DemangleName * __fastcall demangle_name_assign_text(DemangleName *self,void *unused,const char *text);
int __fastcall demangle_name_length(DemangleName *self,void *unused);
char * __fastcall demangle_name_write(DemangleName *self,void *unused,char *output,int capacity);
void * __fastcall demangle_allocate(DemangleHeap *self,void *unused,unsigned size,int standalone);
DemangleName * demangle_parse_declaration(DemangleName *result);
DemangleName * demangle_prepend_text(DemangleName *result,const char *text,const DemangleName *source);
DemangleStatusNode *__fastcall demangle_status_node(DemangleStatusNode *self,void *unused,int status);
DemangleNode *__fastcall demangle_node_initialize(DemangleNode *self,void *unused,int kind);
int __fastcall demangle_name_is_empty(const DemangleName *self,void *unused);
int __fastcall demangle_name_is_valid(const DemangleName *self,void *unused);
DemangleName * __fastcall demangle_name_copy(DemangleName *self,void *unused,const DemangleName *source);
DemangleName * __fastcall demangle_name_plus_text(const DemangleName *self,void *unused,DemangleName *result,const char *text);
DemangleName * __fastcall demangle_name_plus_name(const DemangleName *self,void *unused,DemangleName *result,const DemangleName *source);
DemangleName * __fastcall demangle_name_plus_reference(const DemangleName *self,void *unused,DemangleName *result,DemangleName *source);
DemangleName * __fastcall demangle_name_append_reference(DemangleName *self,void *unused,DemangleName *source);
DemangleName * __fastcall demangle_name_assign_reference(DemangleName *self,void *unused,DemangleName *source);
DemangleName * demangle_parse_operator(DemangleName *result);
DemangleName * demangle_parse_name(DemangleName *result);
DemangleName * demangle_parse_scope(DemangleName *result);
DemangleName * demangle_parse_type_declaration(DemangleName *result,const DemangleName *name);
DemangleName * __fastcall demangle_replicator_get(DemangleReplicator *self,void *unused,int index);
DemangleReplicator * __fastcall demangle_replicator_add(DemangleReplicator *self,void *unused,const DemangleName *name);
DemangleName * __fastcall demangle_name_read_until(DemangleName *self,void *unused,const char **input,int terminator);
void __fastcall demangle_name_set_range(DemangleName *self,void *unused,const char *text,int length);
DemangleName * __fastcall demangle_name_assign_status(DemangleName *self,void *unused,int status);
DemangleCharNode * __fastcall demangle_char_node(DemangleCharNode *self,void *unused,char character);
DemangleTextNode * __fastcall demangle_text_node(DemangleTextNode *self,void *unused,const char *text,int length);
int demangle_text_length(const char *text);
char *demangle_copy_text(char *output,const char *input,unsigned length);
DemangleName * demangle_prepend_char(DemangleName *result,char character,const DemangleName *source);
DemangleName * __fastcall demangle_name_plus_char(const DemangleName *self,void *unused,DemangleName *result,char character);
DemangleName * demangle_parse_template(DemangleName *result);
DemangleName * demangle_parse_dimension(DemangleName *result);
DemangleName * __fastcall demangle_name_append_status(DemangleName *self,void *unused,int status);
DemangleName * __fastcall demangle_name_append_name(DemangleName *self,void *unused,const DemangleName *source);
DemangleNode * __fastcall demangle_node_wrap(DemangleNode *self,void *unused);
DemangleNode * __fastcall demangle_node_append(DemangleNode *self,void *unused,DemangleNode *next);
DemangleReferenceNode *__fastcall demangle_reference_node(DemangleReferenceNode *self,void *unused,DemangleName *name);
DemangleName *__fastcall demangle_name_append_text(DemangleName *self,void *unused,const char *text);
DemangleName * demangle_prepend_status(DemangleName *result,int status,const DemangleName *source);
DemangleName * demangle_parse_data_qualifiers(DemangleName *result,const DemangleName *type,char indirection,const DemangleName *scope,int member);
DemangleName * demangle_parse_vbtable_type(DemangleName *result);
DemangleName * __fastcall demangle_name_append_char(DemangleName *self,void *unused,char character);
DemangleName * demangle_parse_data_type(DemangleName *result,DemangleName *declarator);
DemangleName * demangle_parse_based(DemangleName *result);
DemangleName * __fastcall demangle_name_character(DemangleName *self,void *unused,char character);
const char * demangle_keyword(int keyword);
DemangleName * __fastcall demangle_name_merge_error(DemangleName *self,void *unused,const DemangleName *source);
DemangleName * demangle_parse_calling_convention(DemangleName *result);
DemangleName * demangle_parse_return_type(DemangleName *result,DemangleName *declarator);
DemangleName * demangle_parse_arguments(DemangleName *result);
DemangleName * demangle_parse_throw_types(DemangleName *result);
DemangleName * __fastcall demangle_name_reference(DemangleName *self,void *unused,DemangleName *source);
DemangleName * demangle_parse_indirect_type(DemangleName *result,unsigned attributes);
DemangleName * demangle_parse_primary_type(DemangleName *result,const DemangleName *declarator);
DemangleName * __fastcall demangle_name_text(DemangleName *self,void *unused,const char *text);
DemangleName * demangle_parse_basic_type(DemangleName *result,const DemangleName *declarator);
DemangleName * demangle_parse_pointer_type(DemangleName *result,const DemangleName *qualifier,const DemangleName *declarator,int pointer);
DemangleName *__fastcall demangle_name_assign_char(DemangleName *self,void *unused,char character);
int __fastcall demangle_node_length(DemangleNode *self,void *unused);
char *__fastcall demangle_node_write(DemangleNode *self,void *unused,char *output,int capacity);
char *__fastcall demangle_char_write(DemangleCharNode *self,void *unused,char *output,int capacity);
char *__fastcall demangle_text_write(DemangleTextNode *self,void *unused,char *output,int capacity);
char *__fastcall demangle_reference_write(DemangleReferenceNode *self,void *unused,char *output,int capacity);
char *__fastcall demangle_status_write(DemangleStatusNode *self,void *unused,char *output,int capacity);
DemangleName *demangle_parse_argument_list(DemangleName *result);
DemangleName *__fastcall demangle_name_plus_status(const DemangleName *self,void *unused,DemangleName *result,int status);
unsigned demangle_read_dimension(void);
DemangleName *demangle_parse_type_name(DemangleName *result);
DemangleName *__fastcall demangle_name_unsigned(DemangleName *self,void *unused,unsigned value);
#endif
