#include "link.h"
/* The following independent global begins at original 0005c06c. */
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
int debug_enabled(int index) { return debug_flags[index]; }
BOOL WINAPI control_handler(DWORD event) { link_cleanup(); return FALSE; }
