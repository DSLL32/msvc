#include <setjmp.h>
#include <recon.h>
static int (__cdecl *save)(jmp_buf)=setjmp;
static void (__cdecl *jump)(jmp_buf,int)=longjmp;
typedef char Size[sizeof(jmp_buf)==32?1:-1];
struct Layout{char prefix[3];jmp_buf owner;};
typedef char Alignment[offsetof(Layout,owner)==3?1:-1];
