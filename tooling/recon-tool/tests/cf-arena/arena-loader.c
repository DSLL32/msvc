/* Authored no-CRT loader fixture. No original Watcom payload is accepted here.
 * The observer stops at probe_ready before any allocated CF code executes.
 */
#include <windows.h>

const char probe_signature[16] = "WCF-AUTHORED-v1";
void * volatile probe_arena;
volatile DWORD probe_arena_size;
void (__cdecl * volatile probe_arena_entry)(void);
volatile DWORD probe_thread_marker;

DWORD WINAPI probe_unexpected_thread(void *unused)
{
    (void)unused;
    probe_thread_marker=0x11223344UL;
    return 0;
}

static void fail(DWORD status)
{
    ExitProcess(status);
}

static DWORD word(const BYTE *bytes, DWORD at)
{
    return (DWORD)bytes[at] | ((DWORD)bytes[at+1] << 8);
}

static DWORD dword(const BYTE *bytes, DWORD at)
{
    return word(bytes, at) | (word(bytes, at+2) << 16);
}

static int fixture_path(char *path)
{
    const char *p;
    DWORD n;
    int quoted;

    p=GetCommandLineA();
    while(*p==' ' || *p=='\t') ++p;
    quoted=(*p=='"');
    if(quoted) ++p;
    while(*p && (quoted ? *p!='"' : *p!=' ' && *p!='\t')) ++p;
    if(quoted) {
        if(*p!='"') fail(101);
        ++p;
    }
    while(*p==' ' || *p=='\t') ++p;
    quoted=(*p=='"');
    if(quoted) ++p;
    n=0;
    while(*p && (quoted ? *p!='"' : *p!=' ' && *p!='\t')) {
        if(n>=MAX_PATH-1) fail(102);
        path[n++]=*p++;
    }
    path[n]=0;
    if(!n) fail(103);
    if(quoted) {
        if(*p!='"') fail(104);
        ++p;
    }
    while(*p==' ' || *p=='\t') ++p;
    if(*p) {
        const char *mode="thread-negative";
        while(*mode && *p==*mode) {++p;++mode;}
        if(*mode) fail(105);
        while(*p==' ' || *p=='\t') ++p;
        if(*p) fail(105);
        return 1;
    }
    return 0;
}

/* /Od /Ob0 retains this public boundary. Its body only near-indirect CALLs
 * the published entry then loops; it does not execute CF during setup. */
void __cdecl probe_ready(void)
{
    probe_arena_entry();
    for(;;) {}
}

void __cdecl probe_loader_entry(void)
{
    BYTE header[128];
    char path[MAX_PATH];
    HANDLE file;
    BYTE *arena;
    DWORD got,high,length,load,size,relocation,allocation,entry;
    DWORD cursor,count,position,index,previous,fixups;
    int unexpected_thread;

    unexpected_thread=fixture_path(path);
    file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
    if(file==INVALID_HANDLE_VALUE) fail(106);
    high=0;
    length=GetFileSize(file,&high);
    if(high || length<128 || length>1024*1024) fail(107);
    got=0;
    if(!ReadFile(file,header,128,&got,0) || got!=128) fail(108);
    if(word(header,0)!=0x5a4d || word(header,8)!=4 || dword(header,64)!=0x4643) fail(109);
    load=dword(header,68);size=dword(header,72);relocation=dword(header,76);
    allocation=dword(header,80);entry=dword(header,84);
    if(load!=128 || size!=length-128 || size<6 || size>allocation ||
       allocation>1024*1024 || relocation<4 || entry>=relocation || relocation>=size) fail(110);
    arena=(BYTE *)VirtualAlloc(0,allocation,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!arena) fail(111);
    got=0;
    if(!ReadFile(file,arena,size,&got,0) || got!=size || !CloseHandle(file)) fail(112);
    cursor=relocation;previous=0;fixups=0;
    for(;;) {
        if(cursor>size-2) fail(113);
        count=word(arena,cursor);cursor+=2;
        if(!count) break;
        if(cursor>size-4 || count>8192-fixups) fail(114);
        position=(word(arena,cursor)<<16)|word(arena,cursor+2);cursor+=4;
        for(index=0;index<count;++index) {
            if(index) {
                DWORD delta;
                if(cursor>size-2) fail(115);
                delta=word(arena,cursor);cursor+=2;
                if(position>0xffffffffUL-delta) fail(116);
                position+=delta;
            }
            if(position>relocation-4 || (fixups && position<previous+4)) fail(117);
            *(DWORD *)(arena+position)+=(DWORD)arena;
            previous=position;++fixups;
        }
    }
    if(cursor!=size || !fixups) fail(118);
    if(!FlushInstructionCache(GetCurrentProcess(),arena,size)) fail(119);
    probe_arena=arena;
    probe_arena_size=allocation;
    probe_arena_entry=(void (__cdecl *)(void))(arena+entry);
    if(unexpected_thread) {
        HANDLE created=CreateThread(0,0,probe_unexpected_thread,0,0,0);
        if(!created) fail(121);
        if(WaitForSingleObject(created,INFINITE)!=WAIT_OBJECT_0) fail(123);
        if(!CloseHandle(created)) fail(122);
    }
    probe_ready();
    fail(120);
}

/* Public end anchor, never a runtime destination. */
void __cdecl probe_loader_end(void)
{
}
