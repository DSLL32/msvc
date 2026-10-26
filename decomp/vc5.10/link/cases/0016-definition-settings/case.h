/* Genuine VC5-compiled DLL input. The completed DLL is compared as a linker
 * artifact; this fixture does not load it or claim runtime initialization. */
#pragma data_seg(".payload")
int shared_total=11;
#pragma data_seg()
int uninitialized_table[32];
int __stdcall DllMain(void *module,unsigned reason,void *reserved)
{
    if(reason==1)uninitialized_table[0]=shared_total;
    return 1;
}
int __stdcall Transform(int left,int right)
{
    return left*3+right+shared_total+uninitialized_table[0];
}
int __cdecl ClampValue(int value)
{
    if(value<0)return 0;
    if(value>255)return 255;
    return value;
}
