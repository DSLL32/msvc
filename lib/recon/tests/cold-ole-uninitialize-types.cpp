#include <windows.h>
#include <objbase.h>
#include <recon.h>
static void (WINAPI *typed)(void)=CoUninitialize;
int main(void) { return typed==0; }
