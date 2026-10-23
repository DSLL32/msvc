#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <windows.h>
#include <recon.h>
int main(void)
{
    int descriptors[6]={-1,0,1,2,3,10000};unsigned i,n;char text[]="az\t";char *next;int value,error;DWORD last;
    for(n=0;n<4;++n) {
        for(i=0;i<6;++i) {
            errno=(n&1)?111:0;_doserrno=77;SetLastError((n&2)?0x12345678:0);
            value=_isatty(descriptors[i]);error=errno;last=GetLastError();
            printf("tty %d %d %d %lu %lu\n",descriptors[i],value,error,_doserrno,last);
        }
        for(i=0;i<4;++i) {
            errno=(n&1)?111:0;SetLastError((n&2)?0x12345678:0);
            next=CharNextA(text+i);error=errno;last=GetLastError();
            printf("next %u %d %d %lu\n",i,(int)(next-(text+i)),error,last);
        }
    }
    return 0;
}
