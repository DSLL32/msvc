#ifndef LINK_DEFINITION_BUFFER_APPEND_H
#define LINK_DEFINITION_BUFFER_APPEND_H
#include <string.h>
#include "definition-buffer.h"
#include <recon.h>

/* These operations are wholly inlined into the definition-file owner. Their
 * definitions are deliberately absent from the canonical instantiation TU. */
#pragma intrinsic(strlen,memcpy)
template<class T> RECON_FORCEINLINE void DefinitionStorage<T>::Write(const void *source,int bytes)
{
    T *destination;
    if(Reserve(bytes,&destination)) {
        Set(data,end+bytes);
        memcpy(destination,source,bytes);
    }
}
template<class T> RECON_FORCEINLINE void DefinitionStorage<T>::Append(const void *source,int bytes)
{
    T *destination;
    if(Advance(bytes,&destination)) memcpy(destination,source,bytes);
}
template<class T> RECON_FORCEINLINE void DefinitionStorage<T>::AppendString(const char *source)
{
    /* The original measures the string before its subsequent pointer check;
     * this is not a supported NULL-string operation. */
    unsigned bytes=strlen(source);
    if(source) Append(source,bytes);
}
#endif
