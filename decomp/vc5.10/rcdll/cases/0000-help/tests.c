/* Native-only regression through the existing real DLL host. This does not
 * supply an original export/callback transcript or a new CLI oracle. */
#include "rcdll-crt.h"
#include <recon-tool.h>
#include <recon.h>

extern int rcdll_native_invoke(int,char **);

int rc_no_input_usage_unit(void)
{
    char name[]="RC.EXE";
    char *arguments[]={name,NULL};
    int result=rcdll_native_invoke(1,arguments);
    return result==1&&dll_attach_count==0;
}
