#ifndef RECON_SHIMS_H
#define RECON_SHIMS_H
#include <trace-abi.h>
#include <recon-inline.h>
/* VC5 expresses this convention implicitly on C++ members. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define RECON_THISCALL __attribute__((thiscall))
/* Typed private MSPDB static import; declaration binding only, no wrapper. */
#define RECON_MSPDB_NAMEMAP_OPEN __asm__("recon_mspdb_NameMap_open")
/* Use only for an independently proved 8-byte EDX:EAX return ABI. The
 * return object is addressable; sizeof checks never read/initialize its fields. */
#define RECON_RETURN8_TYPE(type) uint64_t
#define RECON_RETURN8(value) do { \
    uint64_t _recon_return8_bits; \
    typedef char _recon_return8_size[(sizeof(value)==8)?1:-1] __attribute__((unused)); \
    __builtin_memcpy(&_recon_return8_bits,&(value),8); \
    return _recon_return8_bits; \
} while (0)
#define RECON_ASSIGN_RETURN8(destination,call) do { \
    uint64_t _recon_return8_bits=(call); \
    typedef char _recon_return8_size[(sizeof(destination)==8)?1:-1] __attribute__((unused)); \
    __builtin_memcpy(&(destination),&_recon_return8_bits,8); \
} while (0)
#else
#define RECON_THISCALL
#define RECON_MSPDB_NAMEMAP_OPEN
#define RECON_RETURN8_TYPE(type) type
#define RECON_RETURN8(value) return (value)
#define RECON_ASSIGN_RETURN8(destination,call) ((destination)=(call))
#endif
/* Native remaps are disabled in canonical Win32. Include last. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#include <recon-remaps.h>
#endif /* Linux native test build */
#endif
