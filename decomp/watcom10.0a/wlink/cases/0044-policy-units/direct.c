/* Valid typed helper states; disabled ORL is not COFF capability. */
#include <string.h>
#include <recon-tool.h>
#include "../../src/cmdline.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) return 0; } while(0)
extern unsigned long ORLPass1( void );
extern void ORLPass2( void );
int policy_unit( void ) { return IsORL(0,0)==0; }
static int no_reloc_policy( void )
{
 LinkState=0xa5a5;
 CHECK(ProcPENoRelocs()==1 && LinkState==0xa5a4);
 CHECK(ProcPENoRelocs()==1 && LinkState==0xa5a4);
 return 1;
}
static int language_selection( void )
{
 static const char *names[]={"Japanese","Chinese","Korean"};
 static const unsigned flags[]={0x1000,0x2000,0x3000};
 char *line;unsigned n;
 for(n=0;n<3;++n) {
  line=ChkLAlloc(strlen(names[n])+1);strcpy(line,names[n]);
  FmtData.type=0xffff;LinkState=1;CmdFlags=0x3010;
  NewCommandSource(0,line,BUFFERED);
  CHECK(ProcLanguage()==1 && CmdFlags==(flags[n]|0x10));
  CHECK(Token.this==line && Token.len==strlen(names[n]));
  /* Stop immediately after selection. No multibyte token is lexed with
   * the new flag, and no English/multibyte capability is inferred. */
  BurnUtils();CHECK(CmdFile==0);
 }
 return 1;
}
static int disabled_orl( void )
{
 file_list owner,expected;
 memset(&owner,0x5a,sizeof(owner));expected=owner;
 CHECK(IsORL(&owner,0xffffffffUL)==0);
 CHECK(memcmp(&owner,&expected,sizeof(owner))==0);
 CHECK(ORLPass1()==0);ORLPass2();
 CHECK(memcmp(&owner,&expected,sizeof(owner))==0);
 return 1;
}
const ReconTest policy_unit_tests[]={
 {"shadowed-PE-relocation-bit",1,no_reloc_policy},
 {"owned-command-language-selection",1,language_selection},
 {"disabled-ORL-zero-noop",1,disabled_orl}
};
