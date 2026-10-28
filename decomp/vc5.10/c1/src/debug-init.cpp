#include <io.h>
#include <new.h>
#include <string.h>
#include "frontend.h"
#pragma intrinsic(strlen, memcpy)
#include <recon.h>

/* These later debug/PDB cells have no encountered semantic defaults yet.
 * They are source-only storage, excluded from snapshot/reset/equality. Every
 * native path to their values is held before the first such read or effect. */
static struct {
    unsigned input_signature;       /* RVA5f2d8 */
    unsigned input_age;             /* RVA5edec */
    unsigned output_signature;      /* RVA5f2dc */
    unsigned output_age;            /* RVA5edf0 */
} debug_unmodeled;

extern "C" {
void __cdecl make_incremental_database_path(char *);
int __cdecl create_debug_module(PDB *, int, NameMap *, const char *, const char *,
    CompilerTextInput *, int, int, int, CompilerDebugModule **);
void __cdecl initialize_il_store(PDB *);
CompilerBackend *__cdecl acquire_backend(void);
CompilerBackend *__cdecl acquire_initial_backend(void);
void __cdecl report_response_error(int, ...);
}

extern "C" void __cdecl begin_source_input(void)
{
    const char *database_path;
    int mode, language;
    unsigned age, signature, saved_value;
    void *dependency_record;

    /* The current case reaches path construction using existing options and
     * session state. Other modes require their actual owners first. */
#ifdef RECON_NATIVE_TEST
    if (c1_reuse_state)
        recon_unsupported("debug initialization: reused state is not modeled");
#endif
    if ((!c1_bss.options.preprocess_stdout || c1_data.options.dependency_output) &&
        !c1_bss.options.prototypes_only) {
        if (!c1_reuse_state) {
            if (c1_data.options.split_pdbs) {
                if (c1_data.options.line_records &&
                    !c1_pdb_context.Open(c1_data.options.database_path,
                        &debug_unmodeled.input_signature, &debug_unmodeled.input_age,
                        0x1000, "i"))
                    compiler_error(83, 354, c1_data.options.database_path);
                database_path = c1_data.options.database_path;
                if (c1_data.options.split_pdbs) {
                    if (!c1_bss.incremental_database_path[0])
                        make_incremental_database_path(c1_bss.incremental_database_path);
                    database_path = c1_bss.incremental_database_path;
                }
                if (!_strnicmp(c1_data.options.database_path, database_path, 256))
                    report_response_error(74, c1_data.options.database_path);
                if (!c1_data.c9_il || c1_data.options.incremental_compile ||
                    c1_data.dependency_local || c1_data.options.dependency_output) {
                    database_path = c1_data.options.database_path;
                    if (c1_data.options.split_pdbs) {
                        if (!c1_bss.incremental_database_path[0])
                            make_incremental_database_path(c1_bss.incremental_database_path);
                        database_path = c1_bss.incremental_database_path;
                    }
                    if (!c1_current_pdb_context->Open(database_path,
                        &debug_unmodeled.output_signature, &debug_unmodeled.output_age,
                        0x400, NULL)) {
                        database_path = c1_data.options.database_path;
                        if (c1_data.options.split_pdbs) {
                            if (!c1_bss.incremental_database_path[0])
                                make_incremental_database_path(c1_bss.incremental_database_path);
                            database_path = c1_bss.incremental_database_path;
                        }
                        compiler_error(83, 354, database_path);
                    }
                }
            } else if (c1_data.options.line_records || !c1_data.c9_il ||
                c1_data.options.incremental_compile || !c1_data.dependency_local ||
                !c1_data.options.dependency_output) {
                if (!c1_pdb_context.Open(c1_data.options.database_path,
                    &debug_unmodeled.input_signature, &debug_unmodeled.input_age,
                    0x1000, "i"))
                    compiler_error(83, 354, c1_data.options.database_path);
            }
            if (!c1_data.c9_il || c1_data.options.incremental_compile) {
                if (!NameMap::open(c1_current_pdb_context->database, 1,
                    &c1_current_pdb_context->names)) {
                    database_path = c1_data.options.database_path;
                    if (c1_data.options.split_pdbs) {
                        if (!c1_bss.incremental_database_path[0])
                            make_incremental_database_path(c1_bss.incremental_database_path);
                        database_path = c1_bss.incremental_database_path;
                    }
                    compiler_error(83, 354, database_path);
                }
            }
        }
        if (!c1_data.c9_il)
            initialize_il_store(c1_current_pdb_context->database);
    }
    mode = c1_data.options.incremental_compile &&
        !c1_bss.options.preprocess_stdout && !c1_bss.options.prototypes_only;
    if (!create_debug_module(c1_current_pdb_context->database, 0,
        c1_current_pdb_context->names, c1_bss.options.source_file,
        c1_data.options.object_file, c1_bss.text_input, mode, 0, 0,
        &c1_bss.debug_module))
        report_response_error(73, ".\\main.c", 581);
    if (!c1_bss.debug_module->Slot54(c1_bss.pdb_signature, &language))
        report_response_error(73, ".\\main.c", 586);
    if (language != 2)
        _unlink(c1_data.options.object_file);
    if (c1_data.options.line_records) {
        age = c1_pdb_context.database ? c1_pdb_context.database->QueryAge() : 0;
        signature = c1_pdb_context.database ? c1_pdb_context.database->QuerySignature() : 0;
        if (!c1_bss.debug_module->SlotE0(signature, age))
            report_response_error(73, ".\\main.c", 601);
    }
    if (c1_data.options.debug_types || c1_data.options.debug_old_format)
        c1_bss.debug_module->Slot60();
    if (c1_data.dependency_local || c1_data.options.dependency_output) {
        c1_data.dependency->SetSourceName(c1_bss.options.source_file);
        c1_data.dependency->RecordSource(c1_bss.current_context.FileName(),
            c1_data.source_descriptor, 1);
        if (c1_data.dependency_local) {
            dependency_record = NULL;
            if (c1_data.dependency->QuerySource(&dependency_record) &&
                !c1_bss.debug_module->Slot88(dependency_record))
                report_response_error(73, ".\\main.c", 631);
        }
    }
    c1_data.backend = acquire_backend();
    if (c1_data.options.pch_use || c1_data.options.pch_auto) {
        if (!c1_bss.debug_module->Slot0c(&c1_data.debug_module_kind))
            report_response_error(73, ".\\main.c", 641);
    } else {
        if (!c1_bss.debug_module->Slot08(&c1_data.debug_module_kind))
            report_response_error(73, ".\\main.c", 645);
        if (c1_data.saved_pch_mode || c1_data.debug_module_kind != 1 ||
            !c1_data.backend->Disable())
            c1_data.backend->ResetMode();
    }
    if (c1_data.options.pch_use) {
        saved_value = c1_bss.current_context.value;
        c1_bss.current_context.value = 0;
        c1_data.backend->SetDebugMode(c1_data.debug_module_kind == 5);
        c1_bss.current_context.value = saved_value;
    } else
        c1_data.backend->SetDebugMode(c1_data.debug_module_kind == 5);
}

int RECON_THISCALL CompilerPdbContext::Open(const char *, unsigned *, unsigned *,
    unsigned, const char *) { __asm { int 3 } return 0; }
extern "C" void __cdecl make_incremental_database_path(char *path)
{
    const char *extension = strrchr(c1_data.options.database_path, '.');
    unsigned length = extension ?
        extension - c1_data.options.database_path :
        strlen(c1_data.options.database_path) + 1;
#ifdef RECON_NATIVE_TEST
    if (length > sizeof(c1_bss.incremental_database_path) - 4)
        recon_unsupported("incremental database path exceeds its owned buffer");
#endif
    /* The original copies the NUL too when there is no period, and transfers
     * exactly four suffix bytes. It does not write a final terminator. The
     * actual caller owns a zero-initialized BSS cache; other tails stay intact. */
    memcpy(path, c1_data.options.database_path, length);
    memcpy(path + length, ".idb", 4);
}
/* Cleanup exposes only the pointer members and twelve-byte subobjects used
 * below. Constructor-owned contents remain pending and unmodeled. */
struct CompilerDebugSequence {
    BYTE unmodeled[12];
    void RECON_THISCALL Release(void);
};
struct CompilerDebugOwnedChunks {
    void RECON_THISCALL Destroy(unsigned);
};
struct CompilerRecordedDebugModule : CompilerDebugModule {
    BYTE unmodeled_04[32];
    CompilerDebugOwnedChunks *chunks;
    BYTE unmodeled_28[8];
    CompilerDebugSequence sequence_30;
    void *buffer_3c;
    BYTE unmodeled_40[8];
    CompilerDebugSequence sequence_48;
    CompilerDebugSequence sequence_54;
    BYTE unmodeled_60[12];
    CompilerDebugSequence sequence_6c;
    void *buffer_78;
    BYTE unmodeled_7c[8];
    CompilerDebugSequence sequence_84, sequence_90;
    BYTE unmodeled_9c[4];
    void *buffer_a0;
    BYTE unmodeled_a4[8];
    void *buffer_ac;
    BYTE unmodeled_b0[8];
    CompilerDebugSequence sequence_b8, sequence_c4;
    BYTE unmodeled_d0[4];
    void *buffer_d4;
    BYTE unmodeled_d8[0x44];
    void *buffer_11c;
    BYTE unmodeled_120[24];
    void *buffer_138;
    BYTE unmodeled_13c[8];
    void *buffer_144;
    BYTE unmodeled_148[8];
    void *buffer_150;
    BYTE unmodeled_154[8];
    CompilerDebugSequence sequence_15c, sequence_168;
    BYTE unmodeled_174[56];
    CompilerRecordedDebugModule *RECON_THISCALL Initialize(PDB *, int, NameMap *,
        CompilerTextInput *, int, int, int);
    int RECON_THISCALL Open(const char *, const char *);
};
typedef char CompilerRecordedDebugModule_size[sizeof(CompilerRecordedDebugModule) == 428 ? 1 : -1];

extern "C" int __cdecl create_debug_module(PDB *database, int flags, NameMap *names,
    const char *source_name, const char *object_name, CompilerTextInput *input,
    int record, int first, int second, CompilerDebugModule **result)
{
    void *memory;
    CompilerRecordedDebugModule *module;
    if (!record) {
        memory = ::operator new(4);
        *result = memory ? new(memory) CompilerNullDebugModule : NULL;
        return *result != NULL;
    }
#ifdef RECON_NATIVE_TEST
    recon_unsupported("debug module: recorded object owners are unmodeled");
#endif
    if (input) {
        memory = ::operator new(428);
        module = memory ? ((CompilerRecordedDebugModule *)memory)->Initialize(
            database, flags, names, input, record, first, second) : NULL;
        if (module) {
            if (module->Open(source_name, object_name)) {
                *result = module;
                return 1;
            }
            module->sequence_168.Release();
            module->sequence_15c.Release();
            if (module->buffer_150) ::operator delete(module->buffer_150);
            if (module->buffer_144) ::operator delete(module->buffer_144);
            if (module->buffer_138) ::operator delete(module->buffer_138);
            if (module->buffer_11c) ::operator delete(module->buffer_11c);
            if (module->buffer_d4) ::operator delete(module->buffer_d4);
            module->sequence_c4.Release();
            module->sequence_b8.Release();
            if (module->buffer_ac) ::operator delete(module->buffer_ac);
            if (module->buffer_a0) ::operator delete(module->buffer_a0);
            module->sequence_90.Release();
            module->sequence_84.Release();
            if (module->buffer_78) ::operator delete(module->buffer_78);
            module->sequence_6c.Release();
            module->sequence_54.Release();
            module->sequence_48.Release();
            if (module->buffer_3c) ::operator delete(module->buffer_3c);
            module->sequence_30.Release();
            if (module->chunks) module->chunks->Destroy(3);
            ::operator delete(module);
        }
    }
    *result = NULL;
    return 0;
}
CompilerRecordedDebugModule *RECON_THISCALL CompilerRecordedDebugModule::Initialize(
    PDB *, int, NameMap *, CompilerTextInput *, int, int, int)
{ __asm { int 3 } return NULL; }
int RECON_THISCALL CompilerRecordedDebugModule::Open(const char *, const char *)
{ __asm { int 3 } return 0; }
void RECON_THISCALL CompilerDebugSequence::Release(void) { __asm { int 3 } }
void RECON_THISCALL CompilerDebugOwnedChunks::Destroy(unsigned) { __asm { int 3 } }

extern "C" void __cdecl initialize_il_store(PDB *) { __asm { int 3 } }
void RECON_THISCALL CompilerDependency::SetSourceName(const char *) { __asm { int 3 } }
void RECON_THISCALL CompilerBackend::SetDebugMode(int preserve)
{
    if (c1_data.source_include_depth == 0) {
        c1_bss.browser_context_value = (WORD)c1_bss.current_context.value;
        if (c1_data.browser_mode != 1 && !preserve)
            c1_data.backend = acquire_initial_backend();
        else
            c1_data.backend = acquire_backend();
        c1_data.backend->Slot5c();
    }
}

int RECON_THISCALL CompilerNullDebugModule::Close(void)
{
    ::operator delete(this);
    return 1;
}

int RECON_THISCALL CompilerNullDebugModule::Slot08(unsigned *kind)
{
    *kind = 1;
    return 1;
}

int RECON_THISCALL CompilerNullDebugModule::Slot14(unsigned, unsigned, unsigned,
    unsigned *kind, unsigned *, unsigned *, unsigned *result)
{
    *kind = 2;
    *result = 0;
    return 1;
}

int RECON_THISCALL CompilerNullDebugModule::Slot18(unsigned, unsigned, unsigned,
    unsigned *kind, unsigned *result)
{
    *kind = 1;
    *result = 0;
    return 1;
}

int RECON_THISCALL CompilerNullDebugModule::Slot20(unsigned)
{ return 1; }

int RECON_THISCALL CompilerNullDebugModule::Slot30(const char *, int)
{ return 1; }

int RECON_THISCALL CompilerNullDebugModule::Slot3c(unsigned, unsigned, unsigned *index)
{
    *index = 0;
    return 1;
}

int RECON_THISCALL CompilerNullDebugModule::Slot54(int signature, int *language)
{
    *language = 2;
    return 1;
}

int RECON_THISCALL CompilerNullDebugModule::Slot7c(void)
{
    return 0;
}

int RECON_THISCALL CompilerNullDebugModule::Slotc0(unsigned, unsigned, unsigned)
{ __asm { int 3 } return 0; }

int RECON_THISCALL CompilerNullDebugModule::Slotcc(unsigned, unsigned *)
{ __asm { int 3 } return 0; }
