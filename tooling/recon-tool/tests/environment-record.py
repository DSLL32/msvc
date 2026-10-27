#!/usr/bin/env python3
"""Native mocked I/O controls for the broker's exact prelaunch record function."""
import json
from pathlib import Path
import subprocess
import tempfile
import time

TOOL = Path(__file__).resolve().parents[1]
base = Path.home() / '.ai-tmp/msvc2/os-environment-diagnostics'
base.mkdir(parents=True, exist_ok=True)
root = Path(tempfile.mkdtemp(prefix='controls-', dir=base))
source = (TOOL / 'wine/broker.c').read_text()
record = source[source.index('static void environment_record_io('):source.index('\nint main(')]
prefix = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "environment.h"
typedef unsigned long DWORD;
typedef int HANDLE;
#define INVALID_HANDLE_VALUE (-1)
#define INVALID_FILE_SIZE 0xffffffffUL
#define GENERIC_READ 1
#define GENERIC_WRITE 2
#define FILE_SHARE_READ 1
#define OPEN_EXISTING 1
#define CREATE_NEW 2
#define ERROR_FILE_NOT_FOUND 2
static const char *expected;
static DWORD expected_size, last_error;
static int mode, creates;
static DWORD GetLastError(void) { return last_error; }
static HANDLE CreateFileA(const char *p,int a,int b,void *c,int disposition,int d,void *e)
{
    (void)p;(void)a;(void)b;(void)c;(void)d;(void)e;
    if(disposition==OPEN_EXISTING) {
        if(mode==1){last_error=5;return INVALID_HANDLE_VALUE;}
        if(!expected){last_error=2;return INVALID_HANDLE_VALUE;}
        return 1;
    }
    ++creates;
    if(mode==4){last_error=80;return INVALID_HANDLE_VALUE;}
    return 2;
}
static DWORD GetFileSize(HANDLE h,void *unused)
{
    (void)h;(void)unused;
    if(mode==2){last_error=87;return INVALID_FILE_SIZE;}
    return expected_size;
}
static int ReadFile(HANDLE h,void *bytes,DWORD n,DWORD *got,void *unused)
{
    (void)h;(void)unused;
    if(mode==3){last_error=23;*got=0;return 0;}
    *got=n-(mode==6);memcpy(bytes,expected,*got);return 1;
}
static int WriteFile(HANDLE h,const void *bytes,DWORD n,DWORD *got,void *unused)
{
    (void)h;(void)bytes;(void)unused;
    if(mode==5){last_error=29;*got=0;return 0;}
    *got=n-(mode==7);return 1;
}
static int CloseHandle(HANDLE h) { (void)h;last_error=9999;return 1; }
'''
main = r'''
int main(int argc,char **argv)
{
    static const char same[]="A=SECRET\0Z=SECRET\0\0";
    static const char changed[]="A=SECREX\0Z=SECRET\0\0";
    static const char inserted[]="A=SECRET\0B=SECRET\0Z=SECRET\0\0";
    static const char longer[]="A=LONGSECRET\0Z=SECRET\0\0";
    char long_expected[320],long_actual[320];
    const char *actual=same;
    int ok,success=0;
    (void)original_environment_merge;
    assert(argc==2);expected=same;expected_size=sizeof(same)-1;
    if(!strcmp(argv[1],"same"))success=1;
    else if(!strcmp(argv[1],"undeclared")){expected=NULL;success=1;}
    else if(!strcmp(argv[1],"changed"))actual=changed;
    else if(!strcmp(argv[1],"inserted"))actual=inserted;
    else if(!strcmp(argv[1],"deleted")){expected=inserted;expected_size=sizeof(inserted)-1;}
    else if(!strcmp(argv[1],"length"))actual=longer;
    else if(!strcmp(argv[1],"long-name")){
        memset(long_expected,'N',300);memcpy(long_expected+300,"=SECRET\0\0",9);
        memcpy(long_actual,long_expected,309);long_actual[306]='X';
        expected=long_expected;expected_size=309;actual=long_actual;
    }
    else mode=atoi(argv[1]);
    ok=record_process_environment("receipt",actual);
    assert(ok==success);
    assert(creates==(success || mode==4 || mode==5 || mode==7));
    return 0;
}
'''
probe = root / 'probe.c'
probe.write_text(prefix + record + main)
results = []
for cc in ['gcc', 'clang']:
    binary = root / cc
    start = time.monotonic()
    build = [cc, '-m32', '-std=c89', '-pedantic', '-Wall', '-Wextra', '-Werror',
             '-I' + str(TOOL / 'wine'), str(probe), '-o', str(binary)]
    subprocess.run(build, check=True)
    for case in ['same', 'undeclared', 'changed', 'inserted', 'deleted', 'length',
                 'long-name', '1', '2', '3', '4', '5', '6', '7']:
        p = subprocess.run([binary, case], capture_output=True, text=True)
        (root / (cc + '-' + case + '.log')).write_text(p.stdout + p.stderr)
        assert p.returncode == 0, (cc, case, p.stderr)
        assert 'SECRET' not in p.stdout + p.stderr, (cc, case)
        if case in ['same', 'undeclared']:
            assert not p.stdout and not p.stderr
        elif case in ['changed', 'inserted', 'deleted', 'length', 'long-name']:
            assert 'mismatch:' in p.stderr
            offset = dict(changed=7, inserted=9, deleted=9, length=2, **{'long-name':306})[case]
            assert 'first_offset=' + str(offset) + ' ' in p.stderr
            if case == 'changed': assert 'expected_name=A ' in p.stderr and 'actual_name=A ' in p.stderr
            if case == 'inserted': assert 'expected_name=Z ' in p.stderr and 'actual_name=B ' in p.stderr
            if case == 'deleted': assert 'expected_name=B ' in p.stderr and 'actual_name=Z ' in p.stderr
            if case == 'long-name':
                assert 'expected_name_bytes=300 ' in p.stderr and 'N'*129 not in p.stderr
        else:
            operation, error = {'1':('expected-open',5),'2':('expected-size',87),
                '3':('expected-read',23),'4':('receipt-create',80),
                '5':('receipt-write',29),'6':('expected-read',0),
                '7':('receipt-write',0)}[case]
            assert operation + ' failed:' in p.stderr and 'win32=' + str(error) + ' ' in p.stderr
    results.append(dict(compiler=cc, build=build, cases=14, seconds=time.monotonic()-start))
(root / 'receipt.json').write_text(json.dumps(results, indent=2) + '\n')
print('PASS exact broker record: 28 mismatch/success/I/O controls; no values logged')
print('Results:', root)
