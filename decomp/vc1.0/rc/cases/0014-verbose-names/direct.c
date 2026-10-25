/* _mktemp's first candidate is RC/RD plus the actual PID's six low decimal
 * digits. Source inputs contain neither candidate; only stdout[161..167) and
 * stdout[173..179) are symbolic. Every other byte and the full349 length remain
 * fixed, and the immutable raw streams are retained by the shared checker.
 * The frozen CLI stdout is PID316; the separate child-boundary capture is292. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <recon-output.h>
#include "rcpp.h"

int verbose_setup(void)
{
    char name[16];
    sprintf(name,"RC%06u",(unsigned)getpid()%1000000);
    if(!access(name,F_OK))return 0;
    sprintf(name,"RD%06u",(unsigned)getpid()%1000000);
    return access(name,F_OK)!=0;
}
static int read_stdout(const char *path, unsigned char *data)
{
    FILE *file=fopen(path,"rb");
    unsigned size;
    if(!file)return 0;
    size=fread(data,1,350,file);
    return fclose(file)==0 && size==349;
}
static int check_verbose(const ReconOutputContext *context)
{
    unsigned count,i;
    unsigned char raw[350],comparison[350],*expected;
    char digits[16];
    int passed=0;
    if(context->struct_size!=sizeof(*context) || !context->executor_pid ||
       context->terminal_kind!=RECON_OUTPUT_EXIT || context->terminal_value)
        return 0;
    expected=rc_read_asset("rc/expected-stdout.bin",&count);
    if(!expected || count!=349 || memcmp(expected+159,"RC000316",8) ||
       memcmp(expected+171,"RD000316",8) ||
       !read_stdout(context->stdout_raw,raw) ||
       !read_stdout(context->stdout_compare,comparison) ||
       memcmp(raw,comparison,349))goto done;
    sprintf(digits,"%06u",context->executor_pid%1000000);
    if(memcmp(raw+161,digits,6) || memcmp(raw+173,digits,6))goto done;
    for(i=0;i<349;++i) {
        if((i>=161 && i<167) || (i>=173 && i<179))continue;
        if(raw[i]!=expected[i])goto done;
    }
    memcpy(comparison+161,expected+161,6);
    memcpy(comparison+173,expected+173,6);
    passed=rc_write_file(context->stdout_compare,comparison,349);
done:
    free(expected);
    return passed;
}
RECON_EXPORT const ReconOutputPolicy case_output_policy={
    RECON_OUTPUT_POLICY_VERSION,sizeof(ReconOutputPolicy),check_verbose
};
