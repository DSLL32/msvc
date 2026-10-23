/* Linux-only host; excluded from the target function graph. */
#include <stdio.h>
#include <float.h>
#include "frontend.h"
#include "invoke-arguments.h"
#include <recon.h>


unsigned char *c1xx_aux_old_base, *c1xx_aux_new_base;
FrontendData *c1xx_aux_old_payload, *c1xx_aux_new_payload;

/* Host-only observation after the actual stdcall has returned. */
static void checkpoint(DWORD reason, BOOL status, HINSTANCE module,
                       const ReconModuleSpec *spec)
{
    const char *path = recon_host_getenv("RECON_LIFECYCLE_STATE");
    FILE *file;
    unsigned first = onexit_begin ? (unsigned)*onexit_begin : 0;
    DWORD error = GetLastError();
    unsigned fp = _controlfp(0, 0);
    const char *cursor;
    if (!path) return;
    file = fopen(path, "ab");
    if (!file) exit(126);
    fprintf(file, "{\"reason\":%lu,\"status\":%d,\"module\":%u,"
                  "\"process_attached\":%d,\"raw_dll_entry\":%u,"
                  "\"dll_adjust_fdiv\":%d,\"dll_module\":%u,"
                  "\"onexit_begin\":%u,\"onexit_end\":%u,\"onexit_first\":%u,"
                  "\"initializer_words\":[%u,%u],\"last_error\":%lu,"
                  "\"floating_control\":%u,\"thread_calls_disabled\":%d,"
                  "\"registration\":{\"name\":\"%s\",\"sha256\":\"%s\",\"flags\":%lu,",
                  reason, status, (unsigned)module, process_attached, (unsigned)raw_dll_entry,
                  dll_adjust_fdiv, (unsigned)dll_module, (unsigned)onexit_begin, (unsigned)onexit_end,
                  first, (unsigned)crt_initializers[0], (unsigned)crt_initializers[1], error, fp,
                  recon_module_thread_calls_disabled(module), spec->name, spec->sha256, spec->flags);
    fputs("\"path\":\"", file);
    for (cursor = spec->path; *cursor; ++cursor) {
        if (*cursor == '\\' || *cursor == '"') fputc('\\', file);
        fputc(*cursor, file);
    }
    fputs("\"}}\n", file);
    if (fclose(file)) exit(126);
    SetLastError(error);
}

int main(void)
{
    ReconModuleSpec spec;
    HINSTANCE module;
    FARPROC abort_export, invoke_export;
    BOOL status;
    const char *argument_file;
    int pass_argc, pass_result;
    char **pass_argv;
    const char *seed;
    unsigned i;
    spec.name = "C1XX.DLL";
    spec.path = "R:\\toolchains\\5.10\\bin\\C1XX.DLL";
    spec.sha256 = "e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9";
    spec.flags = RECON_MODULE_NO_TLS;
    module = recon_register_module(&spec);
    if (!module || recon_module_thread_calls_disabled(module)) return 126;
    /* Captured original loader context at this DLL's guarded first entry. */
    SetLastError(203);
    /* Captured caller x87 precision is 53 bits (CW 027f). */
    _controlfp(_PC_53, _MCW_PC);
    status = _DllMainCRTStartup(module, DLL_PROCESS_ATTACH, NULL);
    checkpoint(DLL_PROCESS_ATTACH, status, module, &spec);
    if (!status) return 3;
    puts("attach=1");
    /* The native adapter verifies this exact catalog against the rebuilt real
     * Win32 DLL before executing the host. These pointers are never called. */
    abort_export = (FARPROC)AbortCompilerPass;
    invoke_export = (FARPROC)InvokeCompilerPass;
    if (!abort_export || !invoke_export || abort_export == invoke_export) return 4;
    puts("exports=1");
    argument_file = recon_host_getenv("C1XX_INVOKE_ARGV");
    if (argument_file) {
        pass_argc = read_invoke_arguments(argument_file, &pass_argv);
        seed = recon_host_getenv("C1XX_TEST_INIT_RESTORE");
        if (seed) {
            if (strcmp(seed, "restore-nonzero") && strcmp(seed, "restore-zero") && strcmp(seed, "first-copy-aux")) return 126;
            if (!strcmp(seed, "first-copy-aux")) {
                c1xx_aux_old_base = (unsigned char *)malloc(8584);
                c1xx_aux_new_base = (unsigned char *)malloc(8584);
                if (!c1xx_aux_old_base || !c1xx_aux_new_base) return 126;
                memset(c1xx_aux_old_base, 0xa7, 32); memset(c1xx_aux_old_base + 32 + 8520, 0xa7, 32);
                memset(c1xx_aux_new_base, 0xd3, 32); memset(c1xx_aux_new_base + 32 + 8520, 0xd3, 32);
                c1xx_aux_old_payload = (FrontendData *)(c1xx_aux_old_base + 32);
                c1xx_aux_new_payload = (FrontendData *)(c1xx_aux_new_base + 32);
                c1xx_aux_old_payload->first_invocation = 23;
                c1xx_aux_new_payload->first_invocation = 303;
            }
            frontend_bss.saved_state = !strcmp(seed, "first-copy-aux") ? c1xx_aux_old_payload : (FrontendData *)malloc(FRONTEND_STATE_BYTES);
            if (!frontend_bss.saved_state) return 126;
            *frontend_bss.saved_state = frontend_data;
            for (i = 0; i < 8; ++i) {
                frontend_bss.saved_state->primitive_type_ranks[i].rank += 40;
                frontend_data.primitive_type_ranks[i].code = 31337 + i;
                frontend_data.primitive_type_ranks[i].rank = 200 + i;
            }
            for (i = 0; i < 9; ++i) {
                frontend_bss.saved_state->complex_type_ranks[i].rank += 40;
                frontend_data.complex_type_ranks[i].code = 31437 + i;
                frontend_data.complex_type_ranks[i].rank = 210 + i;
            }
            frontend_bss.saved_state->primitive_type_ranks[0].code = 0x20;
            frontend_bss.saved_state->complex_type_ranks[0].code = 5;
            for (i = 0; i < sizeof(frontend_bss.primitive_type_rank); ++i)
                frontend_bss.primitive_type_rank[i] = 181;
            for (i = 0; i < sizeof(frontend_bss.complex_type_rank); ++i)
                frontend_bss.complex_type_rank[i] = 182;
            frontend_bss.saved_state->first_invocation = !strcmp(seed, "first-copy-aux") ? 23 : 17;
            frontend_data.first_invocation = !strcmp(seed, "first-copy-aux") ? 17 : 0;
            frontend_bss.cleanup_count = 32;
            for (i = 0; i < 32; ++i)
                frontend_bss.cleanup_callbacks[i] = frontend_initializers[1];
            frontend_bss.context_stack.origin = frontend_bss.saved_state;
            frontend_bss.context_stack.position = 17;
            frontend_bss.context_stack.top = frontend_bss.saved_state;
            frontend_bss.saved_context_stack.origin = frontend_bss.saved_state;
            frontend_bss.saved_context_stack.position = 17;
            frontend_bss.saved_context_stack.top = frontend_bss.saved_state;
            for (i = 0; i < 9; ++i) {
                frontend_bss.nested_context_stacks[i].origin = frontend_bss.saved_state;
                frontend_bss.nested_context_stacks[i].position = 17;
                frontend_bss.nested_context_stacks[i].top = frontend_bss.saved_state;
            }
            frontend_bss.source_position.origin = frontend_bss.saved_state;
            frontend_bss.source_position.position = 17;
            for (i = 0; i < FRONTEND_OUTPUT_BUFFERS; ++i) {
                frontend_bss.output_buffers[i].data = (unsigned char *)frontend_bss.saved_state;
                frontend_bss.output_buffers[i].cursor = (unsigned char *)frontend_bss.saved_state + 4;
                frontend_bss.output_buffers[i].capacity = 64;
                /* An explicitly unused sentinel callback; never invoked. */
                frontend_bss.output_buffers[i].relocation = (OutputRelocation)frontend_initializers[1];
                frontend_bss.output_buffers[i].relocation_context = (unsigned char *)frontend_bss.saved_state + 8;
                frontend_bss.output_buffers[i].file = (FILE *)((unsigned char *)frontend_bss.saved_state + 12);
                frontend_bss.output_buffers[i].ok = 53;
                frontend_bss.output_buffers[i].descriptor = 37;
                frontend_bss.output_buffers[i].mode = 23;
            }
            frontend_bss.deferred_work = (DeferredWorkFrame *)frontend_bss.saved_state;
            frontend_bss.token_value.integer = 0x1122334455667788i64;
            frontend_bss.sentinel_token.value.integer = -7;
            frontend_bss.sentinel_token.kind = 165;
            memset(frontend_bss.empty_metadata_prototype, 165, sizeof(frontend_bss.empty_metadata_prototype));
            frontend_bss.pch_context.image_count = 17;
            frontend_bss.pch_context.root_image = (struct PchImage *)((unsigned char *)frontend_bss.saved_state + 4);
            frontend_bss.pch_context.images = (struct PchImage **)((unsigned char *)frontend_bss.saved_state + 8);
            frontend_bss.pch_context.stored_file_position = 23;
            frontend_bss.pch_context.file_name = (const char *)((unsigned char *)frontend_bss.saved_state + 12);
            frontend_bss.pch_context.file_offset = -11;
            frontend_bss.pch_context.file = (HANDLE)((unsigned char *)frontend_bss.saved_state + 16);
            frontend_bss.pch_context.mapping_handles[0] = (HANDLE)((unsigned char *)frontend_bss.saved_state + 20);
            frontend_bss.pch_context.mapping_handles[1] = (HANDLE)((unsigned char *)frontend_bss.saved_state + 24);
            frontend_bss.pch_context.mapping_handles[2] = (HANDLE)((unsigned char *)frontend_bss.saved_state + 28);
            frontend_bss.pch_context.mapping_handles[3] = (HANDLE)((unsigned char *)frontend_bss.saved_state + 32);
            frontend_bss.pch_context.mapping_handles[4] = (HANDLE)((unsigned char *)frontend_bss.saved_state + 36);
            frontend_bss.pch_context.mapping_handles[5] = (HANDLE)((unsigned char *)frontend_bss.saved_state + 40);
            frontend_bss.pch_context.loaded_image_count = 29;
            frontend_bss.pch_context.scratch_arena = (struct CompilerArena *)((unsigned char *)frontend_bss.saved_state + 44);
            frontend_bss.pch_context.original_pages = (struct PchImage **)((unsigned char *)frontend_bss.saved_state + 48);
            frontend_bss.pch_context.mapped_pages = (struct PchImage **)((unsigned char *)frontend_bss.saved_state + 52);
            frontend_bss.type_ranks_initialized = 37;
            frontend_bss.source_signature = 37;
            for (i = 0; i < sizeof(frontend_bss.pools) / sizeof(frontend_bss.pools[0]); ++i) {
                frontend_bss.pools[i].cursor = (unsigned char *)frontend_bss.saved_state + 4;
                frontend_bss.pools[i].remaining = 23;
                frontend_bss.pools[i].minimum_pages = 37;
                frontend_bss.pools[i].arena = (struct CompilerArena *)((unsigned char *)frontend_bss.saved_state + 8);
            }
            restore_mode = 9;
            compiler_mode = !strcmp(seed, "restore-zero") ? 0 : 3;
        }
        SetLastError(203);
        pass_result = InvokeCompilerPass(pass_argc, pass_argv, 1);
        printf("invoke=%d\n", pass_result);
        if (pass_result) return pass_result;
    }
    status = _DllMainCRTStartup(module, DLL_PROCESS_DETACH, NULL);
    checkpoint(DLL_PROCESS_DETACH, status, module, &spec);
    if (!status) return 5;
    puts("detach=1");
    return 0;
}
