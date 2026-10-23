#include <string.h>
#include "frontend.h"
#include "output.h"
#include "pdb-context.h"
#include <recon.h>

void initialize_frontend_01(void)
{
    frontend_bss.context_stack.origin = NULL;
    frontend_bss.context_stack.position = 0;
    frontend_bss.context_stack.top = NULL;
}

void initialize_frontend_02(void)
{
    frontend_bss.saved_context_stack.origin = NULL;
    frontend_bss.saved_context_stack.position = 0;
    frontend_bss.saved_context_stack.top = NULL;
}

void initialize_frontend_03(void)
{
    unsigned i;
    for (i = 0; i < 9; ++i) {
        frontend_bss.nested_context_stacks[i].origin = NULL;
        frontend_bss.nested_context_stacks[i].position = 0;
        frontend_bss.nested_context_stacks[i].top = NULL;
    }
}

void initialize_frontend_04(void)
{
}

void initialize_frontend_05(void)
{
}

void initialize_frontend_06(void)
{
}

void initialize_frontend_07(void)
{
    frontend_bss.source_position.origin = NULL;
    frontend_bss.source_position.position = 0;
}

void initialize_frontend_08(void)
{
    initialize_output_buffer_01();
    register_cleanup(destroy_output_buffer_01);
}

void initialize_frontend_09(void)
{
    initialize_output_buffer_02();
    register_cleanup(destroy_output_buffer_02);
}

void initialize_frontend_10(void)
{
    initialize_output_buffer_03();
    register_cleanup(destroy_output_buffer_03);
}

void initialize_frontend_11(void)
{
    initialize_output_buffer_04();
    register_cleanup(destroy_output_buffer_04);
}

void initialize_frontend_12(void)
{
    initialize_output_buffer_05();
    register_cleanup(destroy_output_buffer_05);
}

void initialize_frontend_13(void)
{
    initialize_output_buffer_06();
    register_cleanup(destroy_output_buffer_06);
}

void initialize_frontend_14(void)
{
    initialize_output_buffer_07();
    register_cleanup(destroy_output_buffer_07);
}

void initialize_frontend_15(void)
{
    initialize_output_buffer_08();
    register_cleanup(destroy_output_buffer_08);
}

void initialize_frontend_16(void)
{
    initialize_deferred_work();
    register_cleanup(destroy_deferred_work);
}

void initialize_frontend_17(void)
{
}

void initialize_frontend_18(void)
{
    frontend_bss.sentinel_token.kind = 1;
    frontend_bss.sentinel_token.value = frontend_bss.token_value;
}

void initialize_frontend_19(void)
{
}

void initialize_frontend_20(void)
{
}

void initialize_frontend_21(void)
{
    frontend_bss.source_signature = (unsigned long)-1;
}

void initialize_frontend_22(void)
{
    initialize_pdb_context_01();
    register_cleanup(destroy_pdb_context_01);
}

void initialize_frontend_23(void)
{
    initialize_pdb_context_02();
    register_cleanup(destroy_pdb_context_02);
}











void initialize_frontend_34(void)
{
    frontend_bss.type_ranks_initialized = initialize_type_ranks();
}

void initialize_frontend_35(void)
{
}

void initialize_frontend_36(void)
{
    unsigned i;
    for (i = 0; i < sizeof(frontend_bss.empty_metadata_prototype); ++i)
        frontend_bss.empty_metadata_prototype[i] = 0;
}


void initialize_frontend_38(void)
{
    frontend_bss.pending_group_head = NULL;
    frontend_bss.active_group = NULL;
}

void initialize_frontend_39(void)
{
    initialize_generated_comment();
    register_cleanup(destroy_generated_comment);
}

/* The original uses the compiler's inline string intrinsics here. */
#ifndef RECON_NATIVE_TEST
#pragma intrinsic(strcpy, strcat, strlen)
#endif
void initialize_generated_comment(void)
{
    frontend_bss.com_initialized = 0;
    strcpy(frontend_bss.generated_comment, "// Created by ");
    strcat(frontend_bss.generated_comment,
           "Microsoft (R) C/C++ Compiler Version 11.00.7307");
    strcat(frontend_bss.generated_comment, " (");
    frontend_bss.comment_value_offset = strlen(frontend_bss.generated_comment);
    strcat(frontend_bss.generated_comment, "XXXXXXXX).");
    frontend_bss.generated_comment_length = strlen(frontend_bss.generated_comment);
}

void destroy_generated_comment(void)
{
    __asm { int 3 }
}

void initialize_frontend_40(void)
{
}

void initialize_frontend_41(void)
{
    frontend_bss.deferred_action_head = NULL;
    frontend_bss.deferred_action_tail = NULL;
    frontend_bss.deferred_action_depth = 0;
}

void initialize_frontend_42(void)
{
}

Initializer frontend_initializers[FRONTEND_INITIALIZERS] = {
    NULL,
    initialize_frontend_01,
    initialize_frontend_02,
    initialize_frontend_03,
    initialize_frontend_04,
    initialize_frontend_05,
    initialize_frontend_06,
    initialize_frontend_07,
    initialize_frontend_08,
    initialize_frontend_09,
    initialize_frontend_10,
    initialize_frontend_11,
    initialize_frontend_12,
    initialize_frontend_13,
    initialize_frontend_14,
    initialize_frontend_15,
    initialize_frontend_16,
    initialize_frontend_17,
    initialize_frontend_18,
    initialize_frontend_19,
    initialize_frontend_20,
    initialize_frontend_21,
    initialize_frontend_22,
    initialize_frontend_23,
    initialize_frontend_24,
    initialize_frontend_25,
    initialize_frontend_26,
    initialize_frontend_27,
    initialize_frontend_28,
    initialize_frontend_29,
    initialize_frontend_30,
    initialize_frontend_31,
    initialize_frontend_32,
    initialize_frontend_33,
    initialize_frontend_34,
    initialize_frontend_35,
    initialize_frontend_36,
    initialize_frontend_37,
    initialize_frontend_38,
    initialize_frontend_39,
    initialize_frontend_40,
    initialize_frontend_41,
    initialize_frontend_42
};
