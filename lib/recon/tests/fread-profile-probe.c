/* Explicit canonical-provider file-buffer/direct-read probe. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <recon.h>
static unsigned sizes[]={2,1,1,3,2,1};
static unsigned counts[]={4098,1024,513,2732,4098,0};
static void save(const char *name,const void *data,unsigned bytes)
{
    FILE *file=fopen(name,"wb");if(!file||fwrite(data,1,bytes,file)!=bytes)exit(92);fclose(file);
}
int main(void)
{
    FILE *stream;unsigned n,result;int entry,saved;DWORD error,dos,physical;
    unsigned char output[8300],before[4096],after[4096];char name[80];
    for(n=0;n<6;++n){
        stream=fopen("input.bin","rb");if(!stream)return 90;
        if(fgetc(stream)==EOF || fseek(stream,0,SEEK_SET))return 91;
        if(n==4 && fgetc(stream)==EOF)return 91;
        memcpy(before,stream->_base,4096);memset(output,0xa5,sizeof(output));
        entry=stream->_ptr-stream->_base;
        printf("before %u %d %d %d %d\n",n,stream->_flag,stream->_bufsiz,entry,stream->_cnt);
        errno=0;_doserrno=0;SetLastError(0);
        result=fread(output,sizes[n],counts[n],stream);saved=errno;dos=_doserrno;error=GetLastError();
        physical=SetFilePointer((HANDLE)_get_osfhandle(_fileno(stream)),0,NULL,FILE_CURRENT);
        memcpy(after,stream->_base,4096);
        printf("after %u result%u flag%d capacity%d ptr%d cnt%d pos%lu errno%d dos%lu last%lu\n",n,result,stream->_flag,stream->_bufsiz,(int)(stream->_ptr-stream->_base),stream->_cnt,physical,saved,dos,error);
        sprintf(name,"case-%u-output.bin",n);save(name,output,sizeof(output));
        sprintf(name,"case-%u-buffer-before.bin",n);save(name,before,sizeof(before));
        sprintf(name,"case-%u-buffer-after.bin",n);save(name,after,sizeof(after));
        fclose(stream);
    }
    /* Return a short final item while retaining its actually written bytes. */
    stream=fopen("short.bin","rb");if(!stream)return 90;
    if(fgetc(stream)==EOF || fseek(stream,0,SEEK_SET))return 91;
    memset(output,0xa5,sizeof(output));errno=0;_doserrno=0;SetLastError(0);
    result=fread(output,2,4098,stream);saved=errno;dos=_doserrno;error=GetLastError();
    printf("partial result%u flag%d capacity%d ptr%d cnt%d errno%d dos%lu last%lu\n",result,stream->_flag,stream->_bufsiz,(int)(stream->_ptr-stream->_base),stream->_cnt,saved,dos,error);
    save("partial-output.bin",output,sizeof(output));fclose(stream);
    return 0;
}
