#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static unsigned char input[9000];
static void fp(unsigned char bytes[108])
{ __asm__ volatile("fnsave (%0); frstor (%0)"::"r"(bytes):"memory"); }
static int refill(ShimFILE *stream)
{
    unsigned char before[108],after[108];int result;
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    fp(before);result=recon_filbuf(stream);fp(after);
    assert(!memcmp(before,after,108));
    /* Existing ReadFile success publishes LastError0; capacity publication must
     * preserve the remainder of that established lowio result. */
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==0);
    return result;
}
static ShimFILE *fresh(void)
{
    ShimFILE *s=recon_fopen("input.bin","rb");assert(s);
    assert(refill(s)==input[0]);
    assert(s->_flag==9 && s->_bufsiz==4096 && s->_cnt==4095 && s->_ptr==s->_base+1);
    assert(recon_tell(s->_file)==4096);return s;
}
static void seek_reset(ShimFILE *s,long offset)
{
    assert(recon_lseek(s->_file,offset,SEEK_SET)==offset);
    s->_ptr=s->_base;s->_cnt=-1;s->_bufsiz=512;
}
int main(int argc,char **argv)
{
    ShimFILE *s,other;FILE *file;unsigned n;char *base,user[4096];
    (void)argv;
    for(n=0;n<sizeof(input);++n)input[n]=(unsigned char)(n%251+1);
    file=fopen("input.bin","wb");assert(file);
    assert(fwrite(input,1,sizeof(input),file)==sizeof(input) && !fclose(file));
    if(argc>1)__asm__ volatile("fld1":::"st");
    s=fresh();base=s->_base;memset(base,0xcc,4096);seek_reset(s,0);
    assert(refill(s)==input[0]);
    assert(s->_bufsiz==4096 && s->_base==base && s->_ptr==base+1 && s->_cnt==511 && s->_flag==9);
    assert(recon_tell(s->_file)==512 && !memcmp(base,input,512));
    for(n=512;n<4096;++n)assert((unsigned char)base[n]==0xcc);
    for(n=1;n<512;++n)assert(recon_fgetc(s)==input[n]);
    assert(refill(s)==input[512]);
    assert(s->_cnt==4095 && s->_bufsiz==4096 && recon_tell(s->_file)==4608);
    assert(!memcmp(base,input+512,4096));assert(!recon_fclose(s));
    /* A real512-byte owned allocation must not acquire an unsafe capacity. */
    s=recon_fopen("input.bin","rb");assert(s);
    assert(!recon_setvbuf(s,NULL,0,512));assert(refill(s)==input[0]);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    assert(s->_flag==0x409);
#else
    assert(s->_flag==9);
#endif
    assert(s->_bufsiz==512 && s->_cnt==511 && recon_tell(s->_file)==512);assert(!recon_fclose(s));
    /* User ownership and the unqualified0x400 domain retain their behavior. */
    s=recon_fopen("input.bin","rb");assert(s);assert(!recon_setvbuf(s,user,0,512));
    assert(refill(s)==input[0] && s->_bufsiz==512 && s->_base==user);assert(!recon_fclose(s));
    s=fresh();seek_reset(s,0);s->_flag|=0x400;
    assert(refill(s)==input[0] && s->_bufsiz==512);assert(!recon_fclose(s));
    /* Default correction is registered-owner only;510 keeps its old branch. */
    s=fresh();seek_reset(s,0);other=*s;
    assert(refill(&other)==input[0]);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    assert(other._bufsiz==4096);
#else
    assert(other._bufsiz==512);
#endif
    assert(!recon_fclose(s));
    /* EOF and error exits must not publish successful-refill capacity. */
    s=fresh();seek_reset(s,sizeof(input));
    assert(refill(s)==EOF && s->_bufsiz==512 && !s->_cnt && (s->_flag&0x10));assert(!recon_fclose(s));
    s=fresh();seek_reset(s,0);assert(!recon_close(s->_file));
    assert(recon_filbuf(s)==EOF && s->_bufsiz==512 && !s->_cnt && (s->_flag&0x20));
    recon_freebuf(s);s->_flag=0;
    if(argc>1)__asm__ volatile("fstp %%st(0)":::"st");
    assert(!remove("input.bin"));puts("temporary512 refill, owned4096 promotion and scope controls pass");return 0;
}
