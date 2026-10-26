#ifndef LINK_LIBRARIAN_H
#define LINK_LIBRARIAN_H
#include <windows.h>
#include <stddef.h>
#include "link-driver.h"
#include "import-object.h"
#include <recon.h>

/* One genuine five-slot interface. Emit takes the encountered header/name
 * pointers; its unconsumed DWORD result remains provisional. Still-pending
 * AddSymbol transports retain their physical widths. */
struct ImportThunkRelocation { BYTE offset,symbol; WORD type; };
struct ImportThunkDescription {
    const void *code;
    const ImportThunkRelocation *relocations;
    DWORD code_size;
    WORD relocation_count,reference_types[3],section_relative_type,section_index_type;
    DWORD flags,trailing_data;
};
typedef char ImportThunkLayout[(sizeof(ImportThunkRelocation)==4 &&
    sizeof(ImportThunkDescription)==32)?1:-1];

#pragma pack(push,2)
struct LibrarianCodeViewObject { WORD size,kind; DWORD signature; };
struct LibrarianCodeViewCompile {
    WORD size,kind;
    BYTE machine,language,flags,mode,version_length;
    char version[25];
};
#pragma pack(pop)
typedef char LibrarianCodeViewLayout[(sizeof(LibrarianCodeViewObject)==8 &&
    sizeof(LibrarianCodeViewCompile)==34)?1:-1];
extern "C" {
extern LibrarianCodeViewObject librarian_codeview_object;
extern LibrarianCodeViewCompile librarian_codeview_compile;
}
#pragma pack(push,2)
/* Actual60-byte transport record. Unread/reserved bytes retain their role;
 * no defaults are inferred for them from the whole-record copy. */
struct LibrarianImportInfo {
    char *name;
    DWORD name_length,padding,file_offset;
    const char *debug_name;
    BYTE debug_length,reserved15;
    WORD machine;
    BYTE type,reserved19;
    WORD name_type,ordinal,symbol_indices[4];
    BYTE reserved26[22];
};
#pragma pack(pop)
typedef char LibrarianImportInfoLayout[(sizeof(LibrarianImportInfo)==60 &&
    offsetof(LibrarianImportInfo,file_offset)==12 &&
    offsetof(LibrarianImportInfo,name_type)==26 &&
    offsetof(LibrarianImportInfo,symbol_indices)==30)?1:-1];


struct LibrarianContext {
    virtual DWORD RECON_THISCALL Emit(CoffImportHeader *,const char *)=0;
    virtual void RECON_THISCALL AddSymbol(CoffImportHeader *,const char *,DWORD,
        IMAGE_ARCHIVE_MEMBER_HEADER *,IMAGE_FILE_HEADER *,IMAGE_SECTION_HEADER *,BYTE *,LibrarianImportInfo *);
    virtual void RECON_THISCALL Prepare(DWORD);
    virtual void RECON_THISCALL Build(DWORD);
    virtual void RECON_THISCALL Finish(void)=0;
    DWORD reserved04;
    LinkContext *image;
    LinkContext *context;
    void *owned10;
    const ImportThunkDescription *thunk;
    DWORD state18;
    const char *member_name;
    BYTE flag20,reserved21[3];
    DWORD ordinal_span;
    WORD *ordinal_name_indices;
    DWORD state2c;
    char *null_thunk_name;
    DWORD debug_formats;
    WORD machine;
    BYTE reserved3a[2];
    char *owned3c;
    DWORD state40;
    char *owned44;
    IMAGE_FILE_HEADER file_header;
    int output_file;
    const char *cached_member_name;
    BYTE cached_name_length,reserved65[3];
    RECON_FORCEINLINE RECON_THISCALL LibrarianContext(LinkContext *,DWORD);
    RECON_FORCEINLINE RECON_THISCALL ~LibrarianContext();
    RECON_THISCALL LibrarianContext(int,const char *,const char *,DWORD,WORD,DWORD);
    int RECON_THISCALL Process(void);
    void RECON_THISCALL InitializeMachine(DWORD);
    DWORD RECON_THISCALL OrdinalSlots(DWORD);
    void RECON_THISCALL CreateImportSymbols(DWORD *);
    DWORD RECON_THISCALL CreateImportReferences(void);
    void RECON_THISCALL MarkExplicitOrdinals(void);
    void RECON_THISCALL EmitImportMembers(DWORD,DWORD *,DWORD *,DWORD *,const IMAGE_OPTIONAL_HEADER *);
    void RECON_THISCALL EmitImportDescriptor(const IMAGE_OPTIONAL_HEADER *);
    void RECON_THISCALL EmitNullImportDescriptor(void);
    void RECON_THISCALL EmitNullThunk(void);
    void RECON_THISCALL WriteExportObject(DWORD,DWORD,DWORD);
    void RECON_THISCALL FillExportTables(ByteBuffer *,DWORD *,DWORD,DWORD);
    void RECON_THISCALL InternExportNames(void);
    void RECON_THISCALL EmitExportRelocations(int,DWORD,DWORD,DWORD);
    void RECON_THISCALL EmitExportSymbols(int);
    void RECON_THISCALL EmitImportSymbol(const char *,DWORD,DWORD,DWORD,DWORD,DWORD,
        IMAGE_ARCHIVE_MEMBER_HEADER *,IMAGE_FILE_HEADER *,IMAGE_SECTION_HEADER *,BYTE *,LibrarianImportInfo *);
    void RECON_THISCALL EmitImportData(DWORD,LibrarianImportInfo *,const WORD *,DWORD,DWORD,DWORD,
        const char *,DWORD,const char *,DWORD,DWORD);
    void RECON_THISCALL EmitImportDebug(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportDebugRelocations(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportCode(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportCodeRelocations(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportFunctionDebug(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportFunctionRelocations(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportOrdinalThunk(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportNamedThunk(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportNameRelocation(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitImportHintName(LibrarianImportInfo *,void *,void *);
    void RECON_THISCALL EmitMacImportMember(StringNode *);
    void RECON_THISCALL CompleteLinkerMembers(void);
    void RECON_THISCALL WriteOutputs(DWORD,DWORD,DWORD,DWORD);
};
/* Shared encountered dispatch-table prefix for converted short-import sections. */
typedef void (RECON_THISCALL LibrarianContext::*ConvertedSectionEmitter)(
    LibrarianImportInfo *,void *,void *);
extern ConvertedSectionEmitter converted_data_emitters[9];
extern ConvertedSectionEmitter converted_relocation_emitters[9];
struct ExportLibrarianContext : LibrarianContext {
    RECON_FORCEINLINE RECON_THISCALL ExportLibrarianContext(LinkContext *);
    virtual DWORD RECON_THISCALL Emit(CoffImportHeader *,const char *);
    virtual void RECON_THISCALL Prepare(DWORD);
    virtual void RECON_THISCALL Build(DWORD);
    virtual void RECON_THISCALL Finish(void);
};
struct CoffLibrarianContext : LibrarianContext {
    RECON_FORCEINLINE RECON_THISCALL CoffLibrarianContext(LinkContext *);
    RECON_FORCEINLINE RECON_THISCALL CoffLibrarianContext(int,const char *,const char *,DWORD,WORD,DWORD);
    virtual DWORD RECON_THISCALL Emit(CoffImportHeader *,const char *);
    virtual void RECON_THISCALL Prepare(DWORD);
    virtual void RECON_THISCALL Finish(void);
    void RECON_THISCALL EmitShortImport(StringNode *);
    void RECON_THISCALL ConvertShortMember(int,CoffImportHeader *,DWORD,DWORD,
        IMAGE_FILE_HEADER *,IMAGE_SECTION_HEADER *,BYTE *,LibrarianImportInfo *);
    DWORD RECON_THISCALL ConvertDelayedMember(DWORD,DWORD,CoffImportHeader *,int,
        IMAGE_FILE_HEADER *,const char *,const char *,IMAGE_SECTION_HEADER *,BYTE *,LibrarianImportInfo *);
};
typedef char LibrarianContextLayout[(sizeof(struct LibrarianContext)==104 &&
    offsetof(struct LibrarianContext,image)==8 &&
    offsetof(struct LibrarianContext,context)==12 &&
    offsetof(struct LibrarianContext,debug_formats)==0x34 &&
    offsetof(struct LibrarianContext,file_header)==0x48 &&
    offsetof(struct LibrarianContext,output_file)==0x5c &&
    offsetof(struct LibrarianContext,cached_name_length)==0x64)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
BYTE __stdcall find_import_conversion(const char *,const char *,OutputLibrary *,CoffLibrarianContext **);
CoffLibrarianContext *__stdcall get_import_conversion(LinkContext *,OutputLibrary *,int,
    CoffImportHeader *,IMAGE_FILE_HEADER *,const char *,DWORD);
extern struct LibrarianContext *librarian_context;
extern BYTE librarian_remove_options;
extern const char *librarian_extract_member;
extern const char *librarian_list_output;
void __cdecl librarian_usage(void);
void __stdcall configure_librarian_options(LinkContext *);
void __stdcall read_librarian_objects(LinkContext *);
void __stdcall load_librarian_library(LinkContext *,const char *);
void __stdcall list_librarian_members(OutputLibrary *);
void __stdcall extract_librarian_member(LinkContext *);
BYTE __stdcall remove_librarian_member(LinkContext *,const char *);
void __stdcall free_librarian_inputs(LinkPointerList *);
void __stdcall write_librarian_output(LinkContext *,DWORD);
#ifdef __cplusplus
}
#endif
#endif
