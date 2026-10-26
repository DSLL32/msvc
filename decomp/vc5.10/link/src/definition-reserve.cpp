/* Keep Grow as the genuine outlined call; its body is not visible here. */
#include <string.h>
#include "definition-buffer-set.h"
#include <recon.h>

template<class T> int RECON_THISCALL DefinitionStorage<T>::Reserve(int bytes,T **output)
{
    if(bytes<0)return 0;
    if((DWORD)end+(DWORD)bytes>(DWORD)data+(DWORD)capacity && !Grow(bytes))
        return 0;
    if(output)*output=end;
    return 1;
}
template<class T> int RECON_THISCALL DefinitionStorage<T>::Advance(int bytes,T **output)
{
    if(bytes<0)return 0;
    if((DWORD)end+(DWORD)bytes>(DWORD)data+(DWORD)capacity && !Grow(bytes))
        return 0;
    if(output)*output=end;
    Set(data,(T *)((DWORD)end+(DWORD)bytes));
    return 1;
}
template int RECON_THISCALL DefinitionStorage<BYTE>::Reserve(int,BYTE **);
template int RECON_THISCALL DefinitionStorage<BYTE>::Advance(int,BYTE **);

template<class T> int RECON_THISCALL DefinitionStorage<T>::Reserve(int bytes)
{
    if(bytes<0)return 0;
    if((DWORD)end+(DWORD)bytes>(DWORD)data+(DWORD)capacity && !Grow(bytes))
        return 0;
    return 1;
}
template int RECON_THISCALL DefinitionStorage<BYTE>::Reserve(int);

#pragma intrinsic(memcpy)
template<class T> int RECON_THISCALL DefinitionStorage<T>::Copy(const T *source,int bytes,T **output)
{
    T *destination;
    if(!source || bytes<0)return 0;
    if((DWORD)end+(DWORD)bytes>(DWORD)data+(DWORD)capacity && !Grow(bytes))
        return 0;
    destination=end;
    Set(data,(T *)((DWORD)end+(DWORD)bytes));
    if(output)*output=destination;
    memcpy(destination,source,bytes);
    return 1;
}
template int RECON_THISCALL DefinitionStorage<BYTE>::Copy(const BYTE *,int,BYTE **);
