#ifndef LINK_DEFINITION_BUFFER_H
#define LINK_DEFINITION_BUFFER_H
#include <stddef.h>
#include <windows.h>
#include <recon.h>

/* Encountered 20-byte growable buffer. The inline lifetime definition is
 * shared by its owner and the canonical TU, where Set is visible to inlining. */
template<class T> class DefinitionStorage {
public:
    T *data,*end;
    int capacity;
    void (__stdcall *relocate)(void *,void *,void *);
    void *relocation_context;
    RECON_THISCALL DefinitionStorage();
    int RECON_THISCALL Grow(int bytes);
    int RECON_THISCALL Set(T *base,T *end);
    int RECON_THISCALL Reserve(int bytes,T **output);
    int RECON_THISCALL Reserve(int bytes);
    int RECON_THISCALL Advance(int bytes,T **output);
    int RECON_THISCALL Copy(const T *source,int bytes,T **output);
    void Write(const void *source,int bytes);
    void Append(const void *source,int bytes);
    void AppendString(const char *source);
    RECON_THISCALL ~DefinitionStorage();
};
typedef DefinitionStorage<BYTE> DefinitionBuffer;
typedef char DefinitionBufferSize[(sizeof(DefinitionBuffer)==20)?1:-1];
typedef char DefinitionBufferFields[(offsetof(DefinitionBuffer,end)==4 &&
    offsetof(DefinitionBuffer,capacity)==8 && offsetof(DefinitionBuffer,relocate)==12 &&
    offsetof(DefinitionBuffer,relocation_context)==16)?1:-1];
#endif
