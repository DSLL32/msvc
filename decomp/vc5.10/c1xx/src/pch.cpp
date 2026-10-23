#include "frontend.h"
#include <recon.h>

PchContext *RECON_THISCALL PchContext::Initialize(void)
{
    image_count = 0;
    root_image = NULL;
    images = NULL;
    stored_file_position = 0;
    file_name = NULL;
    file_offset = 0;
    file = NULL;
    mapping_handles[0] = NULL;
    mapping_handles[1] = NULL;
    mapping_handles[2] = NULL;
    mapping_handles[3] = NULL;
    mapping_handles[4] = NULL;
    mapping_handles[5] = NULL;
    loaded_image_count = 0;
    scratch_arena = NULL;
    original_pages = NULL;
    mapped_pages = NULL;
    return this;
}

extern "C" void initialize_frontend_37(void)
{
    frontend_bss.pch_context.Initialize();
}
