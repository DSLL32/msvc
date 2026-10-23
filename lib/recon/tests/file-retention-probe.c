#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static void snapshot(unsigned char *bytes)
{ __asm__ volatile("fnsave (%0); frstor (%0)"::"r"(bytes):"memory"); }
int main(void)
{
    ShimFILE *stream,*reopened;unsigned i;int capacities[]={4096,16,512};
    unsigned char before[108],after[108];char saved[sizeof(ShimFILE)];
    stream=recon_fopen("retained.bin","wb");assert(stream);
    assert(!stream->_base && !stream->_ptr && !stream->_cnt && !stream->_bufsiz && !stream->_charbuf);
    for(i=0;i<3;++i){
        assert(!recon_setvbuf(stream,NULL,0,capacities[i]));
        stream->_charbuf=0x12345678+(int)i;
        assert(recon_fputc('A'+i,stream)=='A'+(int)i);
        assert(!recon_fclose(stream));
        assert(!stream->_flag && !stream->_base && !stream->_ptr && !stream->_cnt);
        assert(stream->_bufsiz==capacities[i] && stream->_charbuf==0x12345678+(int)i);
        errno=111;recon_doserrno=77;recon_SetLastError(0x12345678);snapshot(before);
        reopened=recon_fopen("retained.bin","wb");snapshot(after);
        assert(reopened==stream && !memcmp(before,after,108));
        assert(!stream->_ptr && !stream->_base && !stream->_cnt && stream->_flag==2 && stream->_file>=3 && !stream->_tmpfname);
        assert(stream->_bufsiz==capacities[i] && stream->_charbuf==0x12345678+(int)i);
        assert(recon_tell(stream->_file)==0);
    }
    assert(!recon_fclose(stream));
    memcpy(saved,stream,sizeof(*stream));
    assert(!recon_fopen("missing-directory/entry","rb"));
    assert(!memcmp(saved,stream,sizeof(*stream))); /* Existing failed-open path. */
    reopened=recon_fopen("retained.bin","rb");assert(reopened==stream);
    assert(!stream->_ptr && !stream->_base && !stream->_cnt && stream->_flag==1);
    assert(stream->_bufsiz==512 && stream->_charbuf==0x1234567a);
    assert(!recon_fclose(stream));
    assert(!remove("retained.bin"));return 0;
}
