/* Linux-only fixture infrastructure. Not reconstructed target code. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rcdll.h"
#include <recon.h>

static FILE *events;
HINSTANCE fixture_module;
BOOL fixture_thread_calls_disabled;
ReconModuleSpec fixture_module_spec;
const ReconModuleSpec *fixture_registered_identity;
int fixture_argument_count;
char **fixture_argument_vector;
char *fixture_argument_storage[257];
unsigned fixture_argument_lengths[257];
static void bytes(const char *tag, const char *text, unsigned size)
{
    const unsigned char *at = (const unsigned char *)text;
    fprintf(events, "%s ", tag);
    fprintf(events, "%u ", size);
    while (size--) fprintf(events, "%02x", *at++);
    fprintf(events, "\n");
    fflush(events);
}
static int WINAPI fixture_diagnostic(DWORD first, DWORD second, char *message)
{
    unsigned length = strlen(message), count = 0;
    char *at, *escaped, *out;
    fprintf(events, "callback %lu %lu\n", first, second);
    bytes("before", message, length+1);
    if (length >= 2 && ((message[length-1] == '\n' && message[length-2] == '\r') ||
        (message[length-1] == '\r' && message[length-2] == '\n')))
        message[length-2] = 0;
    if (*message) {
        if (strchr(message, '%')) {
            for (at=message; *at; ++at) if (*at == '%') ++count;
            escaped = malloc(strlen(message) + count + 1);
            if (escaped) {
                out=escaped;
                for (at=message; *at; ++at) {
                    if (*at == '%') *out++='%';
                    *out++=*at;
                }
                *out=0;
                printf(escaped);
                /* Original RC host does not free this allocation. */
            } else {
                while ((at=strchr(message,'%')) != NULL) *at=' ';
                printf(message);
            }
        } else printf(message);
    }
    printf("\n");
    bytes("after", message, length+1);
    fprintf(events, "callback-return 0\n");
    fflush(events);
    return 0;
}
void fixture_complete(int result)
{
    /* Native debugger observation point outside the reconstructed DLL graph. */
}
int main(int count, char **arguments)
{
    int result;
    HINSTANCE module;
    char *program = getenv("RECON_RC_ARGV0");
    DWORD opaque_startup = 0;
    char option[] = "/help";
    char *vector[257];
    int target_count, i;
    if (count < 2 || count > 257 || !program) return 125;
    target_count = count == 2 ? 2 : count-1;
    events = fopen(arguments[1], "wb");
    if (!events) return 125;
    fixture_module_spec.name = "RCDLL.dll";
    fixture_module_spec.path = getenv("RECON_RC_DLL_PATH");
    fixture_module_spec.sha256 = "bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0";
    fixture_module_spec.flags = RECON_MODULE_NO_TLS;
    if (getenv("RCDLL_NEGATIVE_MODULE_SHA"))
        fixture_module_spec.sha256 = getenv("RCDLL_NEGATIVE_MODULE_SHA");
    if (!fixture_module_spec.path) return 125;
    fixture_module = recon_register_module(&fixture_module_spec);
    fixture_registered_identity = recon_module_identity(fixture_module);
    if (!fixture_module || !fixture_registered_identity ||
        recon_module_thread_calls_disabled(fixture_module)) return 126;
    if (!getenv("RECON_RC_RESOURCES")) return 125;
    recon_load_module_resources(fixture_module,getenv("RECON_RC_RESOURCES"));
    module = fixture_module;
    vector[0]=program;
    if (count==2) vector[1]=option;
    else for (i=1; i<target_count; ++i) vector[i]=arguments[i+1];
    vector[target_count]=NULL;
    fixture_argument_count=target_count;fixture_argument_vector=vector;
    for(i=0;i<target_count;++i){fixture_argument_storage[i]=vector[i];fixture_argument_lengths[i]=strlen(vector[i])+1;}
    fprintf(events,"attach\n"); fflush(events);
    if (!recon_dll_entry(module,DLL_PROCESS_ATTACH,&opaque_startup)) return 126;
    fixture_thread_calls_disabled = recon_module_thread_calls_disabled(module);
    if (!fixture_thread_calls_disabled) return 126;
    fprintf(events,"export-call RC 0 0 callback 0 %d\n",target_count); fflush(events);
    result=RC(NULL,0,fixture_diagnostic,NULL,target_count,vector);
    fprintf(events,"export-return %d\n",result); fflush(events);
    fprintf(events,"detach\n"); fflush(events);
    if (!recon_dll_entry(module,DLL_PROCESS_DETACH,(LPVOID)1)) return 126;
    fprintf(events,"detach-return 1\n");
    fclose(events);
    fixture_complete(result);
    return result;
}
