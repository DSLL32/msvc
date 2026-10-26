#include "case.h"
#include <recon-native.h>
#include "diagnostics.h"
#include "options.h"
#include "ide.h"
#include "coff-output.h"
#include "algebra.h"
#include "tree-rewrite.h"
#include "expression-fields.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>
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
/* Native API-boundary units, not malformed-IR/compiler reach claims. Run
   unchanged input through real option parsing before invoking the explicit
   error/control API. Capture its actual nonreturning diagnostic and status in
   the unit child. Existing shared live-entry machinery supplies the anchor. */
extern void initialize_limits(void);
static unsigned error_scenario;
static void explicit_error_entry(void)
{
 Node *node;
 if(compiler_errors||!error_filename) _exit(97);
 switch(error_scenario){
 case 0:compiler_internal_error("unit-boundary.c",0x123);break;
 case 1:ide_error("ignored.c",999);break;
 case 2:node=new_node(0x33);node->attributes.words.type=0x204;lower_far_pointer_cast(node);break;
 case 3:node=new_node(0x33);node->attributes.words.type=0x204;lower_target_address(node);break;
 case 4:coff_error(7,"ignored.c",999);break;
 case 5:
  if(!object_filename||strcmp(object_filename,"validate.obj")||
   fopen("absent-diagnostic-directory/file","rb")||errno!=ENOENT)_exit(96);
  coff_error(14,"ignored.c",999);break;
 default:_exit(95);
 }
 _exit(99); /* each original boundary is actually nonreturning */
}
static int fatal_api(unsigned scenario,const char *source,const char *line)
{
 /* Pipe descriptors belong to native test glue, not the target CRT lowio
    table. Use host syscalls; target errors still use the ordinary CRT. */
 int descriptors[2],pid,status,n;char text[2048];unsigned used=0;
 error_scenario=scenario;if(pipe(descriptors))return 0;
 pid=fork();if(pid<0){syscall(SYS_close,descriptors[0]);syscall(SYS_close,descriptors[1]);return 0;}
 if(!pid){
  syscall(SYS_close,descriptors[0]);if(syscall(SYS_dup2,descriptors[1],2)<0)_exit(94);syscall(SYS_close,descriptors[1]);
  recon_native_at_entry((const void*)initialize_limits,(const void*)explicit_error_entry,real_input,0);
  _exit(98);
 }
 syscall(SYS_close,descriptors[1]);
 while(used<sizeof(text)-1){n=syscall(SYS_read,descriptors[0],text+used,sizeof(text)-1-used);
  if(n<0&&errno==EINTR)continue;if(n<=0)break;used+=n;}
 syscall(SYS_close,descriptors[0]);text[used]=0;
 if(waitpid(pid,&status,0)!=pid||!WIFEXITED(status)||
  WEXITSTATUS(status)!=(scenario==5?1:2)||!strstr(text,source)||!strstr(text,line))return 0;
 return strstr(text,scenario==5?"C1085":"C1001")!=0&&compiler_errors==0;
}
static int explicit_internal_error(void){return fatal_api(0,"unit-boundary.c","291");}
static int explicit_ide_error(void){return fatal_api(1,"@(#)ide.c:1.2","68");}
static int unsupported_cast_error(void){return fatal_api(2,"@(#)stub.c:1.23","59");}
static int unsupported_address_error(void){return fatal_api(3,"@(#)stub.c:1.23","66");}
static int explicit_coff_assertion(void){return fatal_api(4,"@(#)coff.c:1.14","1021");}
static int explicit_coff_write_error(void){return fatal_api(5,"validate.obj","compiler generated");}
const ReconTest generated_tests[]={
 {"actual warning-as-error counter",1,actual_error_count},
 {"actual failed listing source arguments",1,actual_context},
 {"actual diagnostic FILE survives descriptor closure",1,actual_close},
 {"unit-qualified internal error increments shared counter before fatal exit",1,explicit_internal_error},
 {"unit-qualified IDE error reports adapter implementation site",1,explicit_ide_error},
 {"unit-qualified unsupported target cast retains fatal boundary",1,unsupported_cast_error},
 {"unit-qualified unsupported target address retains fatal boundary",1,unsupported_address_error},
 {"unit-qualified COFF unsupported callback code retains assertion",1,explicit_coff_assertion},
 {"unit-qualified COFF write callback retains parsed filename and CRT error",1,explicit_coff_write_error}
};
