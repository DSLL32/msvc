#include "observer-state.h"
#include <recon.h>

static int same_gp(const ObRegs *a, const ObRegs *b)
{
    return a->eax == b->eax && a->ebx == b->ebx && a->ecx == b->ecx &&
           a->edx == b->edx && a->esi == b->esi && a->edi == b->edi &&
           a->ebp == b->ebp;
}

static int same_segments(const ObRegs *a, const ObRegs *b)
{
    return a->cs == b->cs && a->ss == b->ss && a->ds == b->ds &&
           a->es == b->es && a->fs == b->fs && a->gs == b->gs;
}

int ob_prepare(ObStep *s, ObWord thread, ObWord site, ObWord target,
               const ObRegs *before)
{
    if (s->active) return OB_ACTIVE;
    if (!thread || !site || !target || before->eip != site + 1 || site == 0xffffffffU)
        return OB_CONTEXT;
    if (before->flags & 0x100) return OB_FLAGS;
    s->before = *before;
    s->before.eip = site;
    s->active = 1; s->armed = 0;
    s->thread = thread; s->site = site; s->target = target;
    return OB_OK;
}

int ob_armed(ObStep *s, ObWord thread, const ObRegs *armed)
{
    if (!s->active || s->armed) return OB_UNARMED;
    if (thread != s->thread) return OB_OWNER;
    if (armed->eip != s->site || armed->esp != s->before.esp ||
        !same_gp(armed, &s->before)) return OB_CONTEXT;
    if (!same_segments(armed, &s->before)) return OB_SEGMENTS;
    if (armed->flags != (s->before.flags | 0x100)) return OB_FLAGS;
    s->armed = 1;
    return OB_OK;
}

int ob_finish(ObStep *s, ObWord thread, const ObRegs *actual)
{
    if (!s->active || !s->armed) return OB_UNARMED;
    if (thread != s->thread) return OB_OWNER;
    if (actual->eip != s->target) return OB_TARGET;
    if (actual->esp != s->before.esp) return OB_STACK;
    if (!same_gp(actual, &s->before)) return OB_CONTEXT;
    if (!same_segments(actual, &s->before)) return OB_SEGMENTS;
    /* Explicit Wine profile: TF was read back armed, then consumed in the
     * first-chance owned SINGLE_STEP delivery. RF and every other flag match. */
    if (actual->flags != s->before.flags) return OB_FLAGS;
    return OB_OK;
}

int ob_clear(ObStep *s, ObWord thread, const ObRegs *actual)
{
    int result = ob_finish(s, thread, actual);
    if (result == OB_OK) { s->active = 0; s->armed = 0; }
    return result;
}

int ob_return_stack(ObWord entry_esp, ObWord count, ObWord actual_esp)
{
    ObWord size;
    if (count > 7) return OB_CONTEXT;
    size = 4 + count * 4;
    if (entry_esp > 0xffffffffU - size) return OB_OVERFLOW;
    return actual_esp == entry_esp + size ? OB_OK : OB_STACK;
}
