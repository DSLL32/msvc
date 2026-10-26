#ifndef LINK_DEFINITION_BUFFER_CONSTRUCT_H
#define LINK_DEFINITION_BUFFER_CONSTRUCT_H
#include "definition-buffer.h"
#include <recon.h>

template<class T> RECON_FORCEINLINE DefinitionStorage<T>::DefinitionStorage()
{
    data=NULL; end=NULL; capacity=0;
    relocate=NULL; relocation_context=NULL;
}
#endif
