#include "cvtres.h"
#include <recon.h>

typedef int (WINAPI *StringLoader)(HINSTANCE, UINT, LPSTR, int);
HMODULE message_library;
StringLoader message_loader;
HINSTANCE resource_instance;
char message_buffer[1024];

char *__stdcall load_message(unsigned int id)
{
    if (message_library == NULL) {
        message_library = LoadLibraryA("USER32.DLL");
        if (message_library == NULL)
            return NULL;
    }
    if (message_loader == NULL) {
        message_loader = (StringLoader)GetProcAddress(message_library, "LoadStringA");
        if (message_loader == NULL)
            return NULL;
    }
    if (!(*message_loader)(resource_instance, id, message_buffer, 1024))
        message_buffer[0] = 0;
    return message_buffer;
}
