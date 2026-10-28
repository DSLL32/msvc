#!/usr/bin/env python3
"""Compile the actual broker patch/add/consume functions against fake Win32 IO.

No debuggee or Wine: this exercises broker source transitions and failures;
the separately compiled Win32 helper verifies the real platform declarations.
"""
from pathlib import Path
import subprocess
import tempfile
import time

package = Path(__file__).resolve().parents[1]
source = (package / 'wine/cf-broker.c').read_text()


def function(name):
    # All selected definitions have a unique name and a top-level closing brace.
    signature = next(line for line in source.splitlines() if line.startswith('static ') and
                     (' ' + name + '(') in line)
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        if source[end] == '{': depth += 1
        if source[end] == '}': depth -= 1
        end += 1
    return source[start:end]


header = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef struct {
    DWORD ContextFlags,Eip,Eax,Ecx,Edx,Ebx,Esp,Ebp,Esi,Edi,EFlags;
    DWORD SegCs,SegSs,SegDs,SegEs,SegFs,SegGs;
} CONTEXT;
typedef struct Trap { DWORD address; BYTE saved; int active; } Trap;
struct { DWORD dwThreadId,dwDebugEventCode;
    struct { struct { struct { DWORD ExceptionCode; void *ExceptionAddress; } ExceptionRecord;
                     DWORD dwFirstChance; } Exception; } u; } event;
enum { MAX_READ=512,MAX_TRAPS=4,CONTEXT_FULL=1,PAGE_EXECUTE_READWRITE=0x40,
       EXCEPTION_DEBUG_EVENT=1,EXCEPTION_BREAKPOINT=0x80000003UL };
static int child=1,thread=1,pending=1,entry_reached=1,ordinary=1;
static DWORD child_tid=8,image_base=0x1000,image_size=256,arena_base=0x2000,arena_size=256;
static DWORD entry_address=0x1000,entry_byte=0x90;
static Trap traps[MAX_TRAPS];static unsigned trap_count;
static CONTEXT current;
static BYTE bytes[256];
static FILE *log_file;
static int fail_write,fail_readback,reads,writes;
static int ReadProcessMemory(int h,void *address,void *out,DWORD size,DWORD *got) {
    DWORD at=(DWORD)(uintptr_t)address-image_base;(void)h;
    if(at+size>sizeof(bytes)) return 0;
    memcpy(out,bytes+at,size);*got=size;++reads;
    if(fail_readback && writes) *(BYTE *)out^=1;
    return 1;
}
static int WriteProcessMemory(int h,void *address,const void *data,DWORD size,DWORD *done) {
    DWORD at=(DWORD)(uintptr_t)address-image_base;(void)h;
    if(fail_write || at+size>sizeof(bytes)) return 0;
    memcpy(bytes+at,data,size);*done=size;++writes;return 1;
}
static int VirtualProtectEx(int h,void *address,DWORD size,DWORD flags,DWORD *old) {
    (void)h;(void)address;(void)size;(void)flags;*old=0x20;return 1;
}
static int FlushInstructionCache(int h,void *address,DWORD size) {
    (void)h;(void)address;(void)size;return 1;
}
static int GetThreadContext(int h,CONTEXT *out) {(void)h;*out=current;return 1;}
static int SetThreadContext(int h,CONTEXT *in) {(void)h;current=*in;return 1;}
'''
body = '\n'.join(function(name) for name in ('bounds','memory','context','pack_context',
        'same_context','patch','in_image','in_arena','add_trap','consume'))
main = r'''
int main(void) {
    DWORD index,again;unsigned before;int old_writes;
    log_file=fopen("/dev/null","w");assert(log_file);
    memset(bytes,0x90,sizeof(bytes));current.Eip=0x1100;
    event.dwThreadId=8;event.dwDebugEventCode=EXCEPTION_DEBUG_EVENT;
    event.u.Exception.ExceptionRecord.ExceptionCode=EXCEPTION_BREAKPOINT;
    event.u.Exception.dwFirstChance=1;
    assert(add_trap(0x1020,0x90,&index));assert(index==0 && trap_count==1 && reads==2);
    old_writes=writes;assert(!add_trap(0x1020,0x90,&again));assert(writes==old_writes);
    current.Eip=0x1021;event.u.Exception.ExceptionRecord.ExceptionAddress=(void *)0x1020;
    current.Esp=0x2000;current.Ebp=0x2100;
    assert(consume(index));assert(current.Eip==0x1020 && current.Esp==0x2000 && current.Ebp==0x2100);
    assert(!traps[index].active && bytes[32]==0x90);
    assert(!add_trap(0x1020,0x90,&again)); /* cannot rearm from consumed PC */
    current.Eip=0x1021;assert(!add_trap(0x1020,0x90,&again));
    current.Eip=0x1025;
    old_writes=writes;assert(!add_trap(0x1020,0x91,&again));assert(writes==old_writes);
    bytes[32]=0x91;assert(!add_trap(0x1020,0x90,&again));assert(writes==old_writes);bytes[32]=0x90;
    ordinary=0;assert(!add_trap(0x1020,0x90,&again));ordinary=1;
    current.EFlags=0x100;assert(!add_trap(0x1020,0x90,&again));current.EFlags=0;
    trap_count=MAX_TRAPS;before=trap_count;assert(add_trap(0x1020,0x90,&again));
    assert(index==again && trap_count==before && traps[index].active && bytes[32]==0xcc);
    current.Eip=0x1021;assert(consume(index));current.Eip=0x1025;
    fail_write=1;assert(!add_trap(0x1020,0x90,&again));assert(!traps[index].active);fail_write=0;
    fail_readback=1;writes=0;assert(!add_trap(0x1020,0x90,&again));assert(!traps[index].active);
    fclose(log_file);return 0;
}
'''
scratch = Path.home() / '.ai-tmp/msvc2'
scratch.mkdir(parents=True, exist_ok=True)
start = time.monotonic()
with tempfile.TemporaryDirectory(prefix='cf-trap-controls-', dir=scratch) as folder:
    root = Path(folder)
    program = root / 'controls'
    c = root / 'controls.c'
    c.write_text(header + body + main)
    subprocess.run(['cc','-Wall','-Wextra','-Werror',str(c),'-o',str(program)],check=True)
    subprocess.run([str(program)],check=True)
print('Actual broker patch/consume/rearm source controls PASS (%.3f ms)' %
      ((time.monotonic()-start)*1000))
