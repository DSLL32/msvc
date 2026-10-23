#include "compiler-state.h"
#include <recon.h>

/* Genuine ECX member, RET0. Only the encountered file/relocation context
 * is defined here; pointed image/arena layouts remain pending. */
CompilerPchState *RECON_THISCALL CompilerPchState::Initialize(void)
{
    int index;
    image_count = 0;
    root_image = NULL;
    images = NULL;
    stored_file_position = 0;
    file_name = NULL;
    file_offset = 0;
    file = NULL;
    for (index = 0; index < 6; ++index) mapping_handles[index] = NULL;
    loaded_image_count = 0;
    scratch_arena = NULL;
    original_pages = NULL;
    mapped_pages = NULL;
    return this;
}

extern "C" void __cdecl initialize_state_1279c(void)
{
    c1_bss.pch_state.Initialize();
}
