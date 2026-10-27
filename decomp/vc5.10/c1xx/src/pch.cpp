#include <stddef.h>
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

/* Only the vtable and kind discriminator of a loaded image are consumed
 * here. Its intervening bytes and concrete image variants remain opaque. */
struct PchImage {
    virtual void RECON_THISCALL Slot00(void) = 0;
    virtual void RECON_THISCALL Slot04(void) = 0;
    virtual void RECON_THISCALL Slot08(void) = 0;
    virtual void RECON_THISCALL Close(void) = 0;
    BYTE unmodeled_04_0b[8];
    DWORD kind;
};
typedef char PchImage_kind_offset[offsetof(PchImage, kind) == 12 ? 1 : -1];
void RECON_THISCALL PchContext::Release(void)
{
    int index;
    if (!images) return;
    for (index = 0; index < image_count; ++index) {
        PchImage *image = images[index];
        if (image && image->kind == 6) image->Close();
    }
    if (mapping_handles[0]) { CloseHandle(mapping_handles[0]); mapping_handles[0] = NULL; }
    if (mapping_handles[1]) { CloseHandle(mapping_handles[1]); mapping_handles[1] = NULL; }
    if (mapping_handles[3]) { CloseHandle(mapping_handles[3]); mapping_handles[3] = NULL; }
    if (mapping_handles[5]) { CloseHandle(mapping_handles[5]); mapping_handles[5] = NULL; }
    if (mapping_handles[2]) { CloseHandle(mapping_handles[2]); mapping_handles[2] = NULL; }
    if (mapping_handles[4]) { CloseHandle(mapping_handles[4]); mapping_handles[4] = NULL; }
    if (file) { CloseHandle(file); file = NULL; }
}
