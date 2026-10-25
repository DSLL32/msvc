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
