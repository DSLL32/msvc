#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <recon.h>
static FILE *output;
static void emit(const char *name)
{
    fprintf(stderr,"%s pending=%d size=%ld\n",name,(int)(output->_ptr-output->_base),_filelength(_fileno(output)));
    fputs("after",output); fputs(name,output);
}
static int first(void) { emit("A"); return 77; }
static void middle(void) { emit("B"); }
static int last(void) { emit("C"); return 99; }
int main(int argc,char **argv)
{
#ifdef RECON_NATIVE_TEST
    if(argc>1 && !strcmp(argv[1],"null")) _onexit(NULL);
    if(argc>1 && !strcmp(argv[1],"dll")) __dllonexit(first,NULL,NULL);
#endif
    output=fopen("exit-output.bin","wb"); if(!output) return 3;
    fputs("before",output);
    if(_onexit(first)!=first || atexit(middle) || _onexit(last)!=last) return 4;
    if(argc>1 && !strcmp(argv[1],"process")) ExitProcess(1100);
    if(argc>1 && !strcmp(argv[1],"return")) return 37;
    exit(1100);
    return 5;
}
