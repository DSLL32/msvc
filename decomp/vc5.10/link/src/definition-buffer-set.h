#ifndef LINK_DEFINITION_BUFFER_SET_H
#define LINK_DEFINITION_BUFFER_SET_H
#include "definition-buffer.h"
#include <recon.h>

/* Buffer internals see this inline definition; outside callers retain the
 * declaration so genuine original Set calls remain outlined. */
template<class T> inline int RECON_THISCALL DefinitionStorage<T>::Set(T *base,T *limit)
{
    T *old;
    if (!base) return 0;
    old=data;
    data=base;
    end=limit;
    if (old!=base && relocate)
        relocate(relocation_context,old,base);
    return 1;
}
#endif
