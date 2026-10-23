/* Original compiler input: force the buffered page table past its4MB start.
 * The486 routines are inspected only, each in an independent COMDAT. */
unsigned char blob[5UL*1024*1024+257]={0x61,0x62,0x63};
unsigned long tail=0x13579bdfUL;
unsigned char *ptr=blob+123;
int __stdcall E(void *module,unsigned long reason,void *reserved)
{
    return ptr[0]==0 && tail==0x13579bdfUL;
}
void ops486(void) { __asm { xadd dword ptr [esi],eax } }
void cmpb(void) { __asm { cmpxchg byte ptr [esi],cl } }
void cmpd(void) { __asm { cmpxchg dword ptr [esi],eax } }
