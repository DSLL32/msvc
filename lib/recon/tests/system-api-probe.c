#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
int main(void)
{
    SYSTEM_INFO system; MEMORYSTATUS memory; unsigned n; FILE *file;
    char name[64]; int error; DWORD last;
    for(n=0;n<4;++n) {
        memset(&system,(n&1)?0x5a:0xa5,sizeof(system));
        memset(&memory,(n&1)?0x5a:0xa5,sizeof(memory));
        errno=(n&2)?111:0; SetLastError((n&1)?0x12345678:0);
        GetSystemInfo(&system); error=errno; last=GetLastError();
        printf("system %u %d %lu\n",n,error,last);
        sprintf(name,"system-%u.bin",n); file=fopen(name,"wb"); if(!file) return 3;
        fwrite(&system,1,sizeof(system),file); fclose(file);
        errno=(n&2)?111:0; SetLastError((n&1)?0x12345678:0);
        GlobalMemoryStatus(&memory); error=errno; last=GetLastError();
        printf("memory %u %d %lu\n",n,error,last);
        sprintf(name,"memory-%u.bin",n); file=fopen(name,"wb"); if(!file) return 4;
        fwrite(&memory,1,sizeof(memory),file); fclose(file);
    }
    return 0;
}
