#include <stdint.h>
#include <stdlib.h>
/* External native minimal-code test caller, outside the original graph.
 * --wrap=main preserves the original main symbol and bypasses it for this
 * explicitly scoped callback test only. This unit is never instrumented.
 */
#define RECON_SHIM_IMPL
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
typedef struct DiagnosticResourceConversion {
    const char *name;
    unsigned long (*convert)(const unsigned char *,unsigned long,unsigned char *);
    void (*display_input)(const unsigned char *,unsigned long);
    void (*display_output)(const unsigned char *,unsigned long);
} DiagnosticResourceConversion;
extern DiagnosticResourceConversion legacy_resource_types[16];
static unsigned char guarded_input[4096+64],original_input[4096];
int *diagnostic_errno;
unsigned long diagnostic_input_size;
const unsigned char *diagnostic_input;
void (*diagnostic_callback)(const unsigned char *,unsigned long);
int probe_main(int argc,char **argv)
{
    int fd,type;
    ssize_t n,extra;
    unsigned long i;
    unsigned char byte;
    if(argc!=3)return 120;
    type=atoi(argv[1]);if(type!=4 && type!=5 && type!=6)return 120;
    fd=open(argv[2],O_RDONLY);if(fd<0)return 121;
    n=read(fd,original_input,sizeof(original_input));extra=read(fd,&byte,1);
    if(close(fd) || n<=0 || extra!=0)return 121;
    diagnostic_input_size=n;memset(guarded_input,0xa5,sizeof(guarded_input));
    memcpy(guarded_input+32,original_input,n);diagnostic_input=guarded_input+32;
    diagnostic_callback=legacy_resource_types[type].display_input;
    if(!diagnostic_callback)return 122;
    diagnostic_errno=&errno;
    diagnostic_callback(diagnostic_input,diagnostic_input_size);
    if(memcmp(diagnostic_input,original_input,n))return 123;
    for(i=0;i<32;++i)
        if(guarded_input[i]!=0xa5 || guarded_input[32+n+i]!=0xa5)return 123;
    if(recon_fflush(&recon_iob[1]))return 124;
    return 0;
}

/* Direct test completion follows the original external driver return/CRT exit. */
int run_probe(void)
{
    char *args[] = {"LINK.EXE","6","payload.bin",NULL};
    int code=probe_main(3,args);
    return code==0;
}
