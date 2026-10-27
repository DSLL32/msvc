#ifndef LINK_DUMPER_H
#define LINK_DUMPER_H
#include <windows.h>
#include "link-driver.h"
#include "import-object.h"
#include <recon.h>

#ifdef __cplusplus
extern "C" {
#endif
extern LinkContext *dumper_context;
extern BYTE dump_width_warning;
/* Encountered 328-byte zero-initialized section/debug scratch. Its consumers
 * are refined as the whole section dumper is recovered. */
typedef struct DumperLabel { BYTE reserved[16]; } DumperLabel;
typedef struct DumperPublicSymbols DumperPublicSymbols;
typedef struct PDB DumperDatabase;
typedef struct DumperDebugInfo DumperDebugInfo;
typedef struct DumperSectionState {
    DumperDatabase *database;
    DumperDebugInfo *debug_info;
    DumperPublicSymbols *public_symbols;
    void *fixups;
    DWORD fixup_count,label_count;
    DumperLabel *labels;
    DWORD reserved1c;
    BYTE external_name;
    char debug_filename[MAX_PATH];
    BYTE reserved125[3];
    DWORD codeview_offset,codeview_bytes;
    DWORD symbol_base;
    IMAGE_SYMBOL **symbol_index;
    DWORD symbol_count;
    BYTE reserved13c[12];
} DumperSectionState;
typedef char DumperSectionStateLayout[(sizeof(DumperSectionState)==328 &&
    offsetof(DumperSectionState,fixups)==12 &&
    offsetof(DumperSectionState,labels)==24 &&
    offsetof(DumperSectionState,public_symbols)==8 &&
    offsetof(DumperSectionState,debug_filename)==0x21 &&
    offsetof(DumperSectionState,codeview_offset)==0x128 &&
    offsetof(DumperSectionState,codeview_bytes)==0x12c &&
    offsetof(DumperSectionState,symbol_base)==0x130 &&
    offsetof(DumperSectionState,symbol_index)==0x134 &&
    offsetof(DumperSectionState,symbol_count)==0x138)?1:-1];
extern IMAGE_SYMBOL *dump_symbols;
extern IMAGE_SECTION_HEADER *dump_section_headers;
extern char *dump_string_table;
extern BYTE dump_strings_owned;
typedef struct DumperArchiveExport {
    const char *name;
    DWORD ordinal;
    BYTE has_ordinal,reserved09[3];
} DumperArchiveExport;
typedef char DumperArchiveExportSize[(sizeof(DumperArchiveExport)==12)?1:-1];
extern DumperArchiveExport *dump_archive_exports;
extern DWORD dump_archive_export_count;
void __stdcall prepare_dumper_file(DWORD);
void __stdcall dump_sections(DumperSectionState *,DWORD,DWORD,DWORD);
void __cdecl dump_coff_map(void);
void __cdecl dump_coff_symbols(void);
void __cdecl dumper_usage(void);
void __cdecl configure_dumper_options(void);
void __stdcall dump_file(const char *);
void __stdcall sort_output_sections_by_name(SectionList *);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* Only slot4 is consumed here. The preceding provider slots have unknown
 * contracts and are never invoked or instantiated by this reconstruction. */
struct DumperPublicSymbols {
    virtual void RECON_THISCALL Unknown0(void)=0;
    virtual void RECON_THISCALL Unknown1(void)=0;
    virtual void RECON_THISCALL Unknown2(void)=0;
    virtual void RECON_THISCALL Unknown3(void)=0;
    virtual const BYTE *RECON_THISCALL NearestPublic(DWORD,DWORD,DWORD *)=0;
    virtual BOOL RECON_THISCALL Close(void)=0;
};
/* These are external receiver declarations only. Unused slots have unknown
 * contracts; no provider object or table is constructed by the application. */
struct DumperDebugInfo {
    virtual void RECON_THISCALL Unknown0(void)=0;
    virtual void RECON_THISCALL Unknown1(void)=0;
    virtual void RECON_THISCALL Unknown2(void)=0;
    virtual void RECON_THISCALL Unknown3(void)=0;
    virtual void RECON_THISCALL Unknown4(void)=0;
    virtual void RECON_THISCALL Unknown5(void)=0;
    virtual BOOL RECON_THISCALL OpenPublics(DumperPublicSymbols **)=0;
    virtual void RECON_THISCALL Unknown7(void)=0;
    virtual void RECON_THISCALL Unknown8(void)=0;
    virtual void RECON_THISCALL Unknown9(void)=0;
    virtual void RECON_THISCALL Unknown10(void)=0;
    virtual void RECON_THISCALL Unknown11(void)=0;
    virtual void RECON_THISCALL Unknown12(void)=0;
    virtual void RECON_THISCALL Unknown13(void)=0;
    virtual BOOL RECON_THISCALL Close(void)=0;
};
struct PDB {
    static int __cdecl OpenValidate(char *,char *,char *,DWORD,DWORD,LONG *,char [],PDB **) RECON_MSPDB_PDB_OPENVALIDATE;
    virtual void RECON_THISCALL Unknown0(void)=0;
    virtual void RECON_THISCALL Unknown1(void)=0;
    virtual void RECON_THISCALL Unknown2(void)=0;
    virtual void RECON_THISCALL Unknown3(void)=0;
    virtual void RECON_THISCALL Unknown4(void)=0;
    virtual void RECON_THISCALL Unknown5(void)=0;
    virtual void RECON_THISCALL Unknown6(void)=0;
    virtual BOOL RECON_THISCALL OpenDebugInfo(DWORD,const char *,DumperDebugInfo **)=0;
    virtual void RECON_THISCALL Unknown8(void)=0;
    virtual void RECON_THISCALL Unknown9(void)=0;
    virtual BOOL RECON_THISCALL Close(void)=0;
};
/* The encountered ordinary COFF dumper has one virtual reader and two fields.
 * Construction and destruction are inlined in its actual callers. */
class CoffDumper {
public:
    LinkContext *context;
    char *name;
    RECON_THISCALL CoffDumper(LinkContext *,const char *);
    RECON_THISCALL ~CoffDumper();
    virtual void RECON_THISCALL ReadSymbolTable(IMAGE_SYMBOL **);
    void RECON_THISCALL Dump(DWORD,DWORD);
};
typedef char CoffDumperSize[(sizeof(CoffDumper)==12)?1:-1];
#endif
#endif
