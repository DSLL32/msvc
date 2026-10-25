#ifndef RECON_TOOL_H
#define RECON_TOOL_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
#define RECON_ABI_VERSION 2u
#define RECON_EXPORT __attribute__((visibility("default")))
#define RECON_COUNT(a) ((uint32_t)(sizeof(a) / sizeof((a)[0])))
    /* All ranges are half-open original image RVAs. */
    typedef struct ReconExtent
    {
        uint32_t start, end;
    } ReconExtent;
    enum ReconDefinition
    {
        RECON_UNKNOWN,
        RECON_IMPORT_NAME,
        RECON_IMPORT_ORDINAL,
        RECON_CRT,
        RECON_APP
    };
    enum ReconImplementation
    {
        RECON_UNIMPLEMENTED,
        RECON_IMPLEMENTED
    };
    typedef struct ReconFunction
    {
        const char *name;
        uint32_t entry_rva;
        const ReconExtent *code;
        uint32_t code_count;
        const ReconExtent *text_data;
        uint32_t text_data_count;
        uint32_t definition;
        uint32_t implementation;   /* App only; omit/zero for Unknown/Import/CRT. */
        const char *native_symbol; /* ELF symbol, including static or mangled names. */
        const char *import_dll, *import_name;
        uint32_t import_ordinal;
        const char *original_symbol; /* Optional original-toolchain MAP symbol; name otherwise. */
    } ReconFunction;
    typedef struct ReconExport
    {
        const char *name;
        uint32_t ordinal, entry_rva;
    } ReconExport;
    typedef struct ReconBinary
    {
        uint32_t abi_version, struct_size;
        const char *name, *original_path, *original_sha256;
        uint32_t original_entry_rva;
        const ReconFunction *functions;
        uint32_t function_count;
        const ReconExport *exports;
        uint32_t export_count;
    } ReconBinary;
    enum ReconExecution
    {
        RECON_IO,
        RECON_DIRECT
    };
    enum ReconTerminal
    {
        RECON_EXIT,
        RECON_FRONTIER
    };
    /* Adapter setup precedes the fork checkpoint. It must not create threads or
     * children. Per-case state belongs to this executor, never the CLI parent. */
    typedef int (*ReconSetup)(void);
    typedef int (*ReconIO)(int argc, char **argv); /* EXE main or typed DLL adapter. */
    typedef int (*ReconCheck)(void);               /* nonzero = pass; zero = fail */
    typedef struct ReconTest
    {
        const char *name;
        uint32_t fork;
        ReconCheck exec;
    } ReconTest;
    typedef struct ReconCase
    {
        uint32_t abi_version, struct_size;
        const char *name, *description;
        const uint32_t *first_entry_order;
        uint32_t first_entry_count;
        const uint32_t *first_entry_seen;
        uint32_t first_entry_seen_count;
        uint32_t execution, terminal, frontier_rva;
        ReconSetup setup;
        ReconIO io;
        ReconCheck direct;
        const ReconTest *tests;
        uint32_t test_count;
    } ReconCase;
    extern RECON_EXPORT const ReconBinary binary_descriptor;
    extern RECON_EXPORT const ReconCase case_descriptor;
#ifdef __cplusplus
}
#endif
#endif
