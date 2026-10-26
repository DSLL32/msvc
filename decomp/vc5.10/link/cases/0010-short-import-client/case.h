__declspec(dllimport) extern int SharedValue;
extern int __stdcall DllMain(void *,unsigned,void *);
/* These are genuine archive roots, not client calls to the two exports. */
#pragma comment(linker,"/include:HelloWindow")
#pragma comment(linker,"/include:Sum")
int __stdcall entry(void)
{
    return DllMain(0,0,0)+SharedValue;
}
