/* Original-only capture diagnostic; never used by fixture runners. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
 STARTUPINFOA si; PROCESS_INFORMATION pi; SECURITY_ATTRIBUTES sa; DEBUG_EVENT event; CONTEXT context;
 HANDLE output,error,thread=NULL; FILE *log; DWORD bytes,status,stack[17],exit_code=999; int loader=0,trapped=0; char command[2048];
 if(argc!=8)return 2;
 log=fopen(argv[4],"wt"); if(!log)return 3;
 memset(&si,0,sizeof(si));memset(&sa,0,sizeof(sa));sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
 output=CreateFileA(argv[5],GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,0,NULL);error=CreateFileA(argv[6],GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,0,NULL);
 fprintf(log,"redirected_error_handle %08lx type %lu\n",(DWORD)error,GetFileType(error));
 si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdOutput=output;si.hStdError=error;si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
 sprintf(command,"\"%s\" %s",argv[1],argv[3]);
 if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[2],&si,&pi))return 5;
 CloseHandle(pi.hProcess);CloseHandle(pi.hThread);
 for(;;){if(!WaitForDebugEvent(&event,30000))return 6;status=DBG_CONTINUE;
  if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){pi.hProcess=event.u.CreateProcessInfo.hProcess;thread=event.u.CreateProcessInfo.hThread;if(event.u.CreateProcessInfo.lpBaseOfImage!=(void *)0x10000)return 7;if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);}
  else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);}
  else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
   if(event.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&!loader){loader=1;}
   else {context.ContextFlags=CONTEXT_FULL;if(!GetThreadContext(thread,&context))return 8;
    fprintf(log,"trap %08lx exception %08lx esp %08lx eax %08lx ecx %08lx edx %08lx\n",context.Eip-1,event.u.Exception.ExceptionRecord.ExceptionCode,context.Esp,context.Eax,context.Ecx,context.Edx);
    memset(stack,0,sizeof(stack));ReadProcessMemory(pi.hProcess,(void *)context.Esp,stack,sizeof(stack),&bytes);{int n;for(n=0;n<17;++n)fprintf(log,"stack %02d %08lx\n",n,stack[n]);}
    {DWORD site=context.Eip-1,state_address=0,words[56];unsigned char code[16];int n,length;
     if(site==0x4e980)state_address=stack[2];
     else if((site>=0x4b5f0&&site<=0x4e090)||site==0x4b550||site==0x4e0e0)state_address=stack[3];
     else if(site==0x4e870||site==0x4e6a0||site==0x4e630||site==0x4e140)state_address=stack[1];
     else if(site==0x4b5c0)state_address=stack[2];
     if(state_address){if(!ReadProcessMemory(pi.hProcess,(void *)state_address,words,sizeof(words),&bytes)||bytes!=sizeof(words))return 12;for(n=0;n<56;++n)fprintf(log,"state %02d %08lx\n",n,words[n]);
      if(words[19]&&words[2]<18){length=18-words[2];if(length>16)length=16;if(!ReadProcessMemory(pi.hProcess,(void *)words[19],code,length,&bytes)||bytes!=length)return 13;for(n=0;n<length;++n)fprintf(log,"code %02d %02x\n",n,code[n]);}
     }
    }
    { DWORD file[8],handle; unsigned char osfile; char buffer[64]; int length;
      ReadProcessMemory(pi.hProcess,(void *)0x85090,file,sizeof(file),&bytes);
      ReadProcessMemory(pi.hProcess,(void *)0x85622,&osfile,1,&bytes);
      ReadProcessMemory(pi.hProcess,(void *)0x85668,&handle,4,&bytes);
      fprintf(log,"stderr flags %08lx count %ld ptr %08lx base %08lx used %ld bufsiz %ld fd %lu osfile %02x handle %08lx inherited_handle_matches %d\n",file[3],file[1],file[0],file[2],file[0]-file[2],file[6],file[4],osfile,handle,handle==(DWORD)error);
      length=file[0]-file[2];if(length>63)length=63;if(length<0)length=0;
      if(length){ReadProcessMemory(pi.hProcess,(void *)file[2],buffer,length,&bytes);{int i;for(i=0;i<length;++i)fprintf(log,"stderr_buffer %02d %02x\n",i,(unsigned char)buffer[i]);}}
      fflush(log);
      if(context.Eip-1==0x30865){unsigned char opcode=0xe8;WriteProcessMemory(pi.hProcess,(void *)0x30865,&opcode,1,&bytes);FlushInstructionCache(pi.hProcess,(void *)0x30865,1);context.Eip=0x30865;if(!SetThreadContext(thread,&context))return 14;}
      else {trapped=1;TerminateProcess(pi.hProcess,99);}
    }
   }
  }else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=event.u.ExitProcess.dwExitCode;fprintf(log,"exit %lu\n",exit_code);ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);break;}
  ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
 }
 fclose(log);CloseHandle(output);CloseHandle(error);CloseHandle(thread);CloseHandle(pi.hProcess);
 return atoi(argv[7])?(trapped&&exit_code==99?0:9):(trapped||exit_code?10:0);
}
