/* Original message-system initializer [00407680,00407686). */
#include "cvpack.h"
#include <recon.h>

typedef int (WINAPI *LoadStringProc)(HINSTANCE, UINT, LPSTR, int);
HMODULE resource_library;
LoadStringProc load_string;
HINSTANCE resource_module;
char message_buffer[1024];

int initialize_messages(const char *file, const char *program, int resources)
{
    return 1;
}

/* Original banner [004049e0,00404a29). */
void print_banner(void)
{
    printf(message_text(8008), 5, 0, 7113, '\n');
    printf("%s\n\n", message_text(8009));
    banner_pending = 0;
}

/* Original resource string lookup [00407610,00407678). */
char *message_text(unsigned int message)
{
    if (!resource_library) {
        resource_library = LoadLibraryA("USER32.DLL");
        if (!resource_library) return NULL;
    }
    if (!load_string) {
        load_string = (LoadStringProc)GetProcAddress(resource_library, "LoadStringA");
        if (!load_string) return NULL;
    }
    if (!load_string(resource_module, message, message_buffer, sizeof(message_buffer)))
        message_buffer[0] = 0;
    return message_buffer;
}
