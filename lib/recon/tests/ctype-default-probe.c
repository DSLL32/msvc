/* Native comparison with encountered shipped static/default-C table hashes. */
#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
int main(void)
{
    unsigned short *initial=recon_pctype;
    int character,mask;FILE *file;
    assert(initial[-1]==0 && initial['\t']==0x28 && initial[' ']==0x48);
    assert(recon_isctype('\t',0x40)==0 && recon_isctype('\t',0x157)==0);
    assert(recon_isctype('\t',8)==8 && recon_isctype('\t',0x20)==0x20);
    for(character=-1;character<256;++character)for(mask=0;mask<512;++mask){
        errno=111;recon_doserrno=77;recon_SetLastError(0x12345678);
        assert(recon_isctype(character,mask)==(initial[character]&mask));
        assert(errno==111 && recon_doserrno==77 && recon_GetLastError()==0x12345678);
        assert(recon_pctype==initial);
    }
    file=fopen("ctype.bin","wb");assert(file);
    assert(fwrite(initial-1,2,257,file)==257);assert(!fclose(file));
    return 0;
}
