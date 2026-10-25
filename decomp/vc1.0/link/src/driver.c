#include "link.h"
#include <recon.h>
/* The following independent global begins at original 0005c008. */
char debug_flags[100];
void debug_initialize(char *text,char *variable)
{
    int n;
    if (!text) text=getenv(variable);
    if (!text) return;
    while (*text && !isdigit(*text)) ++text;
    while (sscanf(text,"%d",&n)==1) {
        debug_flags[n]=1;
        while (*text && isdigit(*text)) ++text;
        while (*text && !isdigit(*text)) ++text;
    }
}
/* Original [0003a970,0003a97f): only the low argument byte is stored. */
void set_debug_flag(int index,unsigned char value)
{
    debug_flags[index]=value;
}
int debug_enabled(int index) { return debug_flags[index]; }
/* The original ignores the event and always exits through linker cleanup. */
BOOL WINAPI control_handler(DWORD event)
{
    link_cleanup();
    return FALSE;
}
