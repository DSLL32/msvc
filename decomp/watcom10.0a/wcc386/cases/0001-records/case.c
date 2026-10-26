#include <recon-tool.h>
#include "startup.h"
static const uint32_t order[]={0x47c77,0x47f15,0x66a2e,0x47b89,0x47bb1,0x47e52,0x47e2b,0x47d3a,0x1a99b,0x1a9c5,0x1183,0x11f42,0x120d8,0x14a2e,0x1a357,0x1115,0x2b9a9,0x13bd,0x2032,0x1600c,0x18fd9,0x19c36,0x19cd0,0x19cdd,0xe6a0,0x1aaf9,0x1211f,0x11fe3,0x11f43,0x1aa87,0x1ab5d,0x1ab4f,0x1abc8,0x14be4,0x14b58,0x1ab8e,0x1201f,0x18f61,0x17cea,0x1ac5d,0x194b2,0x194b7,0x1a3fd,0x5cf6,0x17e9e,0x18195,0x19088,0x183cd,0x18204,0x17e57,0x1813a,0x18379,0x177fb,0x2b559,0x10,0x47,0x4b8f,0x2b7cb,0x12ae2,0x2b76f,0x2b8b0,0x8a,0x6541,0xe628,0x14a0,0x2a3f6};
/* Empty CPragmaInit shares CTypeInit's terminal C3. Exclude only this
 * statically proved RET0 entry on both sides; no matched frontier is claimed. */
/* CSkip likewise shares ChkControl's RET: static empty-body proof only,
 * zero additional unique instruction bytes, no observed CSkip frontier. */
/* ChkPragmas shares CPragma's sole C3. Static empty-body/RET0 proof,
 * zero additional unique bytes; no observed ChkPragmas frontier/order claim. */
/* MemCoalesce is a genuine empty hook sharing MemFree's terminal C3.
 * Exclude this static RET0 identity; no observed frontier/order claim. */
/* EmitAbort's sole C3 is physically shared with EmitCS_Strings. Its empty
 * body/ABI are static facts; exclude only this entry, with no observed order. */
static const uint32_t seen[]={0x19087,0xfb36,0x13f71,0x47e02,0xd97b};
RECON_EXPORT const ReconCase case_descriptor={
    .abi_version=RECON_ABI_VERSION,
    .struct_size=sizeof(ReconCase),
    .name="wcc-records-compilation",
    .description="C89 packed records: preprocessing, declarations, aggregate copies, function pointers, varargs, loops, integers and floating point",
    .first_entry_order=order,
    .first_entry_count=RECON_COUNT(order),
    .first_entry_seen=seen,
    .first_entry_seen_count=RECON_COUNT(seen),
    .execution=RECON_IO,
    .terminal=RECON_FRONTIER,
    .frontier_rva=0x2a3f6,
    .io=wcc_native_startup,
};
