#include <recon-tool.h>
#include <recon-native.h>
#include "options.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
int __fastcall suffix_matches(char *pattern,char *text,unsigned action);
static void real_input(void *unused)
{char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\C23232.EXE",0};(void)unused;main(1,argv,0);}
static int __fastcall wildcard_entry(char *pattern,char *text,unsigned action)
{
 int result;
 if(strcmp(pattern,"*")||strcmp(text,"-reconstruction-unsupported-option")||action!=34)
  recon_native_entry_finish(0);
 result=suffix_matches(pattern,text,action);
 recon_native_entry_finish(result==1&&!strcmp(pattern,"*")&&
  !strcmp(text,"-reconstruction-unsupported-option"));return 0;
}
static int actual_wildcard(void)
{return recon_native_at_entry((const void*)suffix_matches,(const void*)wildcard_entry,real_input,0)==1;}
/* Original cold scan can inspect the byte before the nominal text start.
   Give it an actual enclosing array and a defined non-star guard. These
   controls preserve the old assumption, not a universal string-domain claim. */
static int guarded_suffix(void)
{
 char prefixed[]="!prefix.OBJ",whole[]="!.OBJ",shorter[]="!BJ",wrong[]="!prefix.obj";
 return suffix_matches("*.OBJ",prefixed+1,34)==1&&
  suffix_matches("*.OBJ",whole+1,34)==0&&
  suffix_matches("*.OBJ",shorter+1,34)==0&&
  suffix_matches("*.OBJ",wrong+1,34)==0&&suffix_matches("*",0,34)==1;
}
const ReconTest option_tests[]={
 {"actual wildcard pointer strings and stack action",1,actual_wildcard},
 {"cold guarded suffix preserves original backward scan",1,guarded_suffix}
};
