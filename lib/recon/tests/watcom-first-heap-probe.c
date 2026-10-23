#include <assert.h>
#include <malloc.h>
#include <recon.h>
static const char reference[]="a4f641651dfb1c1d4181a33042ad29eb3f00cdad45d291db02c128f94faae6d7";
static void snapshot(unsigned char *bytes)
{ __asm__ volatile("fnsave (%0); frstor (%0)"::"r"(bytes):"memory"); }
int main(int argc,char **argv)
{
    unsigned char before[108],after[108];void *pointer;int result;
    if(argc>1 && !strcmp(argv[1],"unbound"))return _heapchk();
    if(argc>1 && !strcmp(argv[1],"preactivity")){pointer=malloc(8);(void)pointer;}
    if(argc>1 && !strcmp(argv[1],"bad-reference"))recon_bind_watcom_wlib_first_main_heap("unknown");
    if(argc>1 && !strcmp(argv[1],"null-reference"))recon_bind_watcom_wlib_first_main_heap(NULL);
    errno=111;recon_doserrno=77;SetLastError(0x12345678);
    snapshot(before);recon_bind_watcom_wlib_first_main_heap(reference);snapshot(after);
    assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==77 && GetLastError()==0x12345678);
    assert(recon_watcom_first_main_heap.bound==1 && !recon_watcom_first_main_heap.clean && !recon_watcom_first_main_heap.calls && !recon_watcom_first_main_heap.target_activity);
    if(argc>1){
        if(!strcmp(argv[1],"rebind"))recon_bind_watcom_wlib_first_main_heap(reference);
        if(!strcmp(argv[1],"malloc")){pointer=malloc(16);assert(pointer);}
        if(!strcmp(argv[1],"calloc")){pointer=calloc(2,8);assert(pointer);}
        if(!strcmp(argv[1],"realloc")){pointer=realloc(NULL,16);assert(pointer);}
        if(!strcmp(argv[1],"free"))free(NULL);
        if(!strcmp(argv[1],"strdup")){pointer=strdup("owned");assert(pointer);}
    }
    errno=0;recon_doserrno=0;SetLastError(0x76543210);
    /* The original first row has an empty x87 stack. Native's own environment
     * is preserved exactly; no transported Windows pointers/payload are used. */
    snapshot(before);result=_heapchk();snapshot(after);
    assert(result==0 && !memcmp(before,after,108));
    assert(errno==0 && recon_doserrno==0 && GetLastError()==0x76543210);
    assert(recon_watcom_first_main_heap.bound==1 && recon_watcom_first_main_heap.clean==1 && recon_watcom_first_main_heap.calls==1 && !recon_watcom_first_main_heap.target_activity);
    assert(!strcmp(recon_watcom_first_main_heap.reference_sha256,reference));
    if(argc>1 && !strcmp(argv[1],"repeat"))return _heapchk();
    return 0;
}
