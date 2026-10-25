#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <recon.h>
typedef struct PublicResult {
    unsigned handle,error,saved_errno,doserrno;
    unsigned attempted,query_result,query_error;
    unsigned close_result,close_error,close_errno,close_doserrno;
    CONSOLE_SCREEN_BUFFER_INFO screen;
} PublicResult;
typedef char public_result_size[sizeof(PublicResult)==68?1:-1];
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    PublicResult result;
    unsigned char before[108],after[108],flags[256];ShimFILE streams[20];
    DWORD access=GENERIC_READ|GENERIC_WRITE,share=0,disposition=OPEN_EXISTING,attributes=0;
    SECURITY_ATTRIBUTES security;LPSECURITY_ATTRIBUTES security_pointer=NULL;
    HANDLE handle,template_handle=NULL;const char *name="CONOUT$",*mode=argc>1?argv[1]:"";
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);
    if(!strcmp(mode,"access"))access=GENERIC_READ;
    if(!strcmp(mode,"share"))share=FILE_SHARE_READ;
    if(!strcmp(mode,"security"))security_pointer=&security;
    if(!strcmp(mode,"disposition"))disposition=OPEN_ALWAYS;
    if(!strcmp(mode,"attributes"))attributes=FILE_ATTRIBUTE_NORMAL;
    if(!strcmp(mode,"template"))template_handle=(HANDLE)1;
    if(!strcmp(mode,"std-handle"))assert(recon_SetStdHandle(STD_OUTPUT_HANDLE,(HANDLE)4));
    if(!strcmp(mode,"ordinary"))name="plain";
    if(!strcmp(mode,"missing"))name="absent";
    memcpy(streams,recon_iob,sizeof(streams));memcpy(flags,recon_osfile,sizeof(flags));
    memset(&result,0,sizeof(result));memset(&result.screen,0xa5,sizeof(result.screen));
    if(!strcmp(mode,"live"))__asm__ __volatile__("fld1\n\tfldpi":::"st");
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    fp(before);handle=recon_CreateFileA(name,access,share,security_pointer,
                                      disposition,attributes,template_handle);fp(after);
    result.handle=(unsigned)handle;result.error=recon_GetLastError();
    result.saved_errno=errno;result.doserrno=recon_doserrno;
    assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==222);
    assert(!memcmp(streams,recon_iob,sizeof(streams)) && !memcmp(flags,recon_osfile,sizeof(flags)));
    if(!strcmp(mode,"ordinary")) {
        assert(handle!=INVALID_HANDLE_VALUE && result.error==0);assert(recon_CloseHandle(handle));return 0;
    }
    assert(handle==INVALID_HANDLE_VALUE);
    if(!strcmp(mode,"missing")){assert(result.error==ERROR_FILE_NOT_FOUND);return 0;}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    assert(result.error==ERROR_INVALID_HANDLE);
#else
    assert(result.error==ERROR_FILE_NOT_FOUND);
#endif
    if(!strcmp(mode,"live"))__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    assert(write(1,&result,sizeof(result))==sizeof(result));return 0;
}
