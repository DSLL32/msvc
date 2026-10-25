#include <recon-tool.h>
#include <recon-native.h>
#include "exception-scope.h"
#include "backend.h"
#include <recon.h>
extern unsigned exception_scope_stack_depth;
struct Arena { void *blocks; char *next; unsigned remaining; int heap_flags; };
extern Arena arenas[8];
extern int main(int,char **,char **);
static ExceptionScope *__cdecl query_entry(void)
{
    Arena *arena=&arenas[1];unsigned i;char *record=arena->next-48;
    int valid=!prepared_exception_scopes&&next_exception_scope_identifier==1&&
        !exception_finally_depth&&!exception_scope_stack_depth&&sizeof(ExceptionScope)==48&&
        record>=(char *)arena->blocks+4&&arena->next<=(char *)arena->blocks+1024&&
        arena->remaining==(char *)arena->blocks+1024-arena->next;
    for(i=0;i<48;++i)if((unsigned char)record[i]!=0)valid=0;
    recon_native_entry_finish(valid);
}
static void real_input(void *argument)
{
    char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\C23232.EXE",0};
    main(1,argv,0);
}
int c2_scope_owned_record(void)
{
    return recon_native_at_entry((const void *)enclosing_try_scope,
        (const void *)query_entry,real_input,0)==1;
}
/* Observe the genuine allocator result at the next fastcall boundary. All
   twelve words are constructor-defined; no inactive stack or allocator tail
   is compared. The diagnostic original broker binds the same owner/fields. */
static void __fastcall push_entry(ExceptionScope *scope)
{
    Arena *arena=&arenas[1];unsigned i;unsigned char *record=(unsigned char *)scope;
    int valid=record==(unsigned char *)arena->next-48&&
        !prepared_exception_scopes&&next_exception_scope_identifier==1&&
        !exception_finally_depth&&!exception_scope_stack_depth&&
        record>=(unsigned char *)arena->blocks+4&&
        arena->next<=(char *)arena->blocks+1024&&
        arena->remaining==(char *)arena->blocks+1024-arena->next;
    for(i=0;i<48;++i)
        if(record[i]!=(i>=20&&i<24?255:0))valid=0;
    recon_native_entry_finish(valid);
}
int c2_scope_push_inputs(void)
{
    return recon_native_at_entry((const void *)push_exception_scope,
        (const void *)push_entry,real_input,0)==1;
}

/* Actual nested-scope frontier: only two live pointers are consumed. */
static ExceptionScope *__cdecl current_entry(void)
{
    Arena *arena=&arenas[1];
    int valid=exception_scope_stack_depth==2&&
        prepared_exception_scopes==exception_scope_stack[0]&&
        (char *)exception_scope_stack[1]==arena->next-48&&
        (char *)exception_scope_stack[0]>=(char *)arena->blocks+4&&
        exception_scope_stack[0]<exception_scope_stack[1]&&
        arena->next<=(char *)arena->blocks+1024&&
        arena->remaining==(char *)arena->blocks+1024-arena->next;
    recon_native_entry_finish(valid);
}
int c2_scope_current_inputs(void)
{
    return recon_native_at_entry((const void *)current_exception_scope,
        (const void *)current_entry,real_input,0)==1;
}

static void __cdecl pop_entry(void)
{
    Arena *arena=&arenas[1];
    int valid=exception_scope_stack_depth==2&&
        prepared_exception_scopes==exception_scope_stack[0]&&
        (char *)exception_scope_stack[1]==arena->next-48&&
        (char *)exception_scope_stack[0]>=(char *)arena->blocks+4&&
        exception_scope_stack[0]<exception_scope_stack[1]&&
        arena->next<=(char *)arena->blocks+1024&&
        arena->remaining==(char *)arena->blocks+1024-arena->next;
    recon_native_entry_finish(valid);
}
int c2_scope_pop_inputs(void)
{
    return recon_native_at_entry((const void *)pop_exception_scope,
        (const void *)pop_entry,real_input,0)==1;
}

/* ECX is the actual current Function owner. Only expressions is consumed
   by the rewrite driver; other raw Function words are not an input claim. */
static void __fastcall function_owner_entry(Function *function)
{
    int valid=function==current_function&&function->expressions&&
        function->expressions->next&&function->expressions->tree&&
        function->expressions->tree->opcode==0x56&&
        prepared_exception_scopes&&!exception_scope_stack_depth&&
        !exception_scope_stack[0]&&!exception_scope_stack[1];
    recon_native_entry_finish(valid);
}
int c2_scope_rewrite_inputs(void)
{
    return recon_native_at_entry((const void *)rewrite_exception_expressions,
        (const void *)function_owner_entry,real_input,0)==1;
}

static Node *__fastcall annotation_entry(unsigned short code)
{
    int valid=code==0x26&&exception_scope_stack_depth==1&&
        prepared_exception_scopes==exception_scope_stack[0]&&
        !exception_scope_stack[1];
    recon_native_entry_finish(valid);
}
int c2_scope_annotation_inputs(void)
{
    return recon_native_at_entry((const void *)new_exception_annotation,
        (const void *)annotation_entry,real_input,0)==1;
}

static Node *__fastcall unwind_entry(Expression *previous,Node *before,long target)
{
    ExceptionScope *scope=exception_scope_stack[1];
    int valid=exception_scope_stack_depth==2&&
        prepared_exception_scopes==exception_scope_stack[0]&&scope&&
        scope->kind==2&&scope->state==0x8f&&scope->identifier==1&&
        !scope->finally_depth&&!scope->parent_identifier&&
        previous&&previous->next&&previous->next->tree==before&&
        previous->source_location==43&&before->opcode==0x3a&&target==1&&
        ((LabelSymbol *)before->right.symbol)->field28==(unsigned)target;
    recon_native_entry_finish(valid);
}
int c2_scope_unwind_inputs(void)
{
    return recon_native_at_entry((const void *)insert_exception_unwind,
        (const void *)unwind_entry,real_input,0)==1;
}

/* This constructor consumes only ECX; other registers are caller residue. */
static Node *__fastcall scope_finally_entry(long target)
{
    recon_native_entry_finish(target==-1);
}
int c2_scope_finally_inputs(void)
{
    return recon_native_at_entry((const void *)new_scope_finally_unwind,
        (const void *)scope_finally_entry,real_input,0)==1;
}

static Node *__fastcall finally_entry(unsigned depth)
{
    recon_native_entry_finish(depth==0);
}
int c2_finally_inputs(void)
{
    return recon_native_at_entry((const void *)new_finally_unwind,
        (const void *)finally_entry,real_input,0)==1;
}

int c2_scope_finish_inputs(void)
{
    return recon_native_at_entry((const void *)finish_inline_function,
        (const void *)function_owner_entry,real_input,0)==1;
}

static void __fastcall collect_entry(Node *node,unsigned filter_depth)
{
    ExceptionScope *scope=exception_scope_stack[1];
    int valid=!filter_depth&&node->opcode==0x32&&
        node->attributes.value==0x8010104&&!node->info.value&&
        node->left.node&&node->second.node&&node->right.symbol&&
        node->detail.node==node&&exception_scope_stack_depth==2&&scope&&
        scope->kind==2&&scope->state==0x8e&&scope->identifier==1&&
        !scope->finally_depth&&!scope->parent_identifier;
    recon_native_entry_finish(valid);
}
int c2_scope_collect_inputs(void)
{
    return recon_native_at_entry((const void *)collect_exception_references,
        (const void *)collect_entry,real_input,0)==1;
}

static void __fastcall remember_entry(SymbolHeader *symbol)
{
    ExceptionScope *scope=exception_scope_stack[0];
    int valid=symbol->kind==1&&!symbol->field0d&&symbol->value==4&&
        symbol->field0e==0x100&&symbol->name&&
        symbol->field18.identifier==6&&symbol->identifier==0xa6&&
        exception_scope_stack_depth==1&&scope==prepared_exception_scopes&&
        scope->kind==1&&scope->state==0x8d&&!scope->identifier&&
        !scope->finally_depth&&scope->parent_identifier==-1&&!scope->locals;
    recon_native_entry_finish(valid);
}
int c2_scope_remember_inputs(void)
{
    return recon_native_at_entry((const void *)remember_exception_local,
        (const void *)remember_entry,real_input,0)==1;
}

static SymbolHeader *__fastcall find_local_entry(ExceptionLocalSet *set,unsigned identifier)
{
    Arena *arena=&arenas[4];unsigned i;
    int valid=identifier==0xa6&&exception_scope_stack_depth==1&&
        prepared_exception_scopes==exception_scope_stack[0]&&
        prepared_exception_scopes->locals==set&&
        (char *)set==(char *)arena->blocks+4&&arena->next==(char *)set+64&&
        arena->remaining==(char *)arena->blocks+1024-arena->next;
    for(i=0;i<16;++i)if(set->buckets[i])valid=0;
    recon_native_entry_finish(valid);
}
int c2_scope_find_local_inputs(void)
{
    return recon_native_at_entry((const void *)find_exception_local,
        (const void *)find_local_entry,real_input,0)==1;
}
