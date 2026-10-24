#!/usr/bin/env python3
"""Compile the actual save_file body against drifting-cursor/fault API mocks."""
import ctypes,json,subprocess,tempfile
from pathlib import Path
project=Path(__file__).resolve().parents[1];source=(project/'scripts/reference-debugger.c').read_text();a=source.index('static void save_file(');b=source.index('#include "export-reader.h"',a)
body=source[a:b];assert 'SetFilePointer'not in body and 'ReadFile(source,buffer,bytes,&got,&at)'in body
mock=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
typedef uint32_t DWORD;typedef unsigned char BYTE;typedef void* HANDLE;
typedef struct { DWORD Internal,InternalHigh,Offset,OffsetHigh;HANDLE hEvent; } OVERLAPPED;
#define MAX_PATH 260
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define INVALID_FILE_SIZE 0xffffffffU
#define GENERIC_WRITE 1
#define CREATE_NEW 1
static jmp_buf failure;static FILE*events;static int fault,size_calls;static DWORD written;
DWORD offsets[16],reads,shared_cursor;
static void ExitProcess(unsigned n){longjmp(failure,n);}
static void output_path(char*out,const char*name){strcpy(out,name);}
static DWORD GetFileSize(HANDLE file,DWORD*high){++size_calls;*high=fault==1;return fault==2?0xffffffffU:fault==3?0:fault==4?67108865U:(fault==5&&size_calls>1?50001:50000);}
static HANDLE CreateFileA(const char*p,DWORD a,DWORD s,void*x,DWORD c,DWORD f,HANDLE t){return fault==8?INVALID_HANDLE_VALUE:(HANDLE)2;}
static int ReadFile(HANDLE file,void*out,DWORD n,DWORD*got,OVERLAPPED*at){
 DWORD i;if(file!=(HANDLE)1||!at||at->OffsetHigh||at->Internal||at->InternalHigh||at->hEvent||reads>=16)ExitProcess(200);
 offsets[reads++]=at->Offset;shared_cursor=123456+reads*17;
 if(at->Offset!=written||at->Offset+n>50000)ExitProcess(201);
 for(i=0;i<n;++i)((BYTE*)out)[i]=(BYTE)(at->Offset+i);
 *got=fault==6?n-1:n;return fault!=7;
}
static int WriteFile(HANDLE file,const void*data,DWORD n,DWORD*wrote,void*at){
 DWORD i;if(file!=(HANDLE)2||at)ExitProcess(202);
 for(i=0;i<n;++i)if(((const BYTE*)data)[i]!=(BYTE)(written+i))ExitProcess(203);
 *wrote=fault==9?n-1:n;written+=n;return 1;
}
static void CloseHandle(HANDLE h){}
static int helper_log(FILE*f,const char*format,const char*name,DWORD count){if(count!=50000)ExitProcess(204);return 0;}
#define fprintf helper_log
'''
wrapper=r'''
int run(int mode){int status;fault=mode;size_calls=0;written=0;reads=0;shared_cursor=987654;memset(offsets,0,sizeof(offsets));events=tmpfile();status=setjmp(failure);if(!status)save_file(mode==10?NULL:(HANDLE)1,"loaded.bin");fclose(events);return status;}
'''
with tempfile.TemporaryDirectory(prefix='hfile-save-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as tmp:
 p=Path(tmp);(p/'mock.c').write_text(mock+body+wrapper);subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror','-Wno-unused-parameter',str(p/'mock.c'),'-o',str(p/'mock.so')],check=True)
 library=ctypes.CDLL(str(p/'mock.so'));assert library.run(0)==0
 assert ctypes.c_uint.in_dll(library,'reads').value==4
 assert list((ctypes.c_uint*16).in_dll(library,'offsets'))[:4]==[0,16384,32768,49152]
 controls=[]
 for fault,status,label in [(1,22,'high file size'),(2,22,'invalid size'),(3,22,'empty file'),(4,22,'size cap'),(5,22,'size changed'),(6,24,'partial read'),(7,24,'failed read'),(8,23,'output creation failure'),(9,25,'partial write'),(10,21,'invalid actual handle')]:
  assert library.run(fault)==status;controls.append(label)
print(json.dumps({'passed':True,'negative_controls':len(controls),'positive':'Exact50000B through four explicit offsets while shared cursor drifts','controls':controls,'scope':'Actual helper body, mock API logic only; next required capture verifies loaded hashes'}))
