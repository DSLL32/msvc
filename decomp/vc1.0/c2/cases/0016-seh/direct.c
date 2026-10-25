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
