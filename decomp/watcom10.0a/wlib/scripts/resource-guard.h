/* Original-capture guard policy. Shared verbatim by debugger and offline probes;
   this is analysis tooling, not reconstructed application code. */
#ifndef RESOURCE_GUARD_H
#define RESOURCE_GUARD_H

typedef unsigned long RgWord;
typedef char RgWordMustBe32Bits[(sizeof(RgWord) == 4) ? 1 : -1];

typedef struct RgContext {
    RgWord pc, flags;
    /* Register opcode order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI. */
    RgWord registers[8];
    /* Meaningful selector WORDs: CS, SS, DS, ES, FS, GS. Raw CONTEXT is logged. */
    RgWord segments[6];
} RgContext;

typedef struct RgState {
    RgWord thread, owner_stack, owner_return, pointer, capacity;
    RgWord reader_stack, reader_return, calls, max_calls;
    RgWord step_thread, step_site, step_pc, step_stack, step_value;
    RgWord step_flags, step_ds, step_gs;
    RgWord initial_ds, initial_gs;
    RgContext step_before;
    int owner, reader, step, initial, step_armed;
} RgState;

enum RgResult {
    RG_OK, RG_THREAD, RG_ORDER, RG_OWNER, RG_CAPACITY, RG_SELECTOR,
    RG_RETURN, RG_LIMIT, RG_STEP, RG_OVERFLOW, RG_SEGMENTS
};

static int rg_initial_segments(RgState *s, RgWord thread, RgWord ds, RgWord gs)
{
    if (thread != s->thread) return RG_THREAD;
    if (s->initial || s->owner || s->reader || s->step) return RG_ORDER;
    s->initial = 1; s->initial_ds = ds; s->initial_gs = gs;
    return RG_OK;
}

static int rg_segments(const RgState *s, RgWord thread, RgWord ds, RgWord gs)
{
    if (thread != s->thread) return RG_THREAD;
    if (!s->initial || ds != s->initial_ds || gs != s->initial_gs) return RG_SEGMENTS;
    return RG_OK;
}

static int rg_begin_owner(RgState *s, RgWord thread, RgWord stack,
                          RgWord caller, RgWord pointer, RgWord actual_owner,
                          RgWord actual_capacity)
{
    if (thread != s->thread) return RG_THREAD;
    if (s->owner || s->reader || s->step) return RG_ORDER;
    if (!pointer || pointer != actual_owner || stack > 0xfffffffbUL ||
        pointer < stack + 4) return RG_OWNER;
    if (!actual_capacity || actual_capacity > 512) return RG_CAPACITY;
    if (pointer > 0xffffffffUL - actual_capacity) return RG_OVERFLOW;
    s->owner = 1; s->owner_stack = stack; s->owner_return = caller;
    s->pointer = pointer; s->capacity = actual_capacity;
    return RG_OK;
}

static int rg_enter_reader(RgState *s, RgWord thread, RgWord stack,
                           RgWord caller, RgWord expected_return,
                           RgWord pointer, RgWord capacity,
                           RgWord selector, RgWord actual_ds,
                           RgWord live_owner_return, RgWord actual_gs)
{
    if (thread != s->thread) return RG_THREAD;
    if (rg_segments(s, thread, actual_ds, actual_gs) != RG_OK) return RG_SEGMENTS;
    if (!s->owner || s->reader || s->step) return RG_ORDER;
    if (s->calls >= s->max_calls) return RG_LIMIT;
    if (!actual_ds || selector != actual_ds) return RG_SELECTOR;
    if (pointer != s->pointer || stack >= s->owner_stack ||
        live_owner_return != s->owner_return) return RG_OWNER;
    if (capacity != s->capacity) return RG_CAPACITY;
    if (caller != expected_return) return RG_RETURN;
    if (stack > 0xffffffefUL) return RG_OVERFLOW;
    s->reader = 1; s->reader_stack = stack; s->reader_return = caller;
    ++s->calls;
    return RG_OK;
}

static int rg_leave_reader(RgState *s, RgWord thread, RgWord stack, RgWord pc)
{
    if (thread != s->thread) return RG_THREAD;
    if (!s->owner || !s->reader || s->step) return RG_ORDER;
    if (pc != s->reader_return || stack != s->reader_stack + 16) return RG_RETURN;
    s->reader = 0;
    return RG_OK;
}

static int rg_end_owner(RgState *s, RgWord thread, RgWord stack, RgWord pc)
{
    if (thread != s->thread) return RG_THREAD;
    if (!s->owner || s->reader || s->step) return RG_ORDER;
    if (pc != s->owner_return || stack != s->owner_stack + 4) return RG_RETURN;
    s->owner = 0;
    return RG_OK;
}

static int rg_prepare_step(RgState *s, RgWord thread, RgWord site,
                           RgWord opcode, const RgContext *before)
{
    if (thread != s->thread) return RG_THREAD;
    if (s->step || (before->flags & 0x100UL) || before->pc != site) return RG_STEP;
    if (opcode < 0x50 || opcode > 0x57) return RG_STEP;
    if (site == 0xffffffffUL || before->registers[4] < 4) return RG_OVERFLOW;
    s->step = 1; s->step_thread = thread; s->step_site = site;
    s->step_pc = site + 1; s->step_stack = before->registers[4] - 4;
    s->step_value = before->registers[opcode & 7];
    s->step_flags = before->flags; s->step_ds = before->segments[2]; s->step_gs = before->segments[5];
    s->step_before = *before; s->step_armed = 0;
    return RG_OK;
}

static int rg_confirm_arm(RgState *s, RgWord thread, const RgContext *armed)
{
    unsigned i;
    if (thread != s->thread || thread != s->step_thread) return RG_THREAD;
    if (!s->step || s->step_armed || armed->pc != s->step_before.pc ||
        armed->flags != (s->step_before.flags | 0x100UL)) return RG_STEP;
    for (i = 0; i < 8; ++i)
        if (armed->registers[i] != s->step_before.registers[i]) return RG_STEP;
    for (i = 0; i < 6; ++i)
        if (armed->segments[i] != s->step_before.segments[i]) return RG_STEP;
    s->step_armed = 1;
    return RG_OK;
}

static int rg_finish_step(RgState *s, RgWord thread, RgWord exception_code,
                          int first_chance, const RgContext *after, RgWord pushed_value)
{
    unsigned i;
    if (thread != s->thread || thread != s->step_thread) return RG_THREAD;
    if (!s->step || !s->step_armed || exception_code != 0x80000004UL || !first_chance ||
        after->pc != s->step_pc || after->registers[4] != s->step_stack ||
        pushed_value != s->step_value || after->flags != s->step_before.flags)
        return RG_STEP;
    for (i = 0; i < 8; ++i)
        if (i != 4 && after->registers[i] != s->step_before.registers[i]) return RG_STEP;
    for (i = 0; i < 6; ++i)
        if (after->segments[i] != s->step_before.segments[i]) return RG_STEP;
    s->step = 0; s->step_armed = 0;
    return RG_OK;
}

#endif
