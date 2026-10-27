#include <windows.h>
#include <recon.h>
typedef char Size[sizeof(WIN32_FIND_DATAA)==320?1:-1];
typedef char Name[offsetof(WIN32_FIND_DATAA,cFileName)==44?1:-1];
static HANDLE (WINAPI *first)(LPCSTR,LPWIN32_FIND_DATAA)=FindFirstFileA;
static BOOL (WINAPI *next)(HANDLE,LPWIN32_FIND_DATAA)=FindNextFileA;
static BOOL (WINAPI *close_find)(HANDLE)=FindClose;
int main(void) {return !first || !next || !close_find;}
