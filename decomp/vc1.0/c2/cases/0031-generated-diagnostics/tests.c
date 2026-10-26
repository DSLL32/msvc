#include "case.h"
#include <recon-native.h>
#include "diagnostics.h"
#include "options.h"
#include <errno.h>
#include <io.h>
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,argv,0);
}
/* Actual first WX wrapper has code220 and no message arguments. It emits the
   separately pinned C2220 text and increments the existing counter once. */
static void __cdecl error_entry(unsigned code,...)
{
 if(code!=220||compiler_errors)recon_native_entry_finish(0);
 compiler_error(code);
 recon_native_entry_finish(compiler_errors==1&&diagnostic_file&&
  diagnostic_file!=(FILE*)-1&&ftell(diagnostic_file)>0);
}
static int actual_error_count(void)
{return recon_native_at_entry((const void*)compiler_error,(const void*)error_entry,real_input,0)==1;}
/* Fixed ECX/EDX plus one stack filename: validate the real failed-open inputs
   before the nonreturning fatal path. No invented return from the target. */
static void __fastcall context_entry(unsigned code,char *source,char *filename)
{
 recon_native_entry_finish(code==83&&!strcmp(source,"source")&&
  !strcmp(filename,"generated-validation.c")&&compiler_errors==1&&errno==ENOENT);
}
static int actual_context(void)
{return recon_native_at_entry((const void*)report_context_error,(const void*)context_entry,real_input,0)==1;}
/* Borrow the actual diagnostic FILE. Descriptor closure must leave that FILE
   usable to retrieve fatal1083; retain its identity, position and descriptor.
   The final IO oracle independently proves exact fatal text and output deletion. */
static void close_entry(void)
{
 FILE *file=diagnostic_file;long position;int descriptor;
 if(!file||file==(FILE*)-1||compiler_errors!=1)recon_native_entry_finish(0);
 position=ftell(file);descriptor=(_fileno)(file);
 close_diagnostic_descriptors();
 recon_native_entry_finish(diagnostic_file==file&&(_fileno)(file)==descriptor&&
  ftell(file)==position&&fseek(file,position,SEEK_SET)==0&&
  strstr(diagnostic_format(1083),"Cannot open")!=0);
}
static int actual_close(void)
{return recon_native_at_entry((const void*)close_diagnostic_descriptors,(const void*)close_entry,real_input,0)==1;}
const ReconTest generated_tests[]={
 {"actual warning-as-error counter",1,actual_error_count},
 {"actual failed listing source arguments",1,actual_context},
 {"actual diagnostic FILE survives descriptor closure",1,actual_close}
};
