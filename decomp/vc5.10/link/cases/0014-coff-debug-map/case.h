/* Two genuine original-compiled COFF inputs with source/line metadata,
 * packaged functions, cross-TU calls and initialized pointer relocations. */
#ifdef DEBUG_LEFT
extern int debug_counter;
extern int (__cdecl *debug_hook)(int);
int __cdecl StepLeft(int value)
{
    debug_counter+=value;
    return debug_hook(value)+debug_counter;
}
int __cdecl SameDebugA(int value) { return value+11; }
int __cdecl SameDebugB(int value) { return value+11; }
int __cdecl UnusedDebug(int value) { return value-29; }
int __stdcall DebugEntry(void)
{
    return StepLeft(3)+SameDebugA(1)+SameDebugB(2);
}
#else
#pragma data_seg(".payload")
char debug_label[]="COFF debug map";
int __cdecl StepRight(int value);
int (__cdecl *debug_hook)(int)=StepRight;
#pragma data_seg()
int debug_counter;
int __cdecl StepRight(int value)
{
    static int local_bias=5;
    return value+debug_label[0]+local_bias;
}
#endif
