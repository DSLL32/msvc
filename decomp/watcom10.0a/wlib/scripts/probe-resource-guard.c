/* Offline native probes of the exact policy included by diagnose-startup.c. */
#include <stdio.h>
#include <string.h>
#include "resource-guard.h"

static unsigned checks;
#define CHECK(value) do { ++checks; if (!(value)) { \
    fprintf(stderr, "guard probe failed at line %d\n", __LINE__); return 1; } } while (0)

static RgState fresh(void)
{
    RgState s;
    memset(&s, 0, sizeof(s)); s.thread = 7; s.max_calls = 1024;
    rg_initial_segments(&s, 7, 0x2b, 0x63);
    return s;
}

static RgState owned(void)
{
    RgState s = fresh();
    rg_begin_owner(&s, 7, 0x1000, 0x401000, 0x1004, 0x1004, 128);
    return s;
}

static RgState reading(void)
{
    RgState s = owned();
    rg_enter_reader(&s, 7, 0xf00, 0x402000, 0x402000, 0x1004, 128, 0x2b, 0x2b, 0x401000, 0x63);
    return s;
}

static RgContext before_context(void)
{
    RgContext c;
    unsigned i;
    memset(&c, 0, sizeof(c)); c.pc = 0x400100; c.flags = 0x293;
    for (i = 0; i < 8; ++i) c.registers[i] = 0x10000000UL + i;
    c.registers[4] = 0x1000;
    c.segments[0] = 0x23; c.segments[1] = 0x2b;
    c.segments[2] = 0x2b; c.segments[3] = 0x2b;
    c.segments[4] = 0x6b; c.segments[5] = 0x63;
    return c;
}

static RgContext arm_context(void)
{
    RgContext c = before_context(); c.flags |= 0x100; return c;
}

static RgContext after_context(void)
{
    RgContext c = before_context(); ++c.pc; c.registers[4] -= 4; return c;
}

static RgState stepping(void)
{
    RgState s = fresh();
    RgContext c = before_context();
    rg_prepare_step(&s, 7, 0x400100, 0x53, &c);
    return s;
}

static RgState armed_state(void)
{
    RgState s = stepping();
    RgContext c = arm_context();
    rg_confirm_arm(&s, 7, &c);
    return s;
}

static int step_probes(void)
{
    RgState s, saved;
    RgContext c, before;
    unsigned i;
    /* Same pending state is rejected without any ARM readback evidence. */
    s = stepping(); c = after_context(); saved = s;
    CHECK(rg_finish_step(&s,7,0x80000004UL,1,&c,0x10000003UL) == RG_STEP);
    CHECK(!memcmp(&s,&saved,sizeof(s)));
    s = fresh(); c = arm_context(); CHECK(rg_confirm_arm(&s,7,&c) != RG_OK);
    s = stepping(); c = arm_context(); CHECK(rg_confirm_arm(&s,8,&c) == RG_THREAD);
    s = stepping(); c = before_context(); CHECK(rg_confirm_arm(&s,7,&c) == RG_STEP);
    s = stepping(); c = arm_context(); ++c.pc; CHECK(rg_confirm_arm(&s,7,&c) == RG_STEP);
    for (i = 0; i < 8; ++i) {
        s = stepping(); c = arm_context(); c.registers[i] ^= 4;
        CHECK(rg_confirm_arm(&s,7,&c) == RG_STEP);
    }
    for (i = 0; i < 6; ++i) {
        s = stepping(); c = arm_context(); c.segments[i] ^= 8;
        CHECK(rg_confirm_arm(&s,7,&c) == RG_STEP);
    }
    /* No non-TF EFlags bit is waived, including the CPU resume flag RF. */
    for (i = 0; i < 32; ++i) if (i != 8) {
        s = stepping(); c = arm_context(); c.flags ^= 1UL << i;
        CHECK(rg_confirm_arm(&s,7,&c) == RG_STEP);
    }
    s = armed_state(); c = arm_context(); CHECK(rg_confirm_arm(&s,7,&c) == RG_STEP);
    s = armed_state(); c = after_context(); CHECK(rg_finish_step(&s,8,0x80000004UL,1,&c,0x10000003UL) == RG_THREAD);
    s = armed_state(); c = after_context(); CHECK(rg_finish_step(&s,7,0x80000003UL,1,&c,0x10000003UL) == RG_STEP);
    s = armed_state(); c = after_context(); CHECK(rg_finish_step(&s,7,0x80000004UL,0,&c,0x10000003UL) == RG_STEP);
    s = armed_state(); c = after_context(); ++c.pc; CHECK(rg_finish_step(&s,7,0x80000004UL,1,&c,0x10000003UL) == RG_STEP);
    s = armed_state(); c = after_context(); CHECK(rg_finish_step(&s,7,0x80000004UL,1,&c,0x10000002UL) == RG_STEP);
    for (i = 0; i < 8; ++i) {
        s = armed_state(); c = after_context(); c.registers[i] ^= 4;
        CHECK(rg_finish_step(&s,7,0x80000004UL,1,&c,0x10000003UL) == RG_STEP);
    }
    for (i = 0; i < 6; ++i) {
        s = armed_state(); c = after_context(); c.segments[i] ^= 8;
        CHECK(rg_finish_step(&s,7,0x80000004UL,1,&c,0x10000003UL) == RG_STEP);
    }
    for (i = 0; i < 32; ++i) {
        s = armed_state(); c = after_context(); c.flags ^= 1UL << i;
        CHECK(rg_finish_step(&s,7,0x80000004UL,1,&c,0x10000003UL) == RG_STEP);
    }
    s = fresh(); c = before_context(); CHECK(rg_prepare_step(&s,8,c.pc,0x53,&c) == RG_THREAD);
    s = fresh(); c = before_context(); CHECK(rg_prepare_step(&s,7,c.pc,0xe8,&c) == RG_STEP);
    s = fresh(); c = arm_context(); CHECK(rg_prepare_step(&s,7,c.pc,0x53,&c) == RG_STEP);
    s = fresh(); c = before_context(); CHECK(rg_prepare_step(&s,7,c.pc+1,0x53,&c) == RG_STEP);
    s = stepping(); c = before_context(); CHECK(rg_prepare_step(&s,7,c.pc,0x53,&c) == RG_STEP);
    s = fresh(); c = before_context(); c.pc = 0xffffffffUL; CHECK(rg_prepare_step(&s,7,c.pc,0x53,&c) == RG_OVERFLOW);
    s = fresh(); c = before_context(); c.registers[4] = 3; CHECK(rg_prepare_step(&s,7,c.pc,0x53,&c) == RG_OVERFLOW);
    /* Exercise all eight real one-byte PUSH-reg opcodes and repeat lifecycles. */
    s = fresh();
    for (i = 0; i < 8; ++i) {
        before = before_context(); c = arm_context();
        CHECK(rg_prepare_step(&s,7,before.pc,0x50+i,&before) == RG_OK);
        CHECK(rg_confirm_arm(&s,7,&c) == RG_OK);
        c = after_context();
        CHECK(rg_finish_step(&s,7,0x80000004UL,1,&c,before.registers[i]) == RG_OK);
        CHECK(!s.step && !s.step_armed);
        CHECK(rg_finish_step(&s,7,0x80000004UL,1,&c,before.registers[i]) == RG_STEP);
    }
    printf("full-context TF protocol: %u native policy checks passed\n", checks);
    return 0;
}

int main(void)
{
    RgState s, t;
    unsigned i;
    s = fresh();
    for (i = 0; i < 3; ++i) {
        CHECK(rg_begin_owner(&s, 7, 0x1000, 0x401000, 0x1004, 0x1004, 128) == RG_OK);
        CHECK(rg_enter_reader(&s, 7, 0xf00, 0x402000, 0x402000, 0x1004, 128, 0x2b, 0x2b, 0x401000, 0x63) == RG_OK);
        CHECK(rg_leave_reader(&s, 7, 0xf10, 0x402000) == RG_OK);
        CHECK(rg_end_owner(&s, 7, 0x1004, 0x401000) == RG_OK);
    }
    CHECK(s.calls == 3 && !s.owner && !s.reader);
    s = fresh(); CHECK(rg_begin_owner(&s, 8, 0x1000, 0x401000, 0x1004, 0x1004, 128) == RG_THREAD);
    s = fresh(); CHECK(rg_begin_owner(&s, 7, 0x1000, 0x401000, 0x1004, 0x1084, 128) == RG_OWNER);
    s = fresh(); CHECK(rg_begin_owner(&s, 7, 0x1000, 0x401000, 0xfff, 0xfff, 128) == RG_OWNER);
    s = fresh(); CHECK(rg_begin_owner(&s, 7, 0x1000, 0x401000, 0x1004, 0x1004, 0) == RG_CAPACITY);
    s = fresh(); CHECK(rg_begin_owner(&s, 7, 0x1000, 0x401000, 0xffffffc0, 0xffffffc0, 128) == RG_OVERFLOW);
    s = owned(); CHECK(rg_begin_owner(&s, 7, 0x1000, 0x401000, 0x1004, 0x1004, 128) == RG_ORDER);

#define READER(thread,stack,caller,expected,pointer,capacity,selector,ds,live) \
    rg_enter_reader(&s,thread,stack,caller,expected,pointer,capacity,selector,ds,live,0x63)
    s = fresh(); CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,128,0x2b,0x2b,0x401000) == RG_ORDER);
    s = owned(); CHECK(READER(8,0xf00,0x402000,0x402000,0x1004,128,0x2b,0x2b,0x401000) == RG_THREAD);
    s = owned(); CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,128,0x63,0x2b,0x401000) == RG_SELECTOR);
    s = owned(); CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,128,0,0,0x401000) == RG_SEGMENTS);
    s = owned(); CHECK(READER(7,0xf00,0x402000,0x402000,0x1008,128,0x2b,0x2b,0x401000) == RG_OWNER);
    s = owned(); CHECK(READER(7,0x1000,0x402000,0x402000,0x1004,128,0x2b,0x2b,0x401000) == RG_OWNER);
    s = owned(); CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,128,0x2b,0x2b,0x401001) == RG_OWNER);
    s = owned(); CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,80,0x2b,0x2b,0x401000) == RG_CAPACITY);
    s = owned(); s.capacity = 80; CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,128,0x2b,0x2b,0x401000) == RG_CAPACITY);
    s = owned(); CHECK(READER(7,0xf00,0x402001,0x402000,0x1004,128,0x2b,0x2b,0x401000) == RG_RETURN);
    s = owned(); s.max_calls = 0; CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,128,0x2b,0x2b,0x401000) == RG_LIMIT);
    s = reading(); CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,128,0x2b,0x2b,0x401000) == RG_ORDER);
    s = owned(); t = s; CHECK(READER(7,0xf00,0x402000,0x402000,0x1004,128,0x63,0x2b,0x401000) == RG_SELECTOR); CHECK(!memcmp(&s,&t,sizeof(s)));
    s = reading(); CHECK(rg_leave_reader(&s,8,0xf10,0x402000) == RG_THREAD);
    s = reading(); CHECK(rg_leave_reader(&s,7,0xf0c,0x402000) == RG_RETURN);
    s = reading(); CHECK(rg_leave_reader(&s,7,0xf10,0x402001) == RG_RETURN);
    s = fresh(); CHECK(rg_leave_reader(&s,7,0xf10,0x402000) == RG_ORDER);
    s = reading(); CHECK(rg_end_owner(&s,7,0x1004,0x401000) == RG_ORDER);
    s = owned(); CHECK(rg_end_owner(&s,8,0x1004,0x401000) == RG_THREAD);
    s = owned(); CHECK(rg_end_owner(&s,7,0x1008,0x401000) == RG_RETURN);
    s = owned(); CHECK(rg_end_owner(&s,7,0x1004,0x401001) == RG_RETURN);
    s = fresh(); CHECK(rg_initial_segments(&s,7,0x2b,0x63) == RG_ORDER);
    s = fresh(); CHECK(rg_segments(&s,7,0x2b,0x63) == RG_OK);
    s = fresh(); CHECK(rg_segments(&s,8,0x2b,0x63) == RG_THREAD);
    s = fresh(); CHECK(rg_segments(&s,7,0x30,0x63) == RG_SEGMENTS);
    s = fresh(); CHECK(rg_segments(&s,7,0x2b,0x2b) == RG_SEGMENTS);
    s = owned(); CHECK(rg_enter_reader(&s,7,0xf00,0x402000,0x402000,0x1004,128,0x2b,0x2b,0x401000,0x2b) == RG_SEGMENTS);
    CHECK(step_probes() == 0);
    printf("resource guard: %u production-policy checks passed\n", checks);
    return 0;
}
