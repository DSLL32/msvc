/* Observe original CL at its CreateProcessA import thunk. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>
static unsigned char buffer[65536];
static void save(char *root, int n, char *suffix, void *data, unsigned length)
{
    char name[1024]; FILE *f;
    sprintf(name,"%s\\spawn-%d.%s",root,n,suffix);
    f=fopen(name,"wb"); if(!f) exit(10);
    fwrite(data,1,length,f); fclose(f);
}
static unsigned read_string(HANDLE process, DWORD address)
{
    unsigned i; DWORD got;
    if(!address) return 0;
    for(i=0;i<sizeof(buffer)-1;++i) {
        if(!ReadProcessMemory(process,(void *)(address+i),buffer+i,1,&got)) exit(11);
        if(!buffer[i]) return i;
    }
    exit(12); return 0;
}
int main(int argc,char **argv)
{
    STARTUPINFOA si; PROCESS_INFORMATION pi; DEBUG_EVENT ev; CONTEXT ctx;
    DWORD base=0,site=0,wrote,status,stack[11],flags,code;
    BYTE original,trap=0xcc; int stepping=0,n=0; unsigned i; char command[2048];
    char prefix[1024],source[1024],dest[1024],*p,*q;
    static char *extensions[]={"ex","sy","in","gl","db",NULL};
    if(argc!=5 && argc!=7) return 2;
    SetEnvironmentVariableA("TMP",argv[4]); SetEnvironmentVariableA("TEMP",argv[4]);
    memset(&si,0,sizeof(si)); si.cb=sizeof(si);
    sprintf(command,"\"%s\" %s",argv[1],argc==7?argv[5]:"test.c");
    if(!CreateProcessA(argv[1],command,0,0,TRUE,DEBUG_ONLY_THIS_PROCESS,0,argv[4],&si,&pi)) return 3;
    for(;;) {
        if(!WaitForDebugEvent(&ev,30000)) return 4;
        status=DBG_CONTINUE;
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            base=(DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
            site=base+strtoul(argv[2],NULL,16);
            if(!ReadProcessMemory(pi.hProcess,(void *)site,&original,1,&wrote)) return 5;
            if(!WriteProcessMemory(pi.hProcess,(void *)site,&trap,1,&wrote)) return 6;
            FlushInstructionCache(pi.hProcess,(void *)site,1);
            if(ev.u.CreateProcessInfo.hFile) CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
        } else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=ev.u.Exception.ExceptionRecord.ExceptionCode;
            if(code==EXCEPTION_BREAKPOINT && (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress==site) {
                ++n; memset(&ctx,0,sizeof(ctx)); ctx.ContextFlags=CONTEXT_FULL;
                if(!GetThreadContext(pi.hThread,&ctx)) return 7;
                if(!ReadProcessMemory(pi.hProcess,(void *)ctx.Esp,stack,sizeof(stack),&wrote)) return 8;
                i=read_string(pi.hProcess,stack[1]); save(argv[3],n,"application",buffer,i);
                i=read_string(pi.hProcess,stack[2]); save(argv[3],n,"command",buffer,i);
                if(n==3) {
                    p=strchr((char *)buffer,'@'); if(!p) return 19;
                    strcpy(source,p+1);
                    sprintf(dest,"%s\\link-input.rsp",argv[3]);
                    if(!CopyFileA(source,dest,FALSE)) return 20;
                    sprintf(source,"%s\\%s",argv[4],argc==7?argv[6]:"test.obj");
                    sprintf(dest,"%s\\link-input.obj",argv[3]);
                    if(!CopyFileA(source,dest,FALSE)) return 21;
                    i=GetEnvironmentVariableA("LIB",(char *)buffer,sizeof(buffer));
                    save(argv[3],n,"LIB",buffer,i);
                }
                i=read_string(pi.hProcess,stack[8]); save(argv[3],n,"cwd",buffer,i);
                if(stack[7]) {
                    flags=stack[6]; if(flags&0x400) return 13;
                    for(i=0;i<sizeof(buffer)-1;++i) {
                        if(!ReadProcessMemory(pi.hProcess,(void *)(stack[7]+i),buffer+i,1,&wrote)) return 14;
                        if(i && !buffer[i] && !buffer[i-1]) break;
                    }
                    save(argv[3],n,"environment",buffer,i+1);
                    if(n==2) {
                        p=(char *)buffer;
                        while(*p && strncmp(p,"MSC_CMD_FLAGS=",14)) p+=strlen(p)+1;
                        p=strstr(p,"-il "); if(!p) return 15; p+=4;
                        q=strchr(p,' '); if(!q) return 16;
                        memcpy(prefix,p,q-p); prefix[q-p]=0;
                        for(i=0;extensions[i];++i) {
                            sprintf(source,"%s%s",prefix,extensions[i]);
                            sprintf(dest,"%s\\c2-input.%s",argv[3],extensions[i]);
                            if(!CopyFileA(source,dest,FALSE)) {
                                /* The debug intermediate is absent without /Z7.
                                   The caller checks the complete expected set. */
                                if(strcmp(extensions[i],"db") ||
                                    GetLastError()!=ERROR_FILE_NOT_FOUND) return 17;
                            }
                        }
                    }
                }
                WriteProcessMemory(pi.hProcess,(void *)site,&original,1,&wrote);
                FlushInstructionCache(pi.hProcess,(void *)site,1);
                ctx.Eip=site;ctx.EFlags|=0x100;
                if(!SetThreadContext(pi.hThread,&ctx)) return 9;
                stepping=1;
            } else if(code==EXCEPTION_SINGLE_STEP && stepping) {
                memset(&ctx,0,sizeof(ctx));ctx.ContextFlags=CONTEXT_FULL;
                GetThreadContext(pi.hThread,&ctx);ctx.EFlags&=~0x100;SetThreadContext(pi.hThread,&ctx);
                WriteProcessMemory(pi.hProcess,(void *)site,&trap,1,&wrote);
                FlushInstructionCache(pi.hProcess,(void *)site,1);stepping=0;
            } else if(code!=EXCEPTION_BREAKPOINT) {
                fprintf(stderr,"exception %08lx at %p\n",code,ev.u.Exception.ExceptionRecord.ExceptionAddress);
                return 18;
            }
        } else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            printf("exit %lu; captured %d launches\n",ev.u.ExitProcess.dwExitCode,n);
            ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,status);
            return ev.u.ExitProcess.dwExitCode;
        }
        ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,status);
    }
}
