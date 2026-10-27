/* Valid typed owners and public parser/message policy, no console delivery. */
#include <string.h>
#include <stdio.h>
#include <recon-tool.h>
#include "../../src/diagnostic.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"final unit %d: %s\n",__LINE__,#x); return 0; } } while(0)
extern unsigned char AddDisable( void );
extern sysblock *FindSysBlock( char * );
int final_control_unit( void )
{
 tok expected;
 expected=Token;ExecSystem("absent");
 return CmdFile==0 && memcmp(&Token,&expected,sizeof(Token))==0;
}
static int disable_message( const char *text, unsigned number )
{
 char *owner;
 memset(MsgFlags,0xff,sizeof(MsgFlags));
 owner=ChkLAlloc(strlen(text)+1);strcpy(owner,text);
 NewCommandSource(0,owner,BUFFERED);
 CHECK(GetToken(SEP_NO,TOK_INCLUDE_DOT)==1);
 CHECK(AddDisable()==1 && TestBit(MsgFlags,number)==0);
 BurnUtils();CHECK(CmdFile==0);
 return 1;
}
static int system_directive_owner( void )
{
 static const char text[]="begin unit option quiet option osname=UnitOS end";
 char *owner;sysblock *sys;
 LinkState=1;LinkFlags=0;FmtData.type=0xffff;
 owner=ChkLAlloc(sizeof(text));memcpy(owner,text,sizeof(text));
 NewCommandSource(0,owner,BUFFERED);
 CHECK(ProcSystem()==1);sys=FindSysBlock("unit");
 CHECK(sys!=0 && sys->name!=0 && strcmp(sys->name,"unit")==0);
 CHECK(strstr(sys->commands,"quiet")!=0 && strstr(sys->commands,"UnitOS")!=0);
 BurnUtils();CHECK(CmdFile==0);
 ExecSystem("unit");
 /* ExecSystem has burned both SYSTEM and command-source owners. Do not
  * dereference the old sys/name aliases after that real cleanup. */
 CHECK(CmdFile==0 && FindSysBlock("unit")==0);
 CHECK((LinkFlags&0x100)!=0 && strcmp(FmtData.osname,"UnitOS")==0);
 _LnkFree(FmtData.osname);FmtData.osname=0;
 return 1;
}
static int early_eof_owner( void )
{
 mod_entry module,module_before;file_list file,file_before;
 infilelist *owner;
 unsigned char before[64];unsigned bytes;
 CHECK(disable_message("29",29));
 owner=AllocFileEntry("unit.obj",0);bytes=sizeof(*owner)+strlen(owner->name);
 CHECK(bytes<=sizeof(before));memcpy(before,owner,bytes);
 memset(&module,0,sizeof(module));memset(&file,0,sizeof(file));
 file.file=owner;module.f.source=&file;module.name="unit-module";
 module_before=module;file_before=file;CurrMod=&module;
 LinkState=0;EarlyEOF();
 before[28]|=4;
 CHECK(owner->flags&4);
 CHECK(memcmp(owner,before,bytes)==0 && memcmp(&module,&module_before,sizeof(module))==0);
 CHECK(memcmp(&file,&file_before,sizeof(file))==0 && LinkState==0);
 CurrMod=0;RelSymBlock();
 return 1;
}
static int caught_break_callback( void )
{
 unsigned long flags;unsigned short state;
 CHECK(disable_message("13",13));
 flags=LinkFlags;state=LinkState;
 TrapBreak(4);
 CheckBreak();
 CheckBreak();
 CHECK(LinkFlags==flags && LinkState==state && CmdFile==0);
 return 1;
}
const ReconTest final_control_tests[]={
 {"owned-default-SYSTEM-directives",1,system_directive_owner},
 {"message-disabled-EOF-owner-context",1,early_eof_owner},
 {"message-disabled-caught-break-reset",1,caught_break_callback}
};
