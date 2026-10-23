#include "frontend.h"
#include "output.h"
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
    __asm { int 3 }
}

void initialize_frontend_12(void)
{
    __asm { int 3 }
}

void initialize_frontend_13(void)
{
    __asm { int 3 }
}

void initialize_frontend_14(void)
{
    __asm { int 3 }
}

void initialize_frontend_15(void)
{
    __asm { int 3 }
}

void initialize_frontend_16(void)
{
    __asm { int 3 }
}

void initialize_frontend_17(void)
{
    __asm { int 3 }
}

void initialize_frontend_18(void)
{
    __asm { int 3 }
}

void initialize_frontend_19(void)
{
    __asm { int 3 }
}

void initialize_frontend_20(void)
{
    __asm { int 3 }
}

void initialize_frontend_21(void)
{
    __asm { int 3 }
}

void initialize_frontend_22(void)
{
    __asm { int 3 }
}

void initialize_frontend_23(void)
{
    __asm { int 3 }
}

void initialize_frontend_24(void)
{
    __asm { int 3 }
}

void initialize_frontend_25(void)
{
    __asm { int 3 }
}

void initialize_frontend_26(void)
{
    __asm { int 3 }
}

void initialize_frontend_27(void)
{
    __asm { int 3 }
}

void initialize_frontend_28(void)
{
    __asm { int 3 }
}

void initialize_frontend_29(void)
{
    __asm { int 3 }
}

void initialize_frontend_30(void)
{
    __asm { int 3 }
}

void initialize_frontend_31(void)
{
    __asm { int 3 }
}

void initialize_frontend_32(void)
{
    __asm { int 3 }
}

void initialize_frontend_33(void)
{
    __asm { int 3 }
}

void initialize_frontend_34(void)
{
    __asm { int 3 }
}

void initialize_frontend_35(void)
{
    __asm { int 3 }
}

void initialize_frontend_36(void)
{
    __asm { int 3 }
}

void initialize_frontend_37(void)
{
    __asm { int 3 }
}

void initialize_frontend_38(void)
{
    __asm { int 3 }
}

void initialize_frontend_39(void)
{
    __asm { int 3 }
}

void initialize_frontend_40(void)
{
    __asm { int 3 }
}

void initialize_frontend_41(void)
{
    __asm { int 3 }
}

void initialize_frontend_42(void)
{
    __asm { int 3 }
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
