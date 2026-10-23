/* Native provider initial-state and lifetime checks, not target application. */
#include <assert.h>
#include <recon.h>
int main(int argc,char **argv)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    ShimFILE saved;char *initial=stdin->_base;int duplicate;unsigned char bytes[4];
    assert(initial && stdin->_ptr==initial && !stdin->_cnt && stdin->_flag==0x101);
    assert(stdin->_file==0 && !stdin->_charbuf && stdin->_bufsiz==4096 && !stdin->_tmpfname);
    saved=*stdin;
    duplicate=_dup(0);assert(duplicate>=3 && !_close(duplicate));
    assert(!_dup2(1,2));assert(!memcmp(&saved,stdin,sizeof(saved)));
    assert(GetStdHandle(STD_INPUT_HANDLE)==(HANDLE)_get_osfhandle(0));
    assert(GetStdHandle(STD_ERROR_HANDLE)==(HANDLE)_get_osfhandle(2));
    if(argc>1 && !strcmp(argv[1],"close")) {
        assert(!fclose(stdin));assert(stdin->_flag==0 && stdin->_file==0);
        assert(stdin->_base==initial && stdin->_ptr==initial && stdin->_bufsiz==4096);
        assert(GetStdHandle(STD_INPUT_HANDLE)==NULL);
    } else {
        assert(freopen("replacement.bin","rb",stdin)==stdin);
        assert(stdin->_file==0 && stdin->_flag==1 && !stdin->_base && !stdin->_ptr && !stdin->_cnt);
        assert(GetStdHandle(STD_INPUT_HANDLE)==(HANDLE)_get_osfhandle(0));
        assert(fread(bytes,1,4,stdin)==4 && !memcmp(bytes,"data",4));
        assert(stdin->_base!=initial && (stdin->_flag&8));assert(!fclose(stdin));
    }
    /* Define these bytes here; no claim about prior buffer contents. */
    initial[0]=0x6b;initial[4095]=0x29;assert(initial[0]==0x6b && initial[4095]==0x29);
#else
    (void)argc;(void)argv;
    assert(stdin->_file==0 && stdin->_flag==1 && !stdin->_base && !stdin->_ptr && !stdin->_bufsiz);
#endif
    return 0;
}
