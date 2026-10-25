#ifndef RECON_NATIVE_NEW_H
#define RECON_NATIVE_NEW_H
#include <stddef.h>
#include <recon-inline.h>
#ifdef __cplusplus
/* Allocating form binds the existing shared boundary, with no wrapper vertex. */
void *__attribute__((cdecl)) operator new(size_t) __asm__("recon_cpp_new");
void __attribute__((cdecl)) operator delete(void *) __asm__("recon_cpp_delete");
/* Period NEW.H placement form: storage is already owned by the caller. */
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
RECON_FORCEINLINE void *__attribute__((cdecl)) operator new(size_t,void *pointer)
{ return pointer; }
#endif
#endif
#endif
