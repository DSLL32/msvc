#define RECON_SHIM_IMPL
#include <assert.h>
#include "storage.h"
#include <recon.h>
typedef char SizeCheck[sizeof(jmp_buf)==32?1:-1];
typedef char AlignmentCheck[offsetof(struct State,owner)==3?1:-1];
int main(int argc,char **argv)
{
 unsigned i;int result;unsigned short cw,after;long double live;
 memset(&state,0xa5,sizeof(state));
 if(argc>1){
  if(!strcmp(argv[1],"address-null"))return save_address()(NULL);
  if(!strcmp(argv[1],"address-owner"))return save_address()(state.owner);
  if(!strcmp(argv[1],"jump-null"))jump_address()(NULL,1);
  if(!strcmp(argv[1],"size")){recon_borland_400_setjmp_context(state.owner,31);return 90;}
  if(!strcmp(argv[1],"null")){recon_borland_400_setjmp_context(NULL,32);return 90;}
  if(!strcmp(argv[1],"wrap")){recon_borland_400_setjmp_context((void*)(uintptr_t)0xfffffff0,32);return 90;}
 }
 errno=137;recon_doserrno=138;recon_SetLastError(139);
 __asm__ volatile("fnstcw %0;fld1":"=m"(cw));result=initial_save();
 __asm__ volatile("fstpt %0;fnstcw %1":"=m"(live),"=m"(after));
 assert(!result && errno==137 && recon_doserrno==138 && recon_GetLastError()==139 && live==1.0L && cw==after);
 if(argc>1){
  if(!strcmp(argv[1],"generic"))recon_longjmp(state.owner,1);
  if(!strcmp(argv[1],"jump"))selected_jump(1);
  if(!strcmp(argv[1],"jump-zero"))selected_jump(0);
 }
 for(i=0;i<100;++i)assert(!initial_save());
 assert(!evaluated_save() && evaluations==1);
 /* Opaque target storage is not a forged/serialized native register image. */
 for(i=0;i<sizeof(state);++i)assert(((unsigned char*)&state)[i]==0xa5);
 assert(errno==137 && recon_doserrno==138 && recon_GetLastError()==139);
 puts("Borland initial-only setjmp: actual caller save,32Bopaque storage,errors/FP,reuse/once-evaluation and cold forms PASS");return 0;
}
