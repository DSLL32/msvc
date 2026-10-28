#include <setjmp.h>
#include <recon.h>
#ifdef RECON_SHIM_IMPL
#ifdef setjmp
#error implementation remapped host save
#endif
#else
int profile_compile(void){jmp_buf b;return setjmp(b);}
#endif
