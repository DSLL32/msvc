#include <recon.h>
static ULONG WINAPI handler(EXCEPTION_RECORD *,void *,CONTEXT *,void *) {return 1;}
static ReconBorland400UserHandler (__cdecl *setter)(ReconBorland400UserHandler)=recon_borland_400_SetUserHandler;
#ifdef RECON_BORLAND_400_USER_HANDLER
static ReconBorland400UserHandler (__cdecl *mapped)(ReconBorland400UserHandler)=_SetUserHandler;
#else
#ifdef _SetUserHandler
#error unselected alias leaked
#endif
#endif
void typing_check(void) {(void)setter;(void)&handler;
#ifdef RECON_BORLAND_400_USER_HANDLER
(void)mapped;
#endif
}
