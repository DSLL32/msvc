#include <recon-tool.h>
#include "exception-scope.h"
#include "c2.h"
#include <recon-native.h>
#include <recon.h>
extern int c2_scope_owned_record(void);
extern int c2_scope_push_inputs(void);
extern int c2_scope_current_inputs(void);
/* Cold valid-stack controls support the recovered whole45B search. These
   synthetic kind lists are not claimed executed by the current public IR. */
static int enclosing_scopes(void)
{
    static const struct { unsigned depth, zero, expected; } rows[] = {
        {0,~0U,~0U}, {1,0,0}, {1,~0U,~0U}, {2,0,0}, {2,1,1},
        {3,1,1}, {20,0,0}, {20,9,9}, {20,19,19}, {20,~0U,~0U}
    };
    ExceptionScope records[20]; unsigned i,j;
    for(i=0;i<RECON_COUNT(rows);++i){
        memset(records,0,sizeof records);
        for(j=0;j<20;++j){records[j].kind=(j&1)+1;exception_scope_stack[j]=&records[j];}
        if(rows[i].zero!=~0U){records[0].kind=0;records[rows[i].zero].kind=0;}
        exception_scope_stack_depth=rows[i].depth;
        if(enclosing_try_scope()!=(rows[i].expected==~0U?0:&records[rows[i].expected]))return 0;
        if(exception_scope_stack_depth!=rows[i].depth)return 0;
        for(j=0;j<20;++j)
            if(exception_scope_stack[j]!=&records[j]||records[j].kind!=((j==rows[i].zero||(j==0&&rows[i].zero!=~0U))?0:(j&1)+1))return 0;
    }
    return 1;
}
/* Pointer-only store: the helper never consumes scope pointee fields. */
static int push_scopes(void)
{
    ExceptionScope records[2];unsigned i,j,k;
    static const unsigned depths[]={0,1,18,19};
    memset(records,0x5a,sizeof records);
    for(i=0;i<RECON_COUNT(depths);++i)for(k=0;k<2;++k){
        ExceptionScope *value=k?&records[0]:0;
        for(j=0;j<20;++j)exception_scope_stack[j]=&records[1];
        exception_scope_stack_depth=depths[i];push_exception_scope(value);
        if(exception_scope_stack_depth!=depths[i]+1)return 0;
        for(j=0;j<20;++j)
            if(exception_scope_stack[j]!=(j==depths[i]?value:&records[1]))return 0;
        for(j=0;j<sizeof records;++j)if(((unsigned char *)records)[j]!=0x5a)return 0;
    }
    return 1;
}
static void __fastcall overflow_entry(char *source,unsigned code)
{
    recon_native_entry_finish(code==0x301&&!strcmp(source,"@(#)except.c:1.25"));
}
static void overflowing_push(void *argument)
{
    exception_scope_stack_depth=20;push_exception_scope((ExceptionScope *)argument);
}
static int push_overflow_boundary(void)
{
    ExceptionScope scope;
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)overflow_entry,overflowing_push,&scope)==1;
}
const ReconTest c2_tests[]={
    {"actual SEH owned48B allocation at parent query",1,c2_scope_owned_record},
    {"nearest active try scope through valid20-slot stack",1,enclosing_scopes},
    {"actual fastcall scope pointer/defined48B before push",1,c2_scope_push_inputs},
    {"scope push opaque-pointer store and20-slot boundary",1,push_scopes},
    {"scope push overflow reaches exact pending diagnostic",1,push_overflow_boundary},
    {"actual current query consumes two live owned pointers",1,c2_scope_current_inputs}
};
