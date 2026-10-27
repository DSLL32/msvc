#ifndef LINK_DRIVER_H
#define LINK_DRIVER_H
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include "symbols.h"
#include "full-link.h"
#include <recon.h>

struct SectionNumberReference;

/* The context is a PE header plus linker options. Unread fields retain their
 * discovered extent until their users are reconstructed. */
typedef struct LinkOptions {
    BYTE output_name_set, image_base_set, heap_set, stack_set;
    DWORD header_size;
    BYTE checksum_enabled, debug_import_library, bind_delay_imports, unload_delay_imports;
    BYTE retain_relocations, edit_and_continue, fixed_image, mppc32_types;
    BYTE fold_identical_comdats, legacy_import_library, mac_bundle, write_map;
    BYTE map_lines, map_fixups, map_exports, mips_r41xx;
    BYTE misc_rdata, new_glue, new_relocations, no_default_libraries;
    BYTE no_entry, no_pack, option142, order_file;
    BYTE default144, profile, rom, order;
    BYTE effective_reference_opt, reference_opt_specified;
    BYTE windowsce_emulation, windowsce_noconvert;
    DWORD gp_size, debug_level, debug_formats, force;
    const char *mac_creator, *mac_type;
    const char *library_basename;
    const char *default_extension;
    BYTE librarian_list,librarian_skip_directive_symbols,librarian_convert,reserved16f;
    WORD dump_linker_members,dump_raw_count;
    DWORD dump_raw_format;
    /* At the same option byte, the editor tests PE signatures; the dumper
     * selects header display. These are separate per-tool context instances. */
    union { BYTE dump_headers; BYTE editor_require_pe; };
    BYTE dump_relocations,dump_line_numbers,dump_symbols;
    BYTE dump_base_relocations,dump_imports,dump_exports,dump_raw_data;
    union { BYTE default180; BYTE dump_summary; };
    BYTE dump_architecture,dump_archive_members,dump_convert,dump_fpo;
    BYTE dump_pdata,dump_omap_to,dump_omap_from,dump_fixups,dump_map;
    BYTE reserved18a,dump_dependents,dump_disassembly,dump_directives;
    BYTE reserved18e[2];
} LinkOptions;
typedef char DumperOptionOffsets[(offsetof(LinkOptions,dump_raw_format)==0x50 &&
    offsetof(LinkOptions,dump_summary)==0x5c)?1:-1];

typedef struct LinkContext LinkContext;
typedef struct LinkAuxiliaryData {
    DWORD procedure_count,emitted_procedure_count;
    BYTE *procedure_data;
    DWORD procedure_capacity,procedure_contribution_count;
    void *contribution_records;
    DWORD auxiliary_capacity,reserved1c;
    void *auxiliary_records;
    DWORD procedure_padding,procedure_record_size;
#ifdef __cplusplus
    BYTE RECON_THISCALL RecordFixup(DWORD,WORD,Contribution *,DWORD,SHORT);
    void RECON_THISCALL SetProcedureSize(WORD machine,DWORD bytes);
    BYTE RECON_THISCALL PrepareOutput(void);
    BYTE RECON_THISCALL WriteProcedures(WORD,void *,Contribution *);
    void RECON_THISCALL FinishOutput(LinkContext *context);
#endif
} LinkAuxiliaryData;
typedef struct DefaultLibrary {
    struct DefaultLibrary *next;
    const char *name;
    DWORD flags;
    void *owner;
} DefaultLibrary;
typedef char DefaultLibrarySize[(sizeof(DefaultLibrary)==16)?1:-1];
typedef struct LinkPointerList {
    void *first;
    void **last;
    BYTE suppress_default_libraries, reserved[3];
    DefaultLibrary *default_libraries;
} LinkPointerList;
typedef char DefaultLibraryListFields[(sizeof(LinkPointerList)==16 &&
    offsetof(LinkPointerList,suppress_default_libraries)==8 &&
    offsetof(LinkPointerList,default_libraries)==12)?1:-1];
/* Incremental definition/export state; fields are named only where the
 * definition-file producer or its incremental collector consumes them. */
typedef struct IncrementalDefinition {
    char *definition_name;
    DWORD reserved04;
    char *import_library_name;
    DWORD import_library_time;
    OutputModule *export_module;
    InputArguments exports;
} IncrementalDefinition;
typedef char IncrementalDefinitionSize[(sizeof(IncrementalDefinition)==32)?1:-1];
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
    BYTE dynamic_vxd, reserved1ed[3];
    DWORD vxd_header_bytes, reserved1f4, vxd_page_map_cursor;
    DWORD vxd_page_count, vxd_fixup_pages, vxd_fixup_records, vxd_first_data;
    DWORD vxd_resident_names,vxd_entry_table;
    IncrementalDefinition definition;
    DWORD pdb_signature,pdb_age,pdb_fpo_count;
    DWORD pdb_module_count, reserved244;
    void *pdb_modules;
    DWORD fpo_directory_offset,mac_procedure_count, mac_module_count, reserved258;
    void *mac_modules;
    LinkAuxiliaryData *auxiliary;
    BYTE reserved264[24];
    void *incremental_symbols;
    BYTE reserved280[8];
    Contribution *incremental_contribution;
    BYTE ignore_directives, reserved28d[3];
    DWORD mac_glue_count; /* Actual+290, counted generated glue entries. */
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
    BYTE RECON_THISCALL MergeNamedSections(const char *source,const char *destination);
    void RECON_THISCALL PrepareMacDebugSections(void);
    void RECON_THISCALL PrepareMacRuntimeSections(void);
    void RECON_THISCALL PrepareNativeRelocations(void);
    BYTE RECON_THISCALL CanMergeReadOnlyData(void);
    void RECON_THISCALL MergeExceptionSections(void);
    BYTE RECON_THISCALL CanMergeImportSections(void);
    void RECON_THISCALL ResolveEntryPoint(void);
    WORD RECON_THISCALL CountOutputSections(void);
    void RECON_THISCALL PrepareSectionLayout(void);
    void RECON_THISCALL LayoutSections(DWORD *rva,DWORD *file_offset,DWORD *line_count);
    void RECON_THISCALL WriteSections(DWORD *line_offset);
    void RECON_THISCALL ApplySectionAttributes(void);
    void RECON_THISCALL ResolveLocalImports(Contribution **contribution,IncludeSymbol **aliases);
    void RECON_THISCALL ResolveUndefined(void);
    void RECON_THISCALL CollectRelocationSymbols(Contribution **section_numbers,
        SectionNumberReference **sections,Contribution **image_base,IncludeSymbol **base_symbol);
    void RECON_THISCALL FinalizeOrder(void);
    void RECON_THISCALL FinishNativeSections(void);
    void RECON_THISCALL WriteBaseRelocations(void);
    void RECON_THISCALL WriteVxdRelocations(void);
    void RECON_THISCALL WriteRelocationSymbols(Contribution *section_numbers,
        SectionNumberReference **sections,Contribution *image_base,IncludeSymbol **base_symbol);
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
    void RECON_THISCALL ApplyOrderFile(void);
    void RECON_THISCALL PrintOrderFile(void);
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
typedef char LinkAuxiliaryOutputFields[(offsetof(LinkAuxiliaryData,procedure_data)==8 &&
    offsetof(LinkAuxiliaryData,procedure_capacity)==12 &&
    offsetof(LinkAuxiliaryData,contribution_records)==20 &&
    offsetof(LinkAuxiliaryData,auxiliary_capacity)==24 &&
    offsetof(LinkAuxiliaryData,auxiliary_records)==32 &&
    offsetof(LinkAuxiliaryData,procedure_record_size)==40)?1:-1];
typedef char LinkContextSymbolsAt1d0[(offsetof(LinkContext,symbols)==0x1d0)?1:-1];
typedef char LinkContextPdbModulesAt248[(offsetof(LinkContext,pdb_modules)==0x248)?1:-1];
typedef char LinkContextMacGlueCountAt290[(offsetof(LinkContext,mac_glue_count)==0x290)?1:-1];
typedef char LinkContextMacModulesAt25c[(offsetof(LinkContext,mac_modules)==0x25c)?1:-1];
typedef char LinkContextAuxiliaryAt260[(offsetof(LinkContext,auxiliary)==0x260)?1:-1];
typedef char LinkContextSectionsAt1b4[(offsetof(LinkContext,sections)==0x1b4)?1:-1];
typedef char LinkContextIncrementalAt288[(offsetof(LinkContext,incremental_contribution)==0x288)?1:-1];
typedef char LinkContextIgnoreDirectivesAt28c[(offsetof(LinkContext,ignore_directives)==0x28c)?1:-1];
typedef char LinkContextResourcesAt294[(offsetof(LinkContext,resources)==0x294)?1:-1];
typedef char LinkOptionsExtensionAt44[(offsetof(LinkOptions,default_extension)==0x44)?1:-1];
typedef char LinkOptionsSize[(sizeof(LinkOptions)==108)?1:-1];
typedef char LinkOptionsForceAt34[(offsetof(LinkOptions,force)==0x34)?1:-1];
typedef char LinkContextDefinitionAt214[(offsetof(LinkContext,definition)==0x214)?1:-1];
typedef char LinkContextEntryAt198[(offsetof(LinkContext,entry_name)==0x198)?1:-1];
typedef char LinkContextIncludesAt1a4[(offsetof(LinkContext,includes)==0x1a4)?1:-1];
typedef char LinkContextDynamicAt1ec[(offsetof(LinkContext,dynamic_vxd)==0x1ec)?1:-1];
typedef char LinkContextVxdPageOwners[(offsetof(LinkContext,vxd_page_count)==0x1fc &&
    offsetof(LinkContext,vxd_first_data)==0x208)?1:-1];
typedef char LinkContextVxdHeaderAt1f0[(offsetof(LinkContext,vxd_header_bytes)==0x1f0)?1:-1];
typedef char LinkContextVxdLimitAt1f8[(offsetof(LinkContext,vxd_page_map_cursor)==0x1f8)?1:-1];
typedef char LinkContextSize[(sizeof(LinkContext)==664)?1:-1];
typedef char LinkContextOptionalAt44[(offsetof(LinkContext,optional_header)==0x44)?1:-1];
typedef char LinkContextOptionsAt124[(offsetof(LinkContext,options)==0x124)?1:-1];
typedef char LinkContextVxdAt1e4[(offsetof(LinkContext,is_vxd)==0x1e4)?1:-1];
typedef char LibrarianDirectiveFlagAt16d[(offsetof(LinkContext,options)+
    offsetof(LinkOptions,librarian_skip_directive_symbols)==0x16d)?1:-1];
typedef char IncrementalSymbolContextAt27c[(offsetof(LinkContext,incremental_symbols)==0x27c)?1:-1];
#endif
