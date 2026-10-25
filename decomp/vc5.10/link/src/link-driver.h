#ifndef LINK_DRIVER_H
#define LINK_DRIVER_H
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include "symbols.h"
#include "full-link.h"
#include <recon.h>

/* The context is a PE header plus linker options. Unread fields retain their
 * discovered extent until their users are reconstructed. */
typedef struct LinkOptions {
    BYTE output_name_set, image_base_set, heap_set, stack_set;
    DWORD header_size;
    BYTE checksum_enabled, debug_import_library, bind_delay_imports, unload_delay_imports;
    BYTE retain_relocations, edit_and_continue, fixed_image, mppc32_types;
    BYTE fold_identical_comdats, legacy_import_library, mac_bundle, write_map;
    BYTE map_lines, map_fixups, map_exports, reserved13b;
    BYTE misc_rdata, new_glue, new_relocations, no_default_libraries;
    BYTE no_entry, no_pack, option142, order_file;
    BYTE default144, profile, rom, order;
    BYTE effective_reference_opt, reference_opt_specified;
    BYTE windowsce_emulation, windowsce_noconvert;
    DWORD gp_size, debug_level, debug_formats, force;
    const char *mac_creator, *mac_type;
    DWORD reserved164;
    const char *default_extension;
    DWORD reserved16c[5];
    BYTE default180, reserved181[15];
} LinkOptions;

typedef struct LinkContext LinkContext;
typedef struct LinkAuxiliaryData {
    DWORD reserved[11];
#ifdef __cplusplus
    void RECON_THISCALL PrepareOutput(void);
    void RECON_THISCALL FinishOutput(LinkContext *context);
#endif
} LinkAuxiliaryData;
typedef struct LinkPointerList {
    void *first;
    void **last;
    BYTE active, reserved[3];
    DWORD count;
} LinkPointerList;
/* A contextual output-file payload, never executed by the reconstruction. */
typedef struct DefaultDosImage {
    IMAGE_DOS_HEADER header;
    BYTE bootstrap[14];
    char message[50];
} DefaultDosImage;
#ifdef __cplusplus
extern "C" {
#endif
extern const DefaultDosImage default_dos_image;
#ifdef __cplusplus
}
#endif

/* Genuine target thiscall: ECX carries this, and one stack DWORD is popped.
 * VC5 C cannot declare it. This POD has no implicit construction, destruction,
 * virtual dispatch, RTTI or exception machinery. Native calls also explicitly
 * use thiscall, rather than the Linux default C++ member convention. */
struct LinkContext {
    char database_signature[32];
    WORD database_type, database_flags;
    LinkContext *self;
    DWORD reserved028[2];
    IMAGE_FILE_HEADER file_header;
    IMAGE_OPTIONAL_HEADER optional_header;
    LinkOptions options;
    DWORD specified_switches, comment_size;
    const char *entry_name;
    const unsigned char *mac_initializer_name, *mac_terminator_name;
    struct IncludeSymbol *includes;
    DWORD reserved1a8[3];
    SectionList sections;
    LinkPointerList pointer_list;
    OutputLibrary *command_line_library;
    StringTable *symbols, *strings;
    const DefaultDosImage *dos_image;
    DWORD dos_image_size, pe_header_offset;
    int is_vxd;
    DWORD reserved1e8;
    BYTE dynamic_vxd, reserved1ed[83];
    DWORD pdb_module_count, reserved244;
    void *pdb_modules;
    DWORD reserved24c[2], mac_module_count, reserved258;
    void *mac_modules;
    LinkAuxiliaryData *auxiliary;
    BYTE reserved264[36];
    Contribution *incremental_contribution;
    BYTE ignore_directives, reserved28d[3];
    DWORD reserved290;
    MacResource *resources;

#ifdef __cplusplus
    LinkAuxiliaryData *RECON_THISCALL Initialize(int vxd);
    void RECON_THISCALL InitializeVxd(void);
    DWORD RECON_THISCALL PeHeaderSize(DWORD *offset);
    DWORD RECON_THISCALL VxdHeaderSize(DWORD *offset);
    void RECON_THISCALL PeWriteSection(int file,void *section,void *header);
    void RECON_THISCALL VxdWriteSection(int file,void *section,void *header);
    void RECON_THISCALL PeWriteHeaders(int file);
    void RECON_THISCALL VxdWriteHeaders(int file);
    int RECON_THISCALL FullLink(BYTE *pack_requested);
    DWORD RECON_THISCALL RangeCodeBytes(DWORD start,DWORD limit,Contribution **last);
    void RECON_THISCALL FoldIdenticalComdats(void);
    void RECON_THISCALL PrintFoldedComdats(void);
    Contribution * RECON_THISCALL CreateIncrementalContribution(void);
    void RECON_THISCALL InitializeSections(void);
    DWORD RECON_THISCALL BuildDosExtension(void **buffer);
    void RECON_THISCALL ApplyMerges(void);
    void RECON_THISCALL PrepareMacDebugSections(void);
    void RECON_THISCALL PrepareMacRuntimeSections(void);
    void RECON_THISCALL PrepareNativeRelocations(void);
    BYTE RECON_THISCALL CanMergeReadOnlyData(void);
    void RECON_THISCALL MergeExceptionSections(void);
    void RECON_THISCALL ResolveEntryPoint(void);
    WORD RECON_THISCALL CountOutputSections(void);
    void RECON_THISCALL PrepareSectionLayout(void);
    void RECON_THISCALL LayoutSections(DWORD *rva,DWORD *file_offset,DWORD *line_count);
    void RECON_THISCALL WriteSections(DWORD *line_offset);
    void RECON_THISCALL ApplySectionAttributes(void);
    void RECON_THISCALL ResolveLocalImports(Contribution **contribution,IncludeSymbol **aliases);
    void RECON_THISCALL ResolveUndefined(void);
    void RECON_THISCALL CollectRelocationSymbols(DWORD *first_count,void **first,DWORD *second_count,void **second);
    void RECON_THISCALL FinalizeOrder(void);
    void RECON_THISCALL FinishNativeSections(void);
    void RECON_THISCALL WriteRelocationSymbols(DWORD first_count,void **first,DWORD second_count,void **second);
    void RECON_THISCALL WriteLocalImports(Contribution *contribution,IncludeSymbol **aliases);
    void RECON_THISCALL EmitDebugDirectories(DWORD start,DWORD original_offset,DWORD lines,DWORD end,DWORD line_offset);
    void RECON_THISCALL FinishImage(void);
    void RECON_THISCALL PassOne(void);
    void RECON_THISCALL LoadInputArgument(InputArgument *argument,OutputLibrary *library);
    void RECON_THISCALL PassTwo(void);
    void RECON_THISCALL PrepareMacJumpTables(void);
    void RECON_THISCALL CheckMachine(const char *name,WORD machine);
    void RECON_THISCALL ResolveMacImports(void);
    void RECON_THISCALL ResolveMacExports(void);
    void RECON_THISCALL PrepareMacResources(void);
    void RECON_THISCALL PrepareMacSections(void);
    void RECON_THISCALL PrepareMacSymbols(void);
    void RECON_THISCALL PrepareAuxiliarySections(void);
    void RECON_THISCALL UpdateChecksum(void);
    void RECON_THISCALL WriteVxdModuleTable(int file);
    void RECON_THISCALL WriteVxdNames(int file);
#endif
};
#ifdef __cplusplus
typedef DWORD (RECON_THISCALL LinkContext::*HeaderSizeHandler)(DWORD *);
typedef void (RECON_THISCALL LinkContext::*SectionWriter)(int,void *,void *);
typedef void (RECON_THISCALL LinkContext::*HeaderWriter)(int);
extern HeaderSizeHandler image_header_size;
extern SectionWriter image_write_section;
extern HeaderWriter image_write_headers;
#endif
typedef char DefaultDosImageSize[(sizeof(DefaultDosImage)==128)?1:-1];
typedef char LinkAuxiliaryDataSize[(sizeof(LinkAuxiliaryData)==44)?1:-1];
typedef char LinkContextSymbolsAt1d0[(offsetof(LinkContext,symbols)==0x1d0)?1:-1];
typedef char LinkContextPdbModulesAt248[(offsetof(LinkContext,pdb_modules)==0x248)?1:-1];
typedef char LinkContextMacModulesAt25c[(offsetof(LinkContext,mac_modules)==0x25c)?1:-1];
typedef char LinkContextAuxiliaryAt260[(offsetof(LinkContext,auxiliary)==0x260)?1:-1];
typedef char LinkContextSectionsAt1b4[(offsetof(LinkContext,sections)==0x1b4)?1:-1];
typedef char LinkContextIncrementalAt288[(offsetof(LinkContext,incremental_contribution)==0x288)?1:-1];
typedef char LinkContextIgnoreDirectivesAt28c[(offsetof(LinkContext,ignore_directives)==0x28c)?1:-1];
typedef char LinkContextResourcesAt294[(offsetof(LinkContext,resources)==0x294)?1:-1];
typedef char LinkOptionsExtensionAt44[(offsetof(LinkOptions,default_extension)==0x44)?1:-1];
typedef char LinkOptionsSize[(sizeof(LinkOptions)==108)?1:-1];
typedef char LinkOptionsForceAt34[(offsetof(LinkOptions,force)==0x34)?1:-1];
typedef char LinkContextEntryAt198[(offsetof(LinkContext,entry_name)==0x198)?1:-1];
typedef char LinkContextIncludesAt1a4[(offsetof(LinkContext,includes)==0x1a4)?1:-1];
typedef char LinkContextDynamicAt1ec[(offsetof(LinkContext,dynamic_vxd)==0x1ec)?1:-1];
typedef char LinkContextSize[(sizeof(LinkContext)==664)?1:-1];
typedef char LinkContextOptionalAt44[(offsetof(LinkContext,optional_header)==0x44)?1:-1];
typedef char LinkContextOptionsAt124[(offsetof(LinkContext,options)==0x124)?1:-1];
typedef char LinkContextVxdAt1e4[(offsetof(LinkContext,is_vxd)==0x1e4)?1:-1];
#endif
