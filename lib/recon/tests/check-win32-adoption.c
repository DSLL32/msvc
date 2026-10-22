#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <recon.h>

/* Match RC's structures, without requiring its private reconstruction header. */
struct AnsiString { USHORT length,capacity; char *text; };
struct UnicodeString { USHORT length,capacity; WCHAR *text; };

int main(void)
{
    CPINFO info; SYSTEMTIME clock; WCHAR wide[8]; char bytes[8];
    char counted[]={'a',0,(char)0x80,(char)0x81};
    struct AnsiString ansi={4,4,counted},back={0,sizeof(bytes),bytes};
    struct UnicodeString unicode={0,sizeof(wide),wide},allocated;
    CONSOLE_SCREEN_BUFFER_INFO screen;
    struct winsize size={37,101,0,0};
    int terminal,fd;

    assert(sizeof(CPINFO)==20 && offsetof(CPINFO,LeadByte)==6);
    assert(sizeof(COORD)==4 && sizeof(SMALL_RECT)==8);
    assert(sizeof(CONSOLE_SCREEN_BUFFER_INFO)==22);
    assert(sizeof(EXCEPTION_RECORD)==80 && sizeof(EXCEPTION_POINTERS)==8);
    assert(sizeof(ansi)==8 && sizeof(unicode)==8);
    assert(FILE_ATTRIBUTE_TEMPORARY==0x100);
    assert(FILE_FLAG_SEQUENTIAL_SCAN==0x08000000);
    assert(FILE_FLAG_RANDOM_ACCESS==0x10000000);
    unsetenv("RECON_CONSOLE_CP");
    assert(recon_GetConsoleCP()==437);
    assert(setenv("RECON_CONSOLE_CP","932",1)==0);
    assert(recon_GetConsoleCP()==932);
    assert(recon_GetCPInfo(recon_GetConsoleCP(),&info));
    assert(info.MaxCharSize==2 && info.DefaultChar[0]=='?');
    assert(!memcmp(info.LeadByte,"\x81\x9f\xe0\xfc\0\0\0\0\0\0\0\0",12));
    assert(recon_GetCPInfo(1252,&info) && info.MaxCharSize==1 && !info.LeadByte[0]);
    assert(recon_GetCPInfo(65001,&info) && info.MaxCharSize==4);
    assert(!recon_GetCPInfo(999,&info) && recon_GetLastError()==ERROR_INVALID_PARAMETER);
    assert(!recon_GetCPInfo(932,NULL) && recon_GetLastError()==ERROR_INVALID_PARAMETER);
    assert(setenv("RECON_CONSOLE_CP","932trailing",1)==0);
    assert(!recon_GetConsoleCP() && recon_GetLastError()==ERROR_INVALID_PARAMETER);
    assert(recon_SetHandleCount(30)==30);

    assert(recon_MultiByteToWideChar(932,0,"\x82\xa0",2,wide,8)==1 && wide[0]==0x3042);
    assert(recon_WideCharToMultiByte(932,0,wide,1,bytes,8,NULL,NULL)==2);
    assert((BYTE)bytes[0]==0x82 && (BYTE)bytes[1]==0xa0);
    assert(recon_MultiByteToWideChar(932,0,"\x82\xa0",2,wide,0)==1);
    assert(!recon_MultiByteToWideChar(932,0,"ab",2,wide,1));
    assert(recon_GetLastError()==ERROR_INSUFFICIENT_BUFFER);

    assert(recon_RtlAnsiStringToUnicodeString(&unicode,&ansi,FALSE)==0);
    assert(unicode.length==8 && !wide[1] && wide[2]==0x20ac && wide[3]==0x81 && !wide[4]);
    assert(recon_RtlUnicodeStringToAnsiString(&back,&unicode,FALSE)==0);
    assert(back.length==4 && !memcmp(bytes,counted,4) && !bytes[4]);
    unicode.capacity=8;
    assert(recon_RtlAnsiStringToUnicodeString(&unicode,&ansi,FALSE)==(LONG)0x80000005UL);
    unicode.capacity=sizeof(wide); unicode.length=7;
    assert(recon_RtlUnicodeStringToAnsiString(&back,&unicode,FALSE)==(LONG)0xc000000dUL);
    assert(recon_RtlAnsiStringToUnicodeString(&allocated,&ansi,TRUE)==0);
    assert(allocated.length==8 && allocated.capacity==10 && !allocated.text[4]);
    free(allocated.text);

    assert(setenv("TZ","UTC0",1)==0); tzset();
    assert(setenv("RECON_FIXED_TIME","951827696",1)==0);
    recon_GetLocalTime(&clock);
    assert(clock.wYear==2000 && clock.wMonth==2 && clock.wDay==29);
    assert(clock.wHour==12 && clock.wMinute==34 && clock.wSecond==56 && !clock.wMilliseconds);
    assert(setenv("TZ","EST5",1)==0); tzset();
    recon_GetLocalTime(&clock); assert(clock.wHour==7 && clock.wDay==29);

    fd=open("/dev/null",O_RDWR); assert(fd>=0);
    assert(!recon_GetConsoleScreenBufferInfo((HANDLE)(uintptr_t)(fd+1),&screen));
    assert(recon_GetLastError()==ERROR_INVALID_HANDLE); close(fd);
    terminal=posix_openpt(O_RDWR|O_NOCTTY); assert(terminal>=0);
    assert(grantpt(terminal)==0 && unlockpt(terminal)==0);
    fd=open(ptsname(terminal),O_RDWR|O_NOCTTY); assert(fd>=0);
    assert(ioctl(fd,TIOCSWINSZ,&size)==0);
    assert(recon_GetConsoleScreenBufferInfo((HANDLE)(uintptr_t)(fd+1),&screen));
    assert(screen.dwSize.X==101 && screen.dwSize.Y==37);
    assert(screen.srWindow.Right==100 && screen.srWindow.Bottom==36);
    close(fd); close(terminal);
    return 0;
}
