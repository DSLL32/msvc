/* Exact old reset fields, valid borrowed owners and independent cleanup. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <recon-tool.h>
#include "../../src/cmdline.h"
#include "../../src/diagnostic.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"reset unit %d: %s\n",__LINE__,#x); return 0; } } while(0)
extern unsigned NameLen;
extern unsigned char BannerPrinted;
extern sysblock *SysBlocks;
extern list_of_names *GetNewName( void );
int subsystem_reset_unit( void )
{
    ResetComdef();
    ResetCmdAll();
    ResetDistrib();
    ResetMsg();
    ResetObjIO();
    ResetObjPass1();
    ResetObjStrip();
    ResetSymTrace();
    ResetODBI();
    return 1;
}
static int borrowed_reset_owners( void )
{
    struct { unsigned char before; unsigned char bytes[64]; unsigned char after; } buffer;
    saved_seg seg, saved;
    sysblock system, expected;
    section **sections;
    path_entry *path;
    unsigned i;

    memset(&buffer,0xa5,sizeof(buffer));memset(&seg,0x69,sizeof(seg));saved=seg;
    memset(&system,0x37,sizeof(system));expected=system;
    path=malloc(sizeof(*path));sections=malloc(2*sizeof(*sections));
    CHECK(path!=0 && sections!=0);
    memset(path,0x56,sizeof(*path));memset(sections,0x27,2*sizeof(*sections));
    SavedSegs=&seg;MapFlags=0x79;SymFileName=(char *)buffer.bytes;NameLen=93;
    SysBlocks=&system;LibPath=path;SectOvlTab=sections;
    BannerPrinted=1;memset(MsgFlags,0x6a,sizeof(MsgFlags));
    ObjBuff=buffer.bytes+3;EOObjRec=buffer.bytes+53;
    ObjFormat=0x5a5a;NextSegIndex=37;SavedSegMask=buffer.bytes+17;
    ResetComdef();CHECK(SavedSegs==0);
    ResetCmdAll();CHECK(MapFlags==0 && SymFileName==0 && NameLen==0 && SysBlocks==0 && LibPath==0);
    ResetDistrib();CHECK(SectOvlTab==0);
    ResetMsg();CHECK(BannerPrinted==0);
    for(i=0;i<sizeof(MsgFlags);++i)CHECK(MsgFlags[i]==0x6a);
    ResetObjIO();CHECK(ObjBuff==0 && EOObjRec==0);
    ResetObjPass1();CHECK(ObjFormat==0 && NextSegIndex==0 && SavedSegMask==0);
    ResetObjStrip();ResetSymTrace();ResetODBI();
    CHECK(memcmp(&seg,&saved,sizeof(seg))==0 && memcmp(&system,&expected,sizeof(system))==0);
    CHECK(buffer.before==0xa5 && buffer.after==0xa5);
    for(i=0;i<sizeof(buffer.bytes);++i)CHECK(buffer.bytes[i]==0xa5);
    for(i=0;i<sizeof(*path);++i)CHECK(((unsigned char *)path)[i]==0x56);
    for(i=0;i<2*sizeof(*sections);++i)CHECK(((unsigned char *)sections)[i]==0x27);
    /* Reset forgets pointers; it neither destroys nor frees these owners. */
    free(path);free(sections);
    return 1;
}
static int comdat_names_and_idempotence( void )
{
    list_of_names *a,*b,*c;
    LinkState=1;ResetComdef();
    a=GetNewName();b=GetNewName();
    CHECK(strcmp(a->name,"COMDAT_SEG1")==0 && strcmp(b->name,"COMDAT_SEG2")==0);
    ResetComdef();ResetComdef();c=GetNewName();
    CHECK(strcmp(c->name,"COMDAT_SEG1")==0);
    CHECK(strcmp(a->name,"COMDAT_SEG1")==0 && strcmp(b->name,"COMDAT_SEG2")==0);
    ResetCmdAll();ResetCmdAll();ResetDistrib();ResetDistrib();ResetMsg();ResetMsg();
    ResetObjIO();ResetObjIO();ResetObjPass1();ResetObjPass1();
    ResetObjStrip();ResetObjStrip();ResetSymTrace();ResetSymTrace();ResetODBI();ResetODBI();
    RelSymBlock();
    return 1;
}
const ReconTest subsystem_reset_tests[]={
 {"borrowed-field-owners-no-free-and-untouched-neighbors",1,borrowed_reset_owners},
 {"COMDAT-name-restart-and-idempotent-empty-resets",1,comdat_names_and_idempotence}
};
