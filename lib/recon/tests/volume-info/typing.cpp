#include <windows.h>
#include <dir.h>
#include <recon.h>
static BOOL (WINAPI *volume)(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD)=GetVolumeInformationA;
static int (__cdecl *next)(ffblk*)=findnext;
typedef char CaseFlag[FILE_CASE_PRESERVED_NAMES==2?1:-1];
