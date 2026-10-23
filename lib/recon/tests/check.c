#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <direct.h>
#include <recon.h>
/* Raw POSIX probes deliberately bypass the legacy descriptor aliases. */
#undef read
#undef write
#undef close
#undef unlink
typedef struct SortItem { int key,identity; } SortItem;
static int compare_item(const void *a,const void *b)
{
    const SortItem *left=a,*right=b;
    return left->key-right->key;
}
/* Exercise the implementation boundary with actual files, not mocked results. */
int main(void)
{
    char directory[4096],host[4096],dos[512],bytes[64];
    ShimFILE *stream; HANDLE file,mapping; char *view; DWORD count; int fd;
    long integer; short small; char remainder[16]; char *cwd;
    WCHAR wide[8],upper[8];
    SortItem items[10]; int i;
    assert(sizeof(IMAGE_DOS_HEADER)==64);
    assert(sizeof(IMAGE_FILE_HEADER)==20);
    assert(sizeof(IMAGE_OPTIONAL_HEADER)==224);
    assert(sizeof(IMAGE_SECTION_HEADER)==40);
    assert(sizeof(IMAGE_SYMBOL)==18 && sizeof(IMAGE_AUX_SYMBOL)==18);
    assert(sizeof(IMAGE_RELOCATION)==10 && sizeof(IMAGE_LINENUMBER)==6);
    assert(getenv("HOME"));
    assert(snprintf(directory,sizeof(directory),"%s/.ai-tmp/recon-check-XXXXXX",getenv("HOME"))<(int)sizeof(directory));
    assert(mkdtemp(directory));
    assert(setenv("RECON_DRIVE_N",directory,1)==0);
    strcpy(bytes,"a\\b\\c");
    assert(strcpy(bytes+1,bytes+2)==bytes+1 && !strcmp(bytes,"ab\\c"));
    cwd=getcwd(NULL,0); assert(cwd && chdir(directory)==0);
    assert(setenv("RECON_CANONICAL_CWD","N:\\",1)==0);
    assert(_fullpath(host,"folder\\..\\sample.txt",sizeof(host))==host);
    assert(!strcmp(host,"N:\\sample.txt"));
    assert(_getcwd(bytes,4)==bytes && !strcmp(bytes,"N:\\"));
    assert(mkdir("browser",0700)==0 && chdir("browser")==0);
    assert(setenv("RECON_CANONICAL_CWD","N:\\browser",1)==0);
    {
        char exact[12],small[10],*allocated;
        memset(exact,0x55,sizeof(exact));
        assert(_getcwd(exact,11)==exact && !strcmp(exact,"N:\\browser"));
        assert(exact[11]==0x55);
        memset(small,0x55,sizeof(small)); errno=0;
        assert(!_getcwd(small,sizeof(small)) && errno==ERANGE);
        for (i=0;i<(int)sizeof(small);++i) assert(small[i]==0x55);
        assert(!_getcwd(small,0) && errno==ERANGE);
        assert(!_getcwd(small,-1) && errno==ERANGE);
        allocated=_getcwd(NULL,0); assert(allocated && !strcmp(allocated,"N:\\browser")); free(allocated);
        allocated=_getcwd(NULL,-1); assert(allocated && !strcmp(allocated,"N:\\browser")); free(allocated);
        allocated=_getcwd(NULL,512); assert(allocated && !strcmp(allocated,"N:\\browser"));
        allocated[511]='x'; free(allocated);
    }
    assert(chdir("..")==0 && rmdir("browser")==0);
    assert(setenv("RECON_CANONICAL_CWD","N:\\",1)==0);
    assert(chdir(cwd)==0); free(cwd);
    assert(!_fullpath(host,"sample.txt",sizeof(host)) && errno==EINVAL);
    assert(!_getcwd(host,sizeof(host)) && errno==EINVAL);
    unsetenv("RECON_CANONICAL_CWD");
    assert(recon_host_path("N:\\sample.txt",host,sizeof(host))==0);
    assert(strstr(host,"sample.txt"));
    stream=fopen("N:\\sample.txt","wt"); assert(stream);
    assert(fprintf(stream,"one\ntwo\n")==8); assert(fclose(stream)==0);
    fd=open(host,O_RDONLY); assert(fd>=0); count=read(fd,bytes,sizeof(bytes)); close(fd);
    assert(count==10 && !memcmp(bytes,"one\r\ntwo\r\n",10));
    stream=fopen("N:\\sample.txt","rt"); assert(stream);
    assert(fgets(bytes,sizeof(bytes),stream) && !strcmp(bytes,"one\n"));
    assert(fclose(stream)==0);
    assert(_access("N:\\SAMPLE.TXT",0)==0);
    fd=_open("N:\\sample.txt",0x4000); assert(fd>=0);
    assert(_read(fd,bytes,4)==3 && !memcmp(bytes,"one",3));
    assert(_tell(fd)==4); /* trailing CR withheld; LF will start next fill */
    assert(_read(fd,bytes,4)==4 && !memcmp(bytes,"\ntwo",4));
    assert(recon_osfile[fd]&4); /* VC1's first-byte-LF positioning flag */
    assert(_close(fd)==0);
    assert(sscanf("4294967296","%ld",&integer)==1 && integer==0);
    assert(sscanf("65537","%hd",&small)==1 && small==1);
    assert(sscanf("0x100000001,tail","%li%s",&integer,remainder)==2);
    assert(integer==1 && !strcmp(remainder,",tail"));
    for (i=0;i<10;++i) { items[i].key=0; items[i].identity=i; }
    qsort(items,2,sizeof(*items),compare_item);
    assert(items[0].identity==1 && items[1].identity==0);
    for (i=0;i<10;++i) items[i].identity=i;
    qsort(items,10,sizeof(*items),compare_item);
    assert(items[0].identity==5 && items[5].identity==0);
    for (i=0;i<10;++i) items[i].key=9-i;
    qsort(items,10,sizeof(*items),compare_item);
    for (i=0;i<10;++i) assert(items[i].key==i);
    cwd=_fullpath(NULL,NULL,0); assert(cwd && strlen(cwd)>3); free(cwd);
    assert(_getcwd(host,sizeof(host))==host && host[0]=='Z' && host[1]==':');
    stream=fopen("N:\\compat.txt","wb"); assert(stream);
    assert(setvbuf(stream,NULL,0,1)==0);
    stream->_bufsiz=0; /* VC1 setvbuf rounds a one-byte buffer down to zero. */
    assert(fprintf(stream,"prefix%")==6);
    assert(fclose(stream)==0);
    stream=fopen("N:\\compat.txt","rb"); assert(stream);
    assert(fread(bytes,1,sizeof(bytes),stream)==6 && !memcmp(bytes,"prefix",6));
    assert(fclose(stream)==0);
    file=CreateFileA("N:\\mapped.bin",GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    assert(file!=INVALID_HANDLE_VALUE);
    mapping=CreateFileMappingA(file,NULL,PAGE_READWRITE,0,8192,NULL); assert(mapping);
    view=MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,0); assert(view);
    memcpy(view+4096,"mapped",6);
    assert(SetFilePointer(file,4102,NULL,FILE_BEGIN)==4102);
    assert(!SetEndOfFile(file) && GetLastError()==1224);
    assert(CloseHandle(mapping)); /* The view remains valid after handle close. */
    assert(!memcmp(view+4096,"mapped",6)); assert(UnmapViewOfFile(view));
    assert(SetFilePointer(file,4096,NULL,SEEK_SET)==4096);
    assert(ReadFile(file,bytes,6,&count,NULL) && count==6 && !memcmp(bytes,"mapped",6));
    assert(CloseHandle(file));
    file=CreateFileA("N:\\mapped.bin",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    assert(file!=INVALID_HANDLE_VALUE);
    assert(SetFilePointer(file,-1,NULL,FILE_BEGIN)==(DWORD)-1);
    assert(GetLastError()==ERROR_NEGATIVE_SEEK);
    recon_dosmaperr(GetLastError()); assert(errno==EINVAL);
    assert(CloseHandle(file));
    recon_fmode=0x8000;
    stream=fopen("N:\\binary-default.bin","w"); assert(stream);
    assert(fwrite("a\nb",1,3,stream)==3 && !fclose(stream));
    stream=fopen("N:\\binary-default.bin","rb"); assert(stream);
    assert(fread(bytes,1,sizeof(bytes),stream)==3 && !memcmp(bytes,"a\nb",3));
    assert(!fclose(stream) && !remove("N:\\binary-default.bin"));
    recon_fmode=0x4000;
    _splitpath("N:\\hello\\world.obj",dos,host,bytes,bytes+32);
    assert(!strcmp(dos,"N:") && !strcmp(host,"\\hello\\"));
    assert(!strcmp(bytes,"world") && !strcmp(bytes+32,".obj"));
    assert(!GetProcAddress(GetModuleHandleA("kernel32.dll"),"IsTNT"));
    assert(GetProcAddress(GetModuleHandleA("kernel32.dll"),"MapViewOfFile"));
    assert(!LoadLibraryA("unsupported.dll") && GetLastError()==126);
    assert(MultiByteToWideChar(1252,0,"a\x80",-1,wide,8)==3);
    assert(wide[0]=='a' && wide[1]==0x20ac && !wide[2]);
    assert(LCMapStringW(0,0x200,wide,-1,upper,8)==3);
    assert(upper[0]=='A' && upper[1]==0x20ac && !upper[2]);
    assert(WideCharToMultiByte(1252,0,upper,-1,bytes,sizeof(bytes),NULL,NULL)==3);
    assert(!memcmp(bytes,"A\x80",3));
    assert(wcscmp(wide,upper)>0 && wcscmp(wide,wide)==0);
    assert(setenv("RECON_FIXED_TIME","0",1)==0 && time(NULL)==0);
    file=CreateFileA("N:\\missing",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    assert(file==INVALID_HANDLE_VALUE && GetLastError()==ERROR_FILE_NOT_FOUND);
    /* A PE, shell script, or Wine launcher must never become a fallback. */
    stream=fopen("N:\\forbidden.exe","wb"); assert(stream);
    assert(fwrite("MZ",1,2,stream)==2); assert(fclose(stream)==0);
    { const char *argv[]={"forbidden.exe",NULL}; assert(_spawnve(0,"N:\\forbidden.exe",argv,NULL)==-1 && errno==ENOEXEC); }
    assert(remove("N:\\sample.txt")==0); assert(remove("N:\\mapped.bin")==0);
    assert(remove("N:\\forbidden.exe")==0); assert(remove("N:\\compat.txt")==0);
    assert(rmdir(directory)==0);
    return 0;
}
