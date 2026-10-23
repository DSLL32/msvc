/* Explicit original reference probe; native regressions never launch Wine. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <windows.h>
#include <recon.h>

int main(void)
{
    WCHAR wide[2];
    unsigned char character;
    int value,result,error;
    for (value=0;value<256;++value) {
        character=(unsigned char)value;
        wide[0]=0xaabb; wide[1]=0xccdd; errno=EDOM;
        result=mbtowc(wide,(char *)&character,1); error=errno;
        printf("byte %d %d %u %u %d\n",value,result,wide[0],wide[1],error);
        errno=EDOM; result=mbtowc(NULL,(char *)&character,1); error=errno;
        printf("null-destination %d %d %d\n",value,result,error);
    }
    wide[0]=0xaabb; wide[1]=0xccdd; errno=EDOM;
    result=mbtowc(wide,NULL,1); error=errno;
    printf("null-source %d %u %u %d\n",result,wide[0],wide[1],error);
    result=mbtowc(wide,"x",0); error=errno;
    printf("zero-capacity %d %u %u %d\n",result,wide[0],wide[1],error);
    return 0;
}
