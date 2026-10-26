#include <new.h>
#include <string.h>
#include "definition-buffer.h"
#include <recon.h>

#pragma intrinsic(memcpy,memset)
template<class T> int RECON_THISCALL DefinitionStorage<T>::Grow(int bytes)
{
    int growth=capacity/2;
    int minimum=(int)((DWORD)bytes+4096);
    if(minimum>growth) growth=minimum;
    int size=(int)(((DWORD)capacity+(DWORD)growth+4095)&0xfffff000);
    T *replacement=(T *)operator new(size);
    if(!replacement) return 0;
    unsigned used=(DWORD)end-(DWORD)data;
    capacity=size;
    memcpy(replacement,data,used);
    memset((BYTE *)replacement+used,0,size-used);
    operator delete(data);
    Set(replacement,(T *)((BYTE *)replacement+used));
    return 1;
}
template<class T> int RECON_THISCALL DefinitionStorage<T>::Set(T *base,T *limit)
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
template<class T> int RECON_THISCALL DefinitionStorage<T>::Reserve(int bytes,T **output)
{ __asm { int 3 } return 0; }
template<class T> int RECON_THISCALL DefinitionStorage<T>::Advance(int bytes,T **output)
{ __asm { int 3 } return 0; }
template<class T> int RECON_THISCALL DefinitionStorage<T>::Copy(const T *source,int bytes,T **output)
{ __asm { int 3 } return 0; }
/* Set is visible here: the original /Ob2 compiler removes Set(NULL,NULL)
 * from the canonical cleanup outline. Callers share the same inline body. */
#include "definition-buffer-cleanup.h"
/* Explicit instantiation emits only the defined real canonical outlines.
 * The constructor and append operations remain wholly inline. This is a
 * source representation, not recovered RTTI. */
template class DefinitionStorage<BYTE>;
