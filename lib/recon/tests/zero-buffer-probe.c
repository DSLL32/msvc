/* Host-only construction of the captured internal FILE state, not target CRT
 * source. Ordinary fputc/fwrite/flush/close then exercise the real provider. */
#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <recon.h>
static void snapshot(unsigned char *data)
{ __asm__ volatile("fnsave (%0); frstor (%0)"::"r"(data):"memory"); }
static long physical(const char *path)
{ struct stat st;assert(!stat(path,&st));return st.st_size; }
int main(int argc,char **argv)
{
    char path[]="zero-buffer-XXXXXX",output[16],*base;
    int fd=mkstemp(path),result,text=argc>1 && !strcmp(argv[1],"text");
    ShimFILE *stream,foreign;unsigned char before[108],after[108];
    assert(fd>=0);close(fd);
    stream=recon_fopen(path,text?"wt":"wb");assert(stream);
    assert(!recon_setvbuf(stream,NULL,0,16));
    /* The observed old setvbuf rounds request1 to0. Preserve real owned backing
     * in this native control; the separate actual LINK packet binds malloc0. */
    stream->_bufsiz=0;base=stream->_base;memset(base,0x5a,16);
    if(argc>1){
        if(!strcmp(argv[1],"foreign")){foreign=*stream;stream=&foreign;}
        if(!strcmp(argv[1],"user"))stream->_flag=0x102;
        if(!strcmp(argv[1],"unowned"))stream->_flag=2;
        if(!strcmp(argv[1],"pointer")){stream->_ptr=base+2;stream->_cnt=-2;}
        if(!strcmp(argv[1],"count")){stream->_ptr=base+1;stream->_cnt=0;}
        if(!strcmp(argv[1],"descriptor"))stream->_file=-1;
    }
    errno=111;recon_doserrno=77;recon_SetLastError(0x12345678);
    snapshot(before);result=recon_fputc('\n',stream);snapshot(after);
    assert(result=='\n' && !memcmp(before,after,108));
    assert(errno==111 && recon_doserrno==77 && recon_GetLastError()==0x12345678);
    assert(stream->_ptr==base+1 && stream->_cnt==-1 && stream->_flag==0xa && *base=='\n');
    assert(physical(path)==0);
    errno=111;recon_doserrno=77;recon_SetLastError(0x12345678);
    snapshot(before);result=recon_fwrite("AB",1,2,stream);snapshot(after);
    assert(result==2 && !memcmp(before,after,108));
    assert(errno==111 && recon_doserrno==77 && recon_GetLastError()==0x12345678);
    assert(stream->_ptr==base+1 && stream->_cnt==-1 && stream->_flag==0xa && *base=='B');
    assert(physical(path)==(text?3:2));
    errno=111;recon_doserrno=77;recon_SetLastError(0x12345678);
    snapshot(before);result=recon_fflush(stream);snapshot(after);
    assert(!result && !memcmp(before,after,108));
    assert(errno==111 && recon_doserrno==77 && recon_GetLastError()==0x12345678);
    assert(stream->_ptr==base && !stream->_cnt && stream->_flag==0xa);
    assert(physical(path)==(text?4:3));
    assert(recon_flsbuf('C',stream)=='C');
    assert(stream->_ptr==base+1 && stream->_cnt==-1 && *base=='C');
    assert(physical(path)==(text?4:3));
    {unsigned i;for(i=1;i<16;++i)assert((unsigned char)base[i]==0x5a);}
    assert(!recon_fclose(stream) && !stream->_flag);
    fd=open(path,O_RDONLY);assert(fd>=0);result=read(fd,output,sizeof(output));close(fd);
    assert(result==(text?5:4) && !memcmp(output,text?"\r\nABC":"\nABC",result));
    assert(!unlink(path));return 0;
}
