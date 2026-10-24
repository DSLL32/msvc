#include "c2.h"
#include <recon.h>

int c2_cleanup_started, c2_output_incomplete;
FILE *c2_output_stream;

extern int c2_internal_error_active, c2_errors, c2_assembly_redirected;
extern char *c2_output_name, *c2_assembly_file;

void c2_cleanup(void)
{
    int (__cdecl *erase)(const char *);
    int (__cdecl *close)(FILE *);
    if (!c2_cleanup_started) {
        c2_cleanup_started = 1;
        if (!c2_internal_error_active) {
            erase = remove;
            close = fclose;
            if (c2_output_incomplete || c2_errors) {
                if (c2_output_stream)
                    close(c2_output_stream);
                if (c2_output_name)
                    erase(c2_output_name);
            }
            if (c2_assembly_redirected) {
                close(stdout);
                erase(c2_assembly_file);
            }
        }
    }
}
