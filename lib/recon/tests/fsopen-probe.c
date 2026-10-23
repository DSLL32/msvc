#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <windows.h>
#include <errno.h>
#include <recon.h>
static void show(const char *role,unsigned row,FILE *file,int error,DWORD last)
{
    printf("%s %u %d %d %d %d %d %d %d %lu\n",role,row,file->_flag,file->_file,file->_cnt,file->_bufsiz,file->_ptr==NULL,file->_base==NULL,error,last);
}
int main(int argc,char **argv)
{
    FILE *input,*output;char path[40];unsigned n;int error;DWORD last;unsigned char buffer[8];
#ifdef RECON_NATIVE_TEST
    if(argc>1 && !strcmp(argv[1],"prioralias")) CreateFileA("input.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if(argc>1 && !strcmp(argv[1],"priormap")) {
        HANDLE h=CreateFileA("input.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        CreateFileMappingA(h,NULL,PAGE_READONLY,0,0,NULL); CloseHandle(h);
    }
#endif
    for(n=0;n<4;++n) {
        errno=111;SetLastError(123);input=_fsopen("input.bin","rb",0x20);error=errno;last=GetLastError();if(!input)return 3;
        show("read",n,input,error,last);
#ifdef RECON_NATIVE_TEST
        if(argc>1 && !strcmp(argv[1],"alias")) _fsopen("input.bin","wb",0x10);
        if(argc>1 && !strcmp(argv[1],"mode")) _fsopen("input.bin","rt",0x20);
        if(argc>1 && !strcmp(argv[1],"share")) _fsopen("input.bin","rb",0x40);
        if(argc>1 && !strcmp(argv[1],"dup")) _dup(_fileno(input));
        if(argc>1 && !strcmp(argv[1],"dup2")) _dup2(_fileno(input),9);
        if(argc>1 && !strcmp(argv[1],"mapping")) CreateFileMappingA((HANDLE)_get_osfhandle(_fileno(input)),NULL,PAGE_READONLY,0,0,NULL);
        if(argc>1 && !strcmp(argv[1],"rawalias")) CreateFileA("input.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
#else
        (void)argc;(void)argv;
#endif
        sprintf(path,"output-%u.bin",n);errno=111;SetLastError(123);output=_fsopen(path,"wb",0x10);error=errno;last=GetLastError();if(!output)return 4;
        show("write",n,output,error,last);
        if(ftell(input)!=0 || ftell(output)!=0 || fread(buffer,1,8,input)!=8 || fwrite(buffer,1,8,output)!=8)return 5;
        if(fclose(input) || fclose(output))return 6;
    }
    return 0;
}
