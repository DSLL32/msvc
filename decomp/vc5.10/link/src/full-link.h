#ifndef LINK_FULL_LINK_H
#define LINK_FULL_LINK_H
#include <windows.h>
#include <stddef.h>
#include "arguments.h"
#include "symbols.h"
#include <recon.h>

struct LinkContext;
typedef struct OutputModule OutputModule;
typedef struct OutputLibrary OutputLibrary;
typedef struct OutputSection OutputSection;
typedef struct ContributionGroup ContributionGroup;
typedef struct Contribution Contribution;
typedef struct ComdatRoot ComdatRoot;
typedef struct ModuleListNode {
    OutputModule *module;
    struct ModuleListNode *next;
} ModuleListNode;
typedef char ModuleListNodeSize[(sizeof(ModuleListNode)==8)?1:-1];
typedef struct MapLineRecord {
    WORD line;
    WORD reserved;
    OutputSection *section;
    DWORD offset;
} MapLineRecord;
typedef char MapLineRecordLayout[(sizeof(MapLineRecord)==12 &&
    offsetof(MapLineRecord,section)==4 && offsetof(MapLineRecord,offset)==8)?1:-1];
typedef struct MapLineFile {
    const char *name;
    ByteBuffer lines;
    struct MapLineFile *next;
} MapLineFile;
typedef char MapLineFileLayout[(sizeof(MapLineFile)==20 &&
    offsetof(MapLineFile,lines)==4 && offsetof(MapLineFile,next)==16)?1:-1];
typedef struct CommonSymbol {
    StringNode *symbol;
    struct CommonSymbol *next;
} CommonSymbol;
typedef char CommonSymbolSize[(sizeof(CommonSymbol)==8)?1:-1];
/* The producer reserves twelve bytes per internal base-relocation record.
 * These are not the public eight-byte IMAGE_BASE_RELOCATION block headers. */
typedef struct LinkBaseRelocation {
    WORD type;
    SHORT section;
    DWORD address,value;
} LinkBaseRelocation;
typedef char LinkBaseRelocationSize[(sizeof(LinkBaseRelocation)==12)?1:-1];

typedef struct SectionFileMap {
    DWORD virtual_address,line_number_offset;
} SectionFileMap;
typedef char SectionFileMapSize[(sizeof(SectionFileMap)==8)?1:-1];
/* Internal per-contribution records at module+60h, separate from COFF headers. */
typedef struct ContributionInfo {
    WORD references_used,reference_count;
    void *references;
    struct ContributionInfo *reference_overflow;
    Contribution *previous_contribution;
    const char *name;
    DWORD fold_signature,content_signature;
    IMAGE_RELOCATION *fold_relocations;
    Contribution *fold_candidate;
    DWORD comparison_depth;
} ContributionInfo;
typedef char ContributionInfoSize[(sizeof(ContributionInfo)==40)?1:-1];
typedef char ContributionInfoFields[(offsetof(ContributionInfo,reference_count)==2 &&
    offsetof(ContributionInfo,references)==4 && offsetof(ContributionInfo,previous_contribution)==12 &&
    offsetof(ContributionInfo,name)==16)?1:-1];
typedef char ContributionInfoFoldingFields[(offsetof(ContributionInfo,content_signature)==0x18 &&
    offsetof(ContributionInfo,fold_relocations)==0x1c &&
    offsetof(ContributionInfo,comparison_depth)==0x24)?1:-1];
typedef char FoldCandidateAt20[(offsetof(ContributionInfo,fold_candidate)==0x20)?1:-1];

/* Only fields consumed by reached reconstruction logic are named here. These are prefixes, not
 * claims about an allocation's complete size or its as-yet unknown tail. */
/* The initializer allocates complete 156-byte module and 68-byte library records.
 * Name fields as their producers/consumers are reached; retain unknown gaps. */
struct CoffLibrarianContext;
struct OutputLibrary {
    char *name;
    /* Conversion contexts, distinct from module_head linked modules. */
    struct CoffLibrarianContext **import_conversions;
    DWORD conversion_capacity;
    LONG conversion_count;
    BYTE active, reserved11[3];
    const char *path;
    DWORD archive_symbol_count;
    /* Opaque library-owned index buffers, released after input processing. */
    void *index_data[5];
    DWORD flags, input_tag;
    OutputModule *module_head;
    OutputLibrary *next;
    DWORD reserved40;
#ifdef __cplusplus
    OutputModule *RECON_THISCALL FindModule(const char *,const void *);
    void RECON_THISCALL SearchSymbols(struct LinkContext *,BYTE *,BYTE *);
    void RECON_THISCALL ReadIndex(struct LinkContext *);
    BYTE RECON_THISCALL FindSymbolMember(struct LinkContext *,StringNode *,const char *,DWORD *);
    void RECON_THISCALL RememberSymbol(StringNode *);
    const char *RECON_THISCALL MemberName(const IMAGE_ARCHIVE_MEMBER_HEADER *);
#endif
};
struct OutputModule {
    /* Duplicated original filename, or a borrowed opaque identity when absent. */
    const void *identity;
    char *name;
    DWORD module_index, symbol_offset, symbol_count;
    WORD optional_header_size, characteristics;
    DWORD section_count;
    DWORD contribution_count;
    DWORD debug_symbol_index;
    IMAGE_SYMBOL *input_symbols;
    DWORD base_relocation_size;
    SectionFileMap *section_map;
    DWORD compiler_id, input_tag, timestamp, file_size;
    DWORD section_offset;
    void *optional_data;
    IMAGE_SECTION_HEADER *section_headers;
    BYTE *section_flags;
    DWORD auxiliary;
    CommonSymbol *common_symbols;
    BYTE reserved58[8];
    ContributionInfo *contribution_info;
    OutputLibrary *library;
    /* Original allocation uses 64 bytes per section; the public Contribution
       prefix does not yet describe that complete record. */
    void *contributions;
    OutputModule *next;
    void *debug_stream;
    BYTE *serialized_debug;
    DWORD serialized_debug_size;
    DWORD reserved7c;
    MapLineFile *map_line_files;
    WORD *incremental_state;
    StringNode *defined_symbols;
    DWORD *symbol_bits[2], *symbol_indices;
    BYTE flags, reserved99[3];
#ifdef __cplusplus
    IMAGE_SECTION_HEADER *RECON_THISCALL ReadSectionHeaders(IMAGE_SECTION_HEADER **);
    void RECON_THISCALL WriteOutput(struct LinkContext *,DWORD pdb_library_state);
    void RECON_THISCALL ProcessOutput(struct LinkContext *,DWORD output_state);
    void RECON_THISCALL ReopenOutput(int file);
    void RECON_THISCALL WriteCommonSymbols(BYTE reference_optimization);
    void RECON_THISCALL WriteDebug(BYTE reference_optimization);
#endif
};
struct OutputSection {
    char *name;
    DWORD rva, reserved08, raw_size, virtual_size, initialized_file_size, file_offset;
    DWORD line_number_offset, characteristics, original_characteristics, line_number_count, reserved2c;
    WORD index, reserved32;
    WORD reserved34;
    BYTE has_base_relocations,vxd_flag10;
    BYTE vxd_flag40,vxd_flag8000,vxd_flag4000,reserved3b;
    OutputSection *merge_target;
    ContributionGroup *groups;
    OutputSection *next;
};
typedef char OutputSectionVxdFlags[(offsetof(OutputSection,vxd_flag10)==0x37 &&
    offsetof(OutputSection,vxd_flag40)==0x38 && offsetof(OutputSection,vxd_flag8000)==0x39 &&
    offsetof(OutputSection,vxd_flag4000)==0x3a)?1:-1];
typedef char OutputSectionSize[(sizeof(OutputSection)==72)?1:-1];
struct ContributionGroup {
    char *name;
    DWORD contribution_count, rva, reserved0c, file_offset, size;
    BYTE alignment, has_ordered_contributions, reserved1a[2];
    Contribution *first, *last;
    ContributionGroup *next;
    OutputSection *section;
};
struct Contribution {
    DWORD rva, characteristics, raw_size, file_offset;
    Contribution *associated_contribution;
    DWORD padding;
    OutputModule *module;
    ContributionGroup *group;
    Contribution *next;
    WORD relocation_count,line_number_count;
    DWORD relocation_offset,input_raw_offset;
    BYTE reserved30[8];
    DWORD comdat_checksum;
    BYTE comdat_selection, reserved3d[3];
#ifdef __cplusplus
    void RECON_THISCALL ProcessOutput(struct LinkContext *,IMAGE_SYMBOL *,DWORD *,DWORD);
    BYTE RECON_THISCALL CanFold(const char *name);
    IMAGE_RELOCATION *RECON_THISCALL ReadRelocations(DWORD *count);
    void RECON_THISCALL ReadConvertedRelocations(void *buffer,DWORD bytes);
    void RECON_THISCALL ReadConvertedData(void *buffer,DWORD bytes);
#endif
};
typedef char ModuleCommonSymbolsAt54[(offsetof(OutputModule,common_symbols)==0x54)?1:-1];
typedef char ModuleCompilerIdAt30[(offsetof(OutputModule,compiler_id)==0x30)?1:-1];
typedef char ModuleContributionCountAt1c[(offsetof(OutputModule,contribution_count)==0x1c)?1:-1];
typedef char ContributionAssociationAndPadding[(offsetof(Contribution,associated_contribution)==0x10 && offsetof(Contribution,padding)==0x14)?1:-1];
typedef char ContributionSize[(sizeof(Contribution)==64)?1:-1];
typedef char ContributionInputFields[(offsetof(Contribution,relocation_count)==0x24 &&
    offsetof(Contribution,line_number_count)==0x26 &&
    offsetof(Contribution,relocation_offset)==0x28 &&
    offsetof(Contribution,input_raw_offset)==0x2c)?1:-1];
/* A COFF symbol is the public prefix of the linker's extended symbol record.
 * The flags and owner slots below are read by FullLink; later tail members
 * remain unknown until their users are reconstructed. */
typedef struct SymbolAuxiliary {
    struct SymbolAuxiliary *next;
    void *owners[4];
} SymbolAuxiliary;
typedef char SymbolAuxiliarySize[(sizeof(SymbolAuxiliary)==20)?1:-1];
struct StringNode {
    IMAGE_SYMBOL symbol;
    WORD input_flags;
    Contribution *contribution;
    DWORD final_value;
    union { StringNode **undefined_previous; StringNode *defined_next; };
    StringNode *undefined_next;
    union { SymbolAuxiliary *auxiliary; void *reference_owner; };
    DWORD reserved28;
    const char *diagnostic_name;
    union { DWORD flags; BYTE flag_byte; };
    WORD attributes;
};
typedef struct SectionList {
    OutputSection *first, **last;
} SectionList;
typedef struct SectionEnumerator {
    BYTE (__stdcall *next)(struct SectionEnumerator *);
    void (__stdcall *end)(struct SectionEnumerator *);
    OutputSection *first, *current;
} SectionEnumerator;
typedef struct MacResource {
    struct MacResource *next;
    DWORD kind;
    const char *filename;
    void *data;
    DWORD size;
    Contribution *contribution;
} MacResource;

typedef char OutputLibrarySize[(sizeof(OutputLibrary)==68)?1:-1];
typedef char LibraryCreatorFields[(offsetof(OutputLibrary,import_conversions)==4 &&
    offsetof(OutputLibrary,conversion_capacity)==8 && offsetof(OutputLibrary,conversion_count)==12 &&
    offsetof(OutputLibrary,active)==16 && offsetof(OutputLibrary,path)==20)?1:-1];
typedef char LibraryIndexDataAt1c[(offsetof(OutputLibrary,index_data)==0x1c)?1:-1];
typedef char LibraryFlagsAt30[(offsetof(OutputLibrary,flags)==0x30)?1:-1];
typedef char LibraryNextAt3c[(offsetof(OutputLibrary,next)==0x3c)?1:-1];
typedef char OutputModuleSize[(sizeof(OutputModule)==156)?1:-1];
typedef char ModuleConstructorFields[(offsetof(OutputModule,module_index)==8 &&
    offsetof(OutputModule,symbol_offset)==12 && offsetof(OutputModule,symbol_count)==16 &&
    offsetof(OutputModule,section_count)==24 && offsetof(OutputModule,section_offset)==64 &&
    offsetof(OutputModule,optional_data)==68 && offsetof(OutputModule,section_headers)==72 &&
    offsetof(OutputModule,section_flags)==76 && offsetof(OutputModule,auxiliary)==80 &&
    offsetof(OutputModule,contributions)==104 && offsetof(OutputModule,next)==108 &&
    offsetof(OutputModule,incremental_state)==132 && offsetof(OutputModule,symbol_bits)==140 &&
    offsetof(OutputModule,symbol_indices)==148 && offsetof(OutputModule,flags)==152)?1:-1];
typedef char LibraryModuleHeadAt38[(offsetof(OutputLibrary,module_head)==56)?1:-1];
typedef char OutputModuleSectionMapAt2c[(offsetof(OutputModule,section_map)==0x2c)?1:-1];
typedef char ModuleMapLinesAt80[(offsetof(OutputModule,map_line_files)==0x80)?1:-1];
typedef char ContributionInfoAt60[(offsetof(OutputModule,contribution_info)==0x60)?1:-1];
typedef char OutputModuleLibraryAt64[(offsetof(OutputModule,library)==0x64)?1:-1];
typedef char SectionFlagsAt20[(offsetof(OutputSection,characteristics)==0x20)?1:-1];
typedef char SectionFileOffsetAt18[(offsetof(OutputSection,file_offset)==0x18)?1:-1];
typedef char SectionIndexAt30[(offsetof(OutputSection,index)==0x30)?1:-1];
typedef char GroupAlignmentAt18[(offsetof(ContributionGroup,alignment)==0x18)?1:-1];
typedef char GroupFirstAt1c[(offsetof(ContributionGroup,first)==0x1c)?1:-1];
typedef char GroupNextAt24[(offsetof(ContributionGroup,next)==0x24)?1:-1];
typedef char GroupSectionAt28[(offsetof(ContributionGroup,section)==0x28)?1:-1];
typedef char ContributionGroupAt1c[(offsetof(Contribution,group)==0x1c)?1:-1];
typedef char ContributionNextAt20[(offsetof(Contribution,next)==0x20)?1:-1];
typedef char SymbolOwnerAt14[(offsetof(StringNode,contribution)==0x14)?1:-1];
typedef char SymbolValueAt18[(offsetof(StringNode,final_value)==0x18)?1:-1];
typedef char ContributionComdatFields[(offsetof(Contribution,comdat_checksum)==0x38 &&
    offsetof(Contribution,comdat_selection)==0x3c)?1:-1];
typedef char ModuleDefinedSymbolsAt88[(offsetof(OutputModule,defined_symbols)==0x88)?1:-1];
typedef char SymbolInputFlagsAt12[(offsetof(StringNode,input_flags)==0x12)?1:-1];
typedef char SymbolUndefinedLinks[(offsetof(StringNode,undefined_previous)==0x1c &&
    offsetof(StringNode,undefined_next)==0x20 && offsetof(StringNode,auxiliary)==0x24)?1:-1];
typedef char SymbolAttributesAt34[(offsetof(StringNode,attributes)==0x34)?1:-1];
typedef char SymbolFlagsAt30[(offsetof(StringNode,flags)==0x30)?1:-1];

#endif
