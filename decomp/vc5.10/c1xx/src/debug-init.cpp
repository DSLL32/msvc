#include <io.h>
#include <new.h>
#include <string.h>
#include "frontend-init.h"
#include "pdb-context.h"
#pragma intrinsic(strlen, memcpy)
#include <recon.h>

/* PDB state used only by the unreached debug modes. These cells are
 * source-only views, excluded from native reset/snapshots.
 * The native guard precedes every read or write of this unmodeled state. */
static struct {
    unsigned input_signature;  /* cd480 */
    unsigned input_age;        /* ccf84 */
    unsigned output_signature; /* cd484 */
    unsigned output_age;       /* ccf88 */
} debug_unmodeled;

struct NameMap {
    static __declspec(dllimport) int __cdecl open(PDB *, int, NameMap **)
        RECON_MSPDB_NAMEMAP_OPEN;
};
extern "C" {
void __cdecl make_incremental_database_path(char *);
int __cdecl create_debug_module(PDB *, int, NameMap *, const char *, const char *,
    CompilerTextInput *, int, int, int, CompilerDebugModule **);
void __cdecl initialize_il_store(PDB *);
CompilerBackend *__cdecl acquire_backend(void);
CompilerBackend *__cdecl acquire_initial_backend(void);
int __cdecl report_response_error(int, ...);
}

extern "C" void __cdecl begin_source_input(void)
{
    const char *database_path;
    int mode, language;
    unsigned age, signature, saved_position;
    void *dependency_record;
#ifdef RECON_NATIVE_TEST
    if (frontend_bss.options.preprocess_stdout || frontend_bss.options.prototypes_only ||
        restore_mode || !frontend_data.options.split_pdbs ||
        frontend_data.options.line_records || !frontend_data.option_c9il ||
        frontend_data.options.incremental_compile)
        recon_unsupported("debug initialization: unsupported PDB state");
#endif
    if ((!frontend_bss.options.preprocess_stdout || frontend_data.options.dependency_output) &&
        !frontend_bss.options.prototypes_only) {
        if (!restore_mode) {
            if (frontend_data.options.split_pdbs) {
                if (frontend_data.options.line_records &&
                    !input_pdb_context.Open(frontend_data.options.database_path,
                        &debug_unmodeled.input_signature, &debug_unmodeled.input_age,
                        0x1000, "i"))
                    report_response_error(83, 354, frontend_data.options.database_path);
                database_path = frontend_data.options.database_path;
                if (frontend_data.options.split_pdbs) {
                    if (!frontend_bss.incremental_database_path[0])
                        make_incremental_database_path(frontend_bss.incremental_database_path);
                    database_path = frontend_bss.incremental_database_path;
                }
                if (!_strnicmp(frontend_data.options.database_path, database_path, 256))
                    report_compiler_diagnostic(74, frontend_data.options.database_path);
                if (!frontend_data.option_c9il || frontend_data.options.incremental_compile ||
                    frontend_data.options.incremental_rebuild ||
                    frontend_data.options.dependency_output) {
                    database_path = frontend_data.options.database_path;
                    if (frontend_data.options.split_pdbs) {
                        if (!frontend_bss.incremental_database_path[0])
                            make_incremental_database_path(frontend_bss.incremental_database_path);
                        database_path = frontend_bss.incremental_database_path;
                    }
#ifdef RECON_NATIVE_TEST
                    recon_unsupported("debug initialization: output PDB state is unmodeled");
#endif
                    if (!current_pdb_context->Open(database_path,
                        &debug_unmodeled.output_signature, &debug_unmodeled.output_age,
                        0x400, NULL)) {
                        database_path = frontend_data.options.database_path;
                        if (frontend_data.options.split_pdbs) {
                            if (!frontend_bss.incremental_database_path[0])
                                make_incremental_database_path(frontend_bss.incremental_database_path);
                            database_path = frontend_bss.incremental_database_path;
                        }
                        report_response_error(83, 354, database_path);
                    }
                }
            } else if (frontend_data.options.line_records || !frontend_data.option_c9il ||
                frontend_data.options.incremental_compile ||
                !frontend_data.options.incremental_rebuild ||
                !frontend_data.options.dependency_output) {
                if (!input_pdb_context.Open(frontend_data.options.database_path,
                    &debug_unmodeled.input_signature, &debug_unmodeled.input_age,
                    0x1000, "i"))
                    report_response_error(83, 354, frontend_data.options.database_path);
            }
            if (!frontend_data.option_c9il || frontend_data.options.incremental_compile) {
                if (!NameMap::open(current_pdb_context->database, 1,
                    &current_pdb_context->names)) {
                    database_path = frontend_data.options.database_path;
                    if (frontend_data.options.split_pdbs) {
                        if (!frontend_bss.incremental_database_path[0])
                            make_incremental_database_path(frontend_bss.incremental_database_path);
                        database_path = frontend_bss.incremental_database_path;
                    }
                    report_response_error(83, 354, database_path);
                }
            }
        }
        if (!frontend_data.option_c9il)
            initialize_il_store(current_pdb_context->database);
    }
    mode = frontend_data.options.incremental_compile &&
        !frontend_bss.options.preprocess_stdout && !frontend_bss.options.prototypes_only;
    if (!create_debug_module(current_pdb_context->database, 0,
        current_pdb_context->names, frontend_bss.options.source_file,
        frontend_data.options.object_file, frontend_bss.text_input, mode, 0, 0,
        &frontend_bss.debug_module))
        report_compiler_diagnostic(73, ".\\main.c", 581);
    if (!frontend_bss.debug_module->Slot54(frontend_bss.pdb_signature, &language))
        report_compiler_diagnostic(73, ".\\main.c", 586);
    if (language != 2) _unlink(frontend_data.options.object_file);
    if (frontend_data.options.line_records) {
        age = input_pdb_context.database ? input_pdb_context.database->QueryAge() : 0;
        signature = input_pdb_context.database ? input_pdb_context.database->QuerySignature() : 0;
        if (!frontend_bss.debug_module->SlotE0(signature, age))
            report_compiler_diagnostic(73, ".\\main.c", 601);
    }
    if (frontend_data.options.debug_types || frontend_data.options.debug_old_format)
        frontend_bss.debug_module->Slot60();
    if (frontend_data.options.incremental_rebuild_forced)
        frontend_bss.debug_module->SlotE4();
    if (frontend_data.options.incremental_rebuild || frontend_data.options.dependency_output) {
        frontend_data.dependency->SetSourceName(frontend_bss.options.source_file);
        frontend_data.dependency->RecordSource(frontend_bss.saved_context_stack.FileName(),
            frontend_data.source_descriptor, 1);
        if (frontend_data.options.incremental_rebuild) {
            dependency_record = NULL;
            if (frontend_data.dependency->QuerySource(&dependency_record) &&
                !frontend_bss.debug_module->SetSourceDependency(dependency_record))
                report_compiler_diagnostic(73, ".\\main.c", 631);
        }
    }
    frontend_data.backend = acquire_backend();
    if (frontend_data.options.pch_use || frontend_data.options.pch_auto) {
        if (!frontend_bss.debug_module->Slot0c(&frontend_data.debug_module_kind))
            report_compiler_diagnostic(73, ".\\main.c", 641);
    } else {
        if (!frontend_bss.debug_module->Slot08(&frontend_data.debug_module_kind))
            report_compiler_diagnostic(73, ".\\main.c", 645);
        if (frontend_data.saved_pch_mode || frontend_data.debug_module_kind != 1 ||
            !frontend_data.backend->Slot5c())
            frontend_data.backend->ResetMode();
    }
    if (frontend_data.options.pch_use) {
        saved_position = frontend_bss.saved_context_stack.position;
        frontend_bss.saved_context_stack.position = 0;
        frontend_data.backend->SetDebugMode(frontend_data.debug_module_kind == 5);
        frontend_bss.saved_context_stack.position = saved_position;
    } else frontend_data.backend->SetDebugMode(frontend_data.debug_module_kind == 5);
}

int RECON_THISCALL CompilerPdbContext::Open(const char *, unsigned *, unsigned *, unsigned, const char *) { __asm { int 3 } return 0; }
extern "C" void __cdecl make_incremental_database_path(char *path)
{
    const char *extension = strrchr(frontend_data.options.database_path, '.');
    unsigned length = extension ? extension - frontend_data.options.database_path :
        strlen(frontend_data.options.database_path) + 1;
#ifdef RECON_NATIVE_TEST
    if (path != frontend_bss.incremental_database_path ||
        length > sizeof(frontend_bss.incremental_database_path) - 4)
        recon_unsupported("incremental database path: unsupported output span");
#endif
    /* The no-period path copies the original NUL as well. The four-byte
     * suffix transfer does not append a terminator of its own. */
    memcpy(path, frontend_data.options.database_path, length);
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
    if (frontend_data.source_include_depth == 0) {
        unsigned position = frontend_data.browser_alternate_position ?
            frontend_bss.source_position.position : frontend_bss.saved_context_stack.position;
        frontend_bss.browser_context_position = (WORD)position;
        if (frontend_data.browser_mode == 1 || preserve)
            frontend_data.backend = acquire_backend();
        else frontend_data.backend = acquire_initial_backend();
        frontend_data.backend->Slot70();
    }
}

int RECON_THISCALL CompilerNullDebugModule::Close(void) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerNullDebugModule::Slot08(unsigned *kind)
{
    *kind = 1;
    return 1;
}
int RECON_THISCALL CompilerNullDebugModule::Slot14(unsigned, unsigned, unsigned,
    unsigned *kind, unsigned *, unsigned *, unsigned *preserve)
{
    *kind = 2;
    *preserve = 0;
    return 1;
}
int RECON_THISCALL CompilerNullDebugModule::Slot18(unsigned, unsigned, unsigned, unsigned *, unsigned *) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerNullDebugModule::Slot20(unsigned) { return 1; }
int RECON_THISCALL CompilerNullDebugModule::Slot3c(unsigned, unsigned, unsigned *result)
{
    *result = 0;
    return 1;
}
int RECON_THISCALL CompilerNullDebugModule::Slot54(unsigned signature, int *language)
{
    *language = 2;
    return 1;
}
int RECON_THISCALL CompilerNullDebugModule::Slot7c(void) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerNullDebugModule::Slotc0(unsigned, unsigned, unsigned) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerNullDebugModule::Slotcc(unsigned, unsigned *) { __asm { int 3 } return 0; }
