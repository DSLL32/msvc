#include <string.h>
#include <stdlib.h>
#include "compiler-state.h"
#pragma intrinsic(memset, memcpy)
#include <recon.h>

int __cdecl initialize_compiler_pass(void)
{
    unsigned index;
    int result = 0;
    CompilerInitializedState *saved;

    if (c1_data.first_call) {
        c1_bss.saved_data = (CompilerInitializedState *)allocate_compiler_memory(6548);
        saved = c1_bss.saved_data;
        /* Typed projection only: unknown size slots/cache heads are never transferred. */
        memcpy(saved, &c1_data, offsetof(CompilerInitializedState, record_sizes));
        saved->record_sizes.known.declaration_symbol_bytes = c1_data.record_sizes.known.declaration_symbol_bytes;
        saved->record_sizes.known.bitfield_symbol_bytes = c1_data.record_sizes.known.bitfield_symbol_bytes;
        saved->record_sizes.known.member_symbol_bytes = c1_data.record_sizes.known.member_symbol_bytes;
        saved->record_sizes.known.tag_symbol_bytes = c1_data.record_sizes.known.tag_symbol_bytes;
        saved->record_sizes.known.enumerator_symbol_bytes = c1_data.record_sizes.known.enumerator_symbol_bytes;
        saved->record_sizes.known.intrinsic_symbol_bytes = c1_data.record_sizes.known.intrinsic_symbol_bytes;
        saved->record_sizes.known.global_symbol_bytes = c1_data.record_sizes.known.global_symbol_bytes;
        saved->record_sizes.known.static_symbol_bytes = c1_data.record_sizes.known.static_symbol_bytes;
        saved->record_sizes.known.named_parameter_symbol_bytes = c1_data.record_sizes.known.named_parameter_symbol_bytes;
        saved->record_sizes.known.parameter_symbol_bytes = c1_data.record_sizes.known.parameter_symbol_bytes;
        saved->record_sizes.known.abstract_symbol_bytes = c1_data.record_sizes.known.abstract_symbol_bytes;
        saved->record_sizes.known.segment_symbol_bytes = c1_data.record_sizes.known.segment_symbol_bytes;
        saved->abstract_symbol_free = c1_data.abstract_symbol_free;
        saved->symbol_cache.known.head10 = c1_data.symbol_cache.known.head10;
        saved->symbol_cache.known.head5 = c1_data.symbol_cache.known.head5;
        saved->segment_defaults.fallback = c1_data.segment_defaults.fallback;
        saved->segment_defaults.type_context = c1_data.segment_defaults.type_context;
        saved->segment_defaults.definition = c1_data.segment_defaults.definition;
        saved->void_type_list = c1_data.void_type_list;
        saved->call_context = c1_data.call_context;
        saved->condition_token = c1_data.condition_token;
        saved->condition_parentheses = c1_data.condition_parentheses;
        saved->diagnostic_context_depth = c1_data.diagnostic_context_depth;
        saved->next_diagnostic_text_handle = c1_data.next_diagnostic_text_handle;
        saved->assembly_lookahead = c1_data.assembly_lookahead;
        saved->assembly_syntax_errors = c1_data.assembly_syntax_errors;
        saved->assembly_recovery_shifts = c1_data.assembly_recovery_shifts;
        saved->diagnostic_prefix_characters = c1_data.diagnostic_prefix_characters;
        saved->diagnostic_line = c1_data.diagnostic_line;
        saved->diagnostic_source = c1_data.diagnostic_source;
        saved->browser_last_line = c1_data.browser_last_line;
        saved->browser_absolute_line = c1_data.browser_absolute_line;
        saved->browser_output_stream = c1_data.browser_output_stream;
        saved->browser_suppressed = c1_data.browser_suppressed;
        saved->browser_header_position = c1_data.browser_header_position;
        saved->browser_generation = c1_data.browser_generation;
        c1_reuse_state = 0;
    } else {
        saved = c1_bss.saved_data;
        memcpy(&c1_data, saved, offsetof(CompilerInitializedState, record_sizes));
        c1_data.record_sizes.known.declaration_symbol_bytes = saved->record_sizes.known.declaration_symbol_bytes;
        c1_data.record_sizes.known.bitfield_symbol_bytes = saved->record_sizes.known.bitfield_symbol_bytes;
        c1_data.record_sizes.known.member_symbol_bytes = saved->record_sizes.known.member_symbol_bytes;
        c1_data.record_sizes.known.tag_symbol_bytes = saved->record_sizes.known.tag_symbol_bytes;
        c1_data.record_sizes.known.enumerator_symbol_bytes = saved->record_sizes.known.enumerator_symbol_bytes;
        c1_data.record_sizes.known.intrinsic_symbol_bytes = saved->record_sizes.known.intrinsic_symbol_bytes;
        c1_data.record_sizes.known.global_symbol_bytes = saved->record_sizes.known.global_symbol_bytes;
        c1_data.record_sizes.known.static_symbol_bytes = saved->record_sizes.known.static_symbol_bytes;
        c1_data.record_sizes.known.named_parameter_symbol_bytes = saved->record_sizes.known.named_parameter_symbol_bytes;
        c1_data.record_sizes.known.parameter_symbol_bytes = saved->record_sizes.known.parameter_symbol_bytes;
        c1_data.record_sizes.known.abstract_symbol_bytes = saved->record_sizes.known.abstract_symbol_bytes;
        c1_data.record_sizes.known.segment_symbol_bytes = saved->record_sizes.known.segment_symbol_bytes;
        c1_data.abstract_symbol_free = saved->abstract_symbol_free;
        c1_data.symbol_cache.known.head10 = saved->symbol_cache.known.head10;
        c1_data.symbol_cache.known.head5 = saved->symbol_cache.known.head5;
        c1_data.segment_defaults.fallback = saved->segment_defaults.fallback;
        c1_data.segment_defaults.type_context = saved->segment_defaults.type_context;
        c1_data.segment_defaults.definition = saved->segment_defaults.definition;
        c1_data.void_type_list = saved->void_type_list;
        c1_data.call_context = saved->call_context;
        c1_data.condition_token = saved->condition_token;
        c1_data.condition_parentheses = saved->condition_parentheses;
        c1_data.diagnostic_context_depth = saved->diagnostic_context_depth;
        c1_data.next_diagnostic_text_handle = saved->next_diagnostic_text_handle;
        c1_data.assembly_lookahead = saved->assembly_lookahead;
        c1_data.assembly_syntax_errors = saved->assembly_syntax_errors;
        c1_data.assembly_recovery_shifts = saved->assembly_recovery_shifts;
        c1_data.diagnostic_prefix_characters = saved->diagnostic_prefix_characters;
        c1_data.diagnostic_line = saved->diagnostic_line;
        c1_data.diagnostic_source = saved->diagnostic_source;
        c1_data.browser_last_line = saved->browser_last_line;
        c1_data.browser_absolute_line = saved->browser_absolute_line;
        c1_data.browser_output_stream = saved->browser_output_stream;
        c1_data.browser_suppressed = saved->browser_suppressed;
        c1_data.browser_header_position = saved->browser_header_position;
        c1_data.browser_generation = saved->browser_generation;
        memset(&c1_bss, 0, sizeof(c1_bss));
        c1_bss.saved_data = saved;
        c1_reuse_state = !c1_pass_mode;
    }
    c1_data.first_call = 0;
    for (index = 0; index < 34; ++index) {
        if (c1_initialization_callbacks[index]) {
            __try {
                (*c1_initialization_callbacks[index])();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                result = 1;
            }
        }
    }
    return result;
}

/* Original checked allocation, hot0xe799..0xe7b6/cold0x2e51c..0x2e52b. */
void *__cdecl allocate_compiler_memory(unsigned size)
{
    void *memory = malloc(size);
    if (!memory)
        compiler_fatal(60);
    return memory;
}

int __cdecl cleanup_compiler_pass(void)
{
    int result = 0;
    while (c1_bss.cleanup_count > 0) {
        __try {
            (*c1_bss.cleanup_callbacks[--c1_bss.cleanup_count])();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            result = 1;
        }
    }
    return result;
}

int __cdecl register_compiler_cleanup(C1Initializer callback)
{
    if (c1_bss.cleanup_count >= 32)
        compiler_error(1, ".\\main.c", 1977);
    c1_bss.cleanup_callbacks[c1_bss.cleanup_count++] = callback;
    return 0;
}
