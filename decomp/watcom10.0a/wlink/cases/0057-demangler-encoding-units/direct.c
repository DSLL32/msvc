/* Borrowed scope views, real callback-owned reallocations and DWARF bytes. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <recon-tool.h>
#include "../../src/demangle.h"
#include "../../src/cmdline.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"demangle/encoding %d: %s\n",__LINE__,#x); return 0; } } while(0)
int demangler_encoding_unit( void )
{
    char buffer[3];memset(buffer,0xa5,sizeof(buffer));
    return DW_ULEB128(buffer,128)==buffer+2 && (unsigned char)buffer[0]==0x80 && buffer[1]==1 && buffer[2]==(char)0xa5;
}
static unsigned reallocations;
static char *live_output;
static int bad_owner;
static char *resize_output( char *old, unsigned size )
{
    char *next;
    if(old!=live_output){bad_owner=1;return 0;}
    next=realloc(old,size);
    if(next!=0){live_output=next;++reallocations;}
    return next;
}
static int owned_demangler( void )
{
    static const char input[]="W?a$ni",expected[]="int near a";
    struct { unsigned char before;char bytes[64];unsigned char after; } fixed;
    char *output;unsigned count;
    memset(&fixed,0xa5,sizeof(fixed));output=fixed.bytes;
    count=__demangle_r(input,0,&output,sizeof(fixed.bytes),0);
    CHECK(count==sizeof(expected)-1 && output==fixed.bytes && strcmp(output,expected)==0);
    CHECK(fixed.before==0xa5 && fixed.after==0xa5);
    live_output=malloc(3);CHECK(live_output!=0);memset(live_output,0xa5,3);
    output=live_output;reallocations=0;bad_owner=0;
    count=__demangle_r(input,0,&output,3,resize_output);
    CHECK(!bad_owner && reallocations>0 && output==live_output);
    CHECK(count==sizeof(expected)-1 && strcmp(output,expected)==0);
    free(output);live_output=0;
    memset(&fixed,0xa5,sizeof(fixed));output=fixed.bytes;
    CHECK(__demangle_r("plainTail",5,&output,sizeof(fixed.bytes),0)==5 && strcmp(output,"plain")==0);
    CHECK(fixed.before==0xa5 && fixed.after==0xa5);
    return 1;
}
static int borrowed_scope( void )
{
    static const char simple[]="W?a$ni",scoped[]="W?foo$:Scope$n()v",plain[]="plain";
    struct { unsigned before;const char *view;unsigned after; } pointer;
    struct { unsigned before,size,after; } length;
    pointer.before=length.before=0x12345678;pointer.after=length.after=0x89abcdef;
    pointer.view=plain;length.size=99;
    CHECK(__scope_name(simple,0,0,&pointer.view,&length.size)==1);
    CHECK(pointer.view==simple+2 && length.size==1 && *pointer.view=='a');
    CHECK(__scope_name(scoped,0,0,&pointer.view,&length.size)==1);
    CHECK(pointer.view==scoped+2 && length.size==3 && memcmp(pointer.view,"foo",3)==0);
    CHECK(__scope_name(scoped,0,1,&pointer.view,&length.size)==1);
    CHECK(pointer.view==scoped+7 && length.size==5 && memcmp(pointer.view,"Scope",5)==0);
    CHECK(__scope_name(plain,0,0,&pointer.view,&length.size)==0 && pointer.view==0 && length.size==0);
    CHECK(pointer.before==0x12345678 && pointer.after==0x89abcdef && length.before==0x12345678 && length.after==0x89abcdef);
    CHECK(strcmp(simple,"W?a$ni")==0 && strcmp(scoped,"W?foo$:Scope$n()v")==0);
    return 1;
}
static int unsigned_leb128( void )
{
    static const unsigned long values[]={0,1,63,64,127,128,129,255,16383,16384,0x7fffffffUL,0x80000000UL,0xffffffffUL};
    static const unsigned char expected[][5]={{0},{1},{63},{64},{127},{0x80,1},{0x81,1},{0xff,1},{0xff,0x7f},{0x80,0x80,1},{0xff,0xff,0xff,0xff,7},{0x80,0x80,0x80,0x80,8},{0xff,0xff,0xff,0xff,15}};
    static const unsigned lengths[]={1,1,1,1,1,2,2,2,2,3,5,5,5};
    struct { unsigned char before;char bytes[5];unsigned char after; } owner;
    unsigned i,k;char *end;
    for(i=0;i<RECON_COUNT(values);++i){
        memset(&owner,0xa5,sizeof(owner));end=DW_ULEB128(owner.bytes,values[i]);
        CHECK(end==owner.bytes+lengths[i] && memcmp(owner.bytes,expected[i],lengths[i])==0);
        CHECK(owner.before==0xa5 && owner.after==0xa5);
        for(k=lengths[i];k<sizeof(owner.bytes);++k)CHECK(owner.bytes[k]==(char)0xa5);
    }
    return 1;
}
const ReconTest demangler_encoding_tests[]={
 {"owned-output-and-live-reallocator-callback",1,owned_demangler},
 {"borrowed-input-scope-views-and-output-canaries",1,borrowed_scope},
 {"unsigned-LEB128-boundaries-return-and-canaries",1,unsigned_leb128}
};
