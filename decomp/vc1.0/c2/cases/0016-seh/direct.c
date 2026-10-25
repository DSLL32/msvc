#include <recon-tool.h>
#include <recon-native.h>
#include "exception-scope.h"
#include "backend.h"
#include "global-registers.h"
#include "register-transfers.h"
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

static void __fastcall insert_local_entry(ExceptionLocalSet *set,SymbolHeader *symbol)
{
    Arena *arena=&arenas[4];unsigned i;
    int valid=symbol->identifier==0xa6&&symbol->kind==1&&
        exception_scope_stack_depth==1&&prepared_exception_scopes==exception_scope_stack[0]&&
        prepared_exception_scopes->locals==set&&(char *)set==(char *)arena->blocks+4&&
        arena->next==(char *)set+64&&
        arena->remaining==(char *)arena->blocks+1024-arena->next;
    for(i=0;i<16;++i)if(set->buckets[i])valid=0;
    recon_native_entry_finish(valid);
}
int c2_scope_insert_local_inputs(void)
{
    return recon_native_at_entry((const void *)insert_exception_local,
        (const void *)insert_local_entry,real_input,0)==1;
}

static void __fastcall mark_entry(Node *node,unsigned filter_depth)
{
    ExceptionScope *scope=exception_scope_stack[0];unsigned i;
    int valid=!filter_depth&&node->opcode==1&&exception_scope_stack_depth==1&&
        scope==prepared_exception_scopes&&scope->kind==1&&scope->state==0x8d&&
        !scope->identifier&&!scope->finally_depth&&scope->parent_identifier==-1&&
        scope->locals&&scope->flags==1;
    for(i=4;i<sizeof *node;++i)if(((unsigned char *)node)[i])valid=0;
    recon_native_entry_finish(valid);
}
int c2_scope_mark_inputs(void)
{
    return recon_native_at_entry((const void *)mark_exception_references,
        (const void *)mark_entry,real_input,0)==1;
}

static int __fastcall visible_entry(SymbolHeader *symbol)
{
    ExceptionScope *scope=exception_scope_stack[1];
    int valid=symbol->kind==1&&symbol->field04==~7U&&symbol->value==4&&
        symbol->field0e==0x900&&symbol->field18.identifier==6&&symbol->identifier==0xab&&
        exception_scope_stack_depth==2&&scope==prepared_exception_scopes->next&&
        scope->kind==2&&scope->state==0x8e&&scope->identifier==1&&
        !scope->parent_identifier&&!scope->locals;
    recon_native_entry_finish(valid);
}
int c2_scope_visible_inputs(void)
{
    return recon_native_at_entry((const void *)exception_local_visible,
        (const void *)visible_entry,real_input,0)==1;
}

static ExceptionScope *__fastcall lookup_scope_entry(ExceptionScope *scopes,long identifier)
{
    int valid=scopes==prepared_exception_scopes&&identifier==0&&!scopes->identifier&&
        exception_scope_stack_depth==2&&scopes->next==exception_scope_stack[1]&&
        scopes->next->identifier==1;
    recon_native_entry_finish(valid);
}
int c2_scope_lookup_inputs(void)
{
    return recon_native_at_entry((const void *)lookup_exception_scope,
        (const void *)lookup_scope_entry,real_input,0)==1;
}

static void __fastcall protected_edge_entry(WorkItem *source,WorkItem *target)
{
    ExceptionScope *scope=current_function->exception_scopes;
    int valid=source!=target&&!source->successors&&!target->predecessors&&
        source->body.expressions&&target->body.expressions&&
        target->body.expressions->tree->opcode==0x29&&
        target->body.expressions->tree->right.symbol==scope->handler_label&&
        source->exception_scope==(short)scope->identifier;
    recon_native_entry_finish(valid);
}
int c2_protected_edge_inputs(void)
{
    return recon_native_at_entry((const void *)add_protected_flow_edge,
        (const void *)protected_edge_entry,real_input,0)==1;
}

static void __fastcall refresh_region_entry(WorkItem *work)
{
    WorkItem *items[25],*cursor=work;FlowRegion *region=0;unsigned i;int valid=work==global_entry_work;
    for(i=0;i<25;++i){
        if(!cursor)recon_native_entry_finish(0);
        items[i]=cursor;if(cursor->number!=(short)i)valid=0;
        if(i==8||i==9||i==10||i==11||i==12||i==14||i==15){
            if(!cursor->region)valid=0;
            if(!region)region=cursor->region;
            if(cursor->region!=region)valid=0;
        }else if(cursor->region)valid=0;
        cursor=cursor->next;
    }
    if(cursor||!region||region->last!=items[15]||region->last->number!=15)valid=0;
    recon_native_entry_finish(valid);
}
int c2_refresh_region_inputs(void)
{
    return recon_native_at_entry((const void *)refresh_region_entries,
        (const void *)refresh_region_entry,real_input,0)==1;
}

static void __fastcall restore_excluded_entry(Node *node)
{
    int valid=node==global_entry_work->body.expressions->tree&&
        node->opcode==0x56&&!node->attributes.value&&!node->info.value&&
        !node->left.value&&!node->second.value&&node->right.pointer&&
        node->detail.value==2&&allocation_excluded_registers;
    recon_native_entry_finish(valid);
}
int c2_restore_excluded_inputs(void)
{
    return recon_native_at_entry((const void *)restore_excluded_registers,
        (const void *)restore_excluded_entry,real_input,0)==1;
}

static void __fastcall register_ends_entry(WorkItem *work,AllocationValue *value,AllocationRange *range)
{
    AllocationBinding *binding=&allocation_register_bindings[0x27];
    Node *tree=work->body.expressions->tree;
    int valid=work->next&&work->previous&&work->body.expressions->next&&
        value==binding->value&&range==binding->range&&!value->next&&
        !range->next&&range->number==0x4027&&range->flags==1&&
        value->node->opcode==0x30&&value->node->attributes.value==0x50104&&
        tree->opcode==0x29&&!tree->attributes.value&&!tree->info.value&&
        !tree->left.value&&!tree->second.value&&tree->right.symbol&&!tree->detail.value;
    recon_native_entry_finish(valid);
}
int c2_register_ends_inputs(void)
{
    return recon_native_at_entry((const void *)mark_register_definition_ends,
        (const void *)register_ends_entry,real_input,0)==1;
}
