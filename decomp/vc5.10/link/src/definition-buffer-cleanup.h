#ifndef LINK_DEFINITION_BUFFER_CLEANUP_H
#define LINK_DEFINITION_BUFFER_CLEANUP_H
#include <new.h>
#include "definition-buffer.h"
#include <recon.h>

template<class T> RECON_RETAIN inline DefinitionStorage<T>::~DefinitionStorage()
{
    if (data) {
        operator delete(data);
        Set(NULL,NULL);
        capacity=0;
    }
}
#endif
