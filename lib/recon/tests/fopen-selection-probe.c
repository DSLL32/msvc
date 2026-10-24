#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/stat.h>
#include <recon.h>
static void fp(unsigned char b[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])b)); }
static char old_name[]="retained-temp-name";
int main(int argc,char **argv)
{
    ShimFILE *stream=&recon_iob[3],expected,neighbor,direct;
    unsigned char before[108],after[108],osfile[256];char storage[32];
    assert(!mkdir("parent",0700));
    memset(storage,0x7a,sizeof(storage));memset(stream,0,sizeof(*stream));
    /* Genuine registered free-slot storage with retained fields. Selection
     * owns no buffer or tmp-name release on this failure path. */
    stream->_ptr=storage+7;stream->_base=storage;stream->_cnt=9;stream->_file=123;
    stream->_charbuf=0x12345678;stream->_bufsiz=4096;stream->_tmpfname=old_name;
    expected=*stream;expected._ptr=expected._base=NULL;expected._cnt=expected._flag=0;expected._file=-1;
    neighbor=recon_iob[4];memcpy(osfile,recon_osfile,256);
    if(argc>1) __asm__ __volatile__("fld1\n\tfldpi":::"st");
    errno=0;recon_doserrno=0;recon_SetLastError(0);
    fp(before);assert(!recon_fopen("absent/log.txt","w"));fp(after);
    assert(errno==2 && recon_doserrno==3 && recon_GetLastError()==3 && !memcmp(before,after,108));
    assert(!memcmp(stream,&expected,sizeof(expected)) && !memcmp(&neighbor,&recon_iob[4],sizeof(neighbor)) && !memcmp(osfile,recon_osfile,256));
    assert(!memcmp(storage,"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",32));
    /* A direct openfile failure has no selection and writes no FILE bytes. */
    direct=*stream;direct._ptr=storage+2;direct._base=storage;direct._cnt=7;direct._flag=0x40;direct._file=77;
    expected=direct;errno=0;recon_doserrno=0;recon_SetLastError(0);
    fp(before);assert(!recon_openfile("absent/log.txt","w",0x40,&direct));fp(after);
    assert(!memcmp(&direct,&expected,sizeof(direct)) && !memcmp(before,after,108));
    assert(errno==2 && recon_doserrno==3 && recon_GetLastError()==3);
    errno=0;recon_doserrno=0;recon_SetLastError(0);
    fp(before);assert(recon_fopen("parent/new","w")==stream);fp(after);
    assert(!memcmp(before,after,108) && stream->_file==3 && stream->_flag==2);
    assert(stream->_charbuf==0x12345678 && stream->_bufsiz==4096 && !stream->_tmpfname);
    assert(!memcmp(&neighbor,&recon_iob[4],sizeof(neighbor)));
    assert(!recon_fclose(stream));
    if(argc>1) __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    puts("fopen selected-slot/retained-fields/neighbors/lowio; direct-openfile no-write; success/errors/full108FP PASS");return 0;
}
