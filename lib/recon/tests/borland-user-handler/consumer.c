#include <recon.h>
#ifdef RECON_BORLAND_400_USER_HANDLER
#ifndef _SetUserHandler
#error selected Borland setter alias missing
#endif
ReconBorland400UserHandler consumer_set(ReconBorland400UserHandler handler)
{ return _SetUserHandler(handler); }
#else
#ifdef _SetUserHandler
#error unselected Borland setter alias
#endif
#endif
