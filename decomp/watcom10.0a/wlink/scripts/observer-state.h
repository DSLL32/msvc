#ifndef WLINK_OBSERVER_STATE_H
#define WLINK_OBSERVER_STATE_H
/* Debugger bookkeeping only, not reconstructed application logic. */
typedef unsigned int ObWord;
typedef char ObWordMustBe32Bits[(sizeof(ObWord) == 4) ? 1 : -1];
typedef struct ObRegs {
    ObWord eip, esp, eax, ebx, ecx, edx, esi, edi, ebp, flags;
    ObWord cs, ss, ds, es, fs, gs;
} ObRegs;
typedef struct ObStep {
    int active, armed;
    ObWord thread, site, target;
    ObRegs before;
} ObStep;
enum { OB_OK = 0, OB_ACTIVE, OB_FLAGS, OB_OWNER, OB_CONTEXT, OB_UNARMED,
       OB_TARGET, OB_STACK, OB_SEGMENTS, OB_OVERFLOW };
int ob_prepare(ObStep *, ObWord, ObWord, ObWord, const ObRegs *);
int ob_armed(ObStep *, ObWord, const ObRegs *);
int ob_finish(ObStep *, ObWord, const ObRegs *);
int ob_clear(ObStep *, ObWord, const ObRegs *);
int ob_return_stack(ObWord, ObWord, ObWord);
#endif
