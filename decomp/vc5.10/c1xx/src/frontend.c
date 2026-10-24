#include "frontend.h"
#include <recon.h>

FrontendData frontend_data = {
    1,
    { { 0x10, 1 }, { 0x40, 2 }, { 0x80, 3 }, { 0x100, 4 }, { 0x200, 5 }, { 0x400, 6 }, { 0x800, 7 }, { 0x0, 0 } },
    { { 0x1, 8 }, { 0x2, 9 }, { 0x3, 10 }, { 0x4, 11 }, { 0x10, 12 }, { 0x20, 13 }, { 0x40, 14 }, { 0x44, 15 }, { 0x0, 0 } },
    0, 0, 0,
    { { 0 }, 4096, NULL, NULL, 0, 0, 0, 65536, 0, 0 },
    0, 0, 100,
    {
        NULL, /* argument_allocation */
        NULL, /* packing_stack */
        NULL, /* include_aliases */
        { 0 }, /* warning_levels */
        1000, /* lowest_warning */
        700, /* highest_warning */
        0, /* preprocess_only */
        0, /* language_extensions */
        0, /* option_zbe */
        0, /* fastcall_calls */
        0, /* japanese_identifiers */
        0, /* unsigned_char */
        0, /* member_pointer_best_case */
        0, /* member_pointer_general */
        0, /* member_pointer_single */
        0, /* member_pointer_multiple */
        0, /* member_pointer_virtual */
        1, /* vtordisp */
        0, /* dll_runtime */
        0, /* multithread_runtime */
        0, /* pascal_calls */
        0, /* debug_info */
        0, /* pch_debug */
        0, /* preprocess_no_lines */
        0, /* option_pf */
        175, /* option_bu */
        0, /* reproducible */
        0, /* intrinsics */
        1, /* stack_checks */
        0, /* cdecl_calls */
        0, /* stdcall_calls */
        0, /* pool_strings */
        0, /* readonly_strings */
        0, /* code_based_strings */
        0, /* processor */
        1, /* minimal_rebuild */
        0, /* inline_level */
        0, /* exception_handling */
        0, /* runtime_type_info */
        0, /* no_bool */
        0, /* dependency_output */
        0, /* incremental_rebuild */
        1, /* optimize_speed */
        0, /* assume_no_alias */
        0, /* assume_no_global_alias */
        0, /* global_optimize */
        0, /* precise_float */
        0, /* omit_frame_pointer */
        1, /* line_records */
        { 0 }, /* database_path */
        { 0 }, /* pch_link_path */
        0, /* option_oq */
        0, /* option_ov */
        0, /* incremental_compile */
        0, /* debug_types */
        1, /* function_records */
        1, /* split_pdbs */
        0, /* no_eh_for_new */
        0, /* incremental_rebuild_forced */
        0, /* extern_c_no_throw */
        0, /* pch_use */
        NULL, /* pch_use_file */
        0, /* pch_create */
        NULL, /* pch_create_file */
        0, /* pch_auto */
        NULL, /* pch_auto_file */
        NULL, /* pch_link_name */
        NULL, /* pch_file */
        1, /* pch_no_preprocess */
        NULL, /* new_version */
        NULL, /* object_file */
        "vc50.pdb", /* database_file */
        1, /* warning_level */
        "\\:/", /* path_separators */
        (char *)compiler_empty_string, /* il_prefix */
        -1, /* packing_alignment */
        0, /* no_expand_inline */
        NULL, /* unrecognized_argument */
        0, /* debug_old_format */
        0, /* move_pch */
        0 /* option_bcmpconv */
    },
    -1, /* language_context_depth */
    C1XX_SCANNER_DEFAULTS
};
FrontendBss frontend_bss;
int restore_mode;

int initialize_compiler_pass(void)
{
    FrontendData *saved;
    Initializer *initializer;
    int error = 0;
    unsigned i;
    if (frontend_data.first_invocation) {
        frontend_bss.saved_state = (FrontendData *)checked_malloc(FRONTEND_STATE_BYTES);
        *frontend_bss.saved_state = frontend_data;
        restore_mode = 0;
    } else {
        saved = frontend_bss.saved_state;
        frontend_data = *saved;
        frontend_bss.saved_state = NULL;
        frontend_bss.cleanup_count = 0;
        for (i = 0; i < 32; ++i)
            frontend_bss.cleanup_callbacks[i] = NULL;
        frontend_bss.context_stack.origin = NULL;
        frontend_bss.context_stack.position = 0;
        frontend_bss.context_stack.top = NULL;
        frontend_bss.saved_context_stack.origin = NULL;
        frontend_bss.saved_context_stack.position = 0;
        frontend_bss.saved_context_stack.top = NULL;
        for (i = 0; i < 9; ++i) {
            frontend_bss.nested_context_stacks[i].origin = NULL;
            frontend_bss.nested_context_stacks[i].position = 0;
            frontend_bss.nested_context_stacks[i].top = NULL;
        }
        frontend_bss.source_position.origin = NULL;
        frontend_bss.source_position.position = 0;
        for (i = 0; i < FRONTEND_OUTPUT_BUFFERS; ++i) {
            frontend_bss.output_buffers[i].data = NULL;
            frontend_bss.output_buffers[i].cursor = NULL;
            frontend_bss.output_buffers[i].capacity = 0;
            frontend_bss.output_buffers[i].relocation = NULL;
            frontend_bss.output_buffers[i].relocation_context = NULL;
            frontend_bss.output_buffers[i].file = NULL;
            frontend_bss.output_buffers[i].ok = 0;
            frontend_bss.output_buffers[i].descriptor = 0;
            frontend_bss.output_buffers[i].mode = 0;
        }
        frontend_bss.deferred_work = NULL;
        frontend_bss.token_value.integer = 0;
        frontend_bss.sentinel_token.kind = 0;
        frontend_bss.sentinel_token.value.integer = 0;
        frontend_bss.source_signature = 0;
        for (i = 0; i < sizeof(frontend_bss.pools) / sizeof(frontend_bss.pools[0]); ++i) {
            frontend_bss.pools[i].cursor = NULL;
            frontend_bss.pools[i].remaining = 0;
            frontend_bss.pools[i].minimum_pages = 0;
            frontend_bss.pools[i].arena = NULL;
        }
        frontend_bss.type_ranks_initialized = 0;
        for (i = 0; i < sizeof(frontend_bss.primitive_type_rank); ++i)
            frontend_bss.primitive_type_rank[i] = 0;
        for (i = 0; i < sizeof(frontend_bss.complex_type_rank); ++i)
            frontend_bss.complex_type_rank[i] = 0;
        for (i = 0; i < sizeof(frontend_bss.empty_metadata_prototype); ++i)
            frontend_bss.empty_metadata_prototype[i] = 0;
        frontend_bss.pch_context.image_count = 0;
        frontend_bss.pch_context.root_image = NULL;
        frontend_bss.pch_context.images = NULL;
        frontend_bss.pch_context.stored_file_position = 0;
        frontend_bss.pch_context.file_name = NULL;
        frontend_bss.pch_context.file_offset = 0;
        frontend_bss.pch_context.file = NULL;
        frontend_bss.pch_context.mapping_handles[0] = NULL;
        frontend_bss.pch_context.mapping_handles[1] = NULL;
        frontend_bss.pch_context.mapping_handles[2] = NULL;
        frontend_bss.pch_context.mapping_handles[3] = NULL;
        frontend_bss.pch_context.mapping_handles[4] = NULL;
        frontend_bss.pch_context.mapping_handles[5] = NULL;
        frontend_bss.pch_context.loaded_image_count = 0;
        frontend_bss.pch_context.scratch_arena = NULL;
        frontend_bss.pch_context.original_pages = NULL;
        frontend_bss.pch_context.mapped_pages = NULL;
        frontend_bss.pending_group_head = NULL;
        frontend_bss.active_group = NULL;
        frontend_bss.com_initialized = 0;
        for (i = 0; i < sizeof(frontend_bss.generated_comment); ++i)
            frontend_bss.generated_comment[i] = 0;
        frontend_bss.comment_value_offset = 0;
        frontend_bss.generated_comment_length = 0;
        frontend_bss.deferred_action_head = NULL;
        frontend_bss.deferred_action_tail = NULL;
        frontend_bss.deferred_action_depth = 0;
        frontend_bss.break_into_debugger = 0;
        frontend_bss.command_argc = 0;
        frontend_bss.command_argv = NULL;
        frontend_bss.current_argument = NULL;
        for (i = 0; i < 2; ++i) {
            frontend_bss.work_buffers[i].data = NULL;
            frontend_bss.work_buffers[i].limit = NULL;
        }
        for (i = 0; i < 9; ++i) {
            frontend_bss.arenas[i].cursor = NULL;
            frontend_bss.arenas[i].committed_limit = NULL;
            frontend_bss.arenas[i].base = NULL;
            frontend_bss.arenas[i].committed_bytes = 0;
            frontend_bss.arenas[i].descriptor = NULL;
            frontend_bss.arenas[i].reserve_bytes = 0;
            frontend_bss.arenas[i].reset_value = 0;
            frontend_bss.arenas[i].mapped_view = 0;
        }
        frontend_bss.options.edit_and_continue = 0;
        frontend_bss.options.option_zn = 0;
        frontend_bss.options.include_directories = NULL;
        frontend_bss.options.source_file = NULL;
        frontend_bss.options.option_zf = 0;
        frontend_bss.options.version_string = NULL;
        frontend_bss.options.forced_includes = NULL;
        frontend_bss.options.show_includes = 0;
        frontend_bss.options.defines = NULL;
        frontend_bss.options.prototypes_only = 0;
        frontend_bss.options.preprocessed_file = NULL;
        frontend_bss.options.browser_file = NULL;
        frontend_bss.options.option_binl = 0;
        frontend_bss.options.preprocess_stdout = 0;
        frontend_bss.options.browser_local_file = NULL;
        frontend_bss.options.preserve_comments = 0;
        frontend_bss.options.dump_options = 0;
        frontend_bss.options.option_xc = 0;
        frontend_bss.options.warnings_are_errors = 0;
        frontend_bss.options.suppress_banner = 0;
        frontend_bss.options.option_zc = 0;
        frontend_bss.options.fat = 0;
        frontend_bss.options.pch_memory_limit = 0;
        frontend_bss.options.response_file = NULL;
        frontend_bss.options.error_file = NULL;
        for (i = 0; i < 10; ++i)
            frontend_bss.language_contexts[i] = 0;
        for (i = 0; i < 256; ++i) {
            frontend_bss.checksum_tables.update[i] = 0;
            frontend_bss.checksum_tables.high[i] = 0;
            frontend_bss.checksum_tables.low[i] = 0;
            frontend_bss.filename_case_map[i] = 0;
        }
        frontend_bss.saved_state = saved;
        restore_mode = compiler_mode == 0;
    }
    frontend_data.first_invocation = 0;
    for (initializer = frontend_initializers;
            initializer < frontend_initializers + FRONTEND_INITIALIZERS; ++initializer) {
        if (*initializer) {
            __try {
                (*initializer)();
            } __except (1) {
                error = 1;
            }
        }
    }
    return error;
}

void *checked_malloc(unsigned bytes)
{
    void *block = malloc(bytes);
    if (block == NULL)
        report_compiler_error(60);
    return block;
}

int report_compiler_error(int number)
{
    __asm { int 3 }
    return 0;
}

int main(int argc, char **argv)
{
    frontend_data.compiler_phase = 1;
    initialize_work_buffers();
    register_cleanup(cleanup_main);
    frontend_bss.command_argc = argc;
    frontend_bss.command_argv = argv;
    scan_internal_switches(argv);
    initialize_frontend_hook();
    GetSystemInfo(&frontend_data.system_info);
    initialize_compiler_environment();
    process_compiler_options(argc, argv);
    if (frontend_bss.break_into_debugger)
        DebugBreak();
    if (frontend_data.option_c9il)
        frontend_data.compiler_flags |= 0x2000;
    else
        frontend_data.compiler_flags &= ~0x2000U;
    return compile_translation_unit();
}

void initialize_work_buffers(void)
{
    frontend_bss.work_buffers[0].data = reserve_work_buffer(0x500000, 0x4000);
    frontend_bss.work_buffers[0].limit = frontend_bss.work_buffers[0].data + 0x3ffb;
    frontend_bss.work_buffers[1].data = reserve_work_buffer(0x500000, 0x4000);
    frontend_bss.work_buffers[1].limit = frontend_bss.work_buffers[1].data + 0x3ffb;
}

BYTE *reserve_work_buffer(unsigned reserve_bytes, unsigned commit_bytes)
{
    BYTE *buffer;
    buffer = (BYTE *)VirtualAlloc(NULL, reserve_bytes, MEM_RESERVE, PAGE_READWRITE);
    if (!buffer)
        report_compiler_error(60);
    if (!VirtualAlloc(buffer, commit_bytes, MEM_COMMIT, PAGE_READWRITE))
        report_compiler_error(60);
    return buffer;
}

void scan_internal_switches(char **argv)
{
    char option[514];
    char *environment;
    int state = 0;

    while (*++argv)
        state = process_early_option(*argv, state);

    environment = getenv("MSC_CMD_FLAGS");
    state = 0;
    if (environment) {
        for (;;) {
            environment = read_command_argument(environment, option, sizeof(option));
            if (!*option)
                break;
            state = process_early_option(option, state);
        }
    }
}

int process_early_option(char *argument, int state)
{
    if (!argument)
        return 0;
    for (;;) {
        switch (state) {
        case 0:
            if (argument[0] == '-' && argument[1] == 'B' && argument[2] == 'm')
                state = 1;
            else if (argument[0] == '-' && argument[1] == 'Z' && argument[2] == 'B')
                state = 2;
            else if (argument[0] == '-' && argument[1] == 'Z' && argument[2] == 'm')
                state = 3;
            else
                return 0;
            argument += 3;
            break;
        case 1:
            frontend_data.option_bm = atoi(argument);
            return 0;
        case 2:
            frontend_data.option_zb = atoi(argument);
            return 0;
        case 3:
            frontend_data.option_zm_percent = atoi(argument);
            if (!frontend_data.option_zm_percent)
                frontend_data.option_zm_percent = 100;
            return 0;
        }
        if (!*argument)
            return state;
        if (!state)
            return 0;
    }
}

char *read_command_argument(char *source, char *argument, unsigned capacity)
{
    __asm { int 3 }
    return NULL;
}

void initialize_frontend_hook(void)
{
}


int compile_translation_unit(void)
{
    __asm { int 3 }
    return 0;
}

void cleanup_main(void)
{
    __asm { int 3 }
}

int cleanup_compiler_pass(void)
{
    __asm { int 3 }
    return 0;
}

int report_compiler_diagnostic(int number, ...)
{
    __asm { int 3 }
    return 0;
}

int register_cleanup(Initializer cleanup)
{
    if (frontend_bss.cleanup_count >= 32)
        report_compiler_diagnostic(1, ".\\main.c", 1977);
    frontend_bss.cleanup_callbacks[frontend_bss.cleanup_count++] = cleanup;
    return 0;
}

void initialize_deferred_work(void)
{
    frontend_bss.deferred_work = NULL;
}

void destroy_deferred_work(void)
{
    __asm { int 3 }
}
