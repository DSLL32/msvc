#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <trace-abi.h>
#include <recon.h>

/* Assembly is essential here: the private entry consumes ESI and ST(0), and
 * ordinary C cannot seed and observe every caller register or EFLAGS. The
 * harness itself uses relative addresses so the same probe runs as PIE. */
unsigned char probe_original_fx[512] __attribute__((aligned(16)));
unsigned char probe_before_fx[512] __attribute__((aligned(16)));
unsigned char probe_after_fx[512] __attribute__((aligned(16)));
unsigned char probe_output[18];
uint32_t probe_registers[9], probe_canaries[4];
uintptr_t probe_expected_sp, probe_actual_sp, probe_expected_bp;
uint32_t probe_expected_flags;
const unsigned short probe_control = 0x0e7f;
const uint32_t probe_mxcsr = 0x3f80;
const uint32_t probe_xmm_zero[4] = { 0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210 };
const uint32_t probe_xmm_seven[4] = { 0x11112222, 0x33334444, 0x55556666, 0x77778888 };
const long double probe_top = 10.5L, probe_middle = -0.25L, probe_bottom = 1234.125L;

__attribute__((naked, noinline)) void naked_store_extended(void)
{
    RECON_TRACE_NAKED_ENTRY(naked_store_extended)
    __asm__ volatile ("fstpt (%esi)\n\tfwait\n\tret");
}

__attribute__((naked, noinline)) void probe_caller_state(void)
{
    __asm__ volatile (
        "pushfl\n\tpushal\n\tsubl $64,%esp\n\t"
        "call 992f\n992:\n\tpopl %ebp\n\t"
        "fxsave probe_original_fx-992b(%ebp)\n\tfninit\n\t"
        "fldcw probe_control-992b(%ebp)\n\t"
        "ldmxcsr probe_mxcsr-992b(%ebp)\n\t"
        "movdqu probe_xmm_zero-992b(%ebp),%xmm0\n\t"
        "movdqu probe_xmm_seven-992b(%ebp),%xmm7\n\t"
        "fldt probe_bottom-992b(%ebp)\n\t"
        "fldt probe_middle-992b(%ebp)\n\t"
        "fldt probe_top-992b(%ebp)\n\t"
        "fxsave probe_before_fx-992b(%ebp)\n\t"
        "movl $0x13579bdf,0(%esp)\n\tmovl $0x2468ace0,4(%esp)\n\t"
        "movl $0x55aa55aa,56(%esp)\n\tmovl $0xaa55aa55,60(%esp)\n\t"
        "movl %esp,probe_expected_sp-992b(%ebp)\n\t"
        "movl %ebp,probe_expected_bp-992b(%ebp)\n\t"
        "leal probe_output+4-992b(%ebp),%esi\n\t"
        "movl $0x23456789,%ecx\n\tmovl $0x3456789a,%edx\n\t"
        "movl $0x456789ab,%ebx\n\tmovl $0x56789abc,%edi\n\t"
        "pushfl\n\tpopl %eax\n\tandl $0xfffff32a,%eax\n\torl $0x8d5,%eax\n\t"
        "movl %eax,probe_expected_flags-992b(%ebp)\n\tpushl %eax\n\tpopfl\n\t"
        "movl $0x12345678,%eax\n\tcall naked_store_extended\n\t"
        "pushfl\n\tpushal\n\tmovl %esp,%esi\n\t"
        "leal probe_registers-992b(%ebp),%edi\n\tmovl $9,%ecx\n\tcld\n\trep movsl\n\t"
        "addl $36,%esp\n\tfxsave probe_after_fx-992b(%ebp)\n\t"
        "movl %esp,probe_actual_sp-992b(%ebp)\n\t"
        "movl 0(%esp),%eax\n\tmovl %eax,probe_canaries-992b(%ebp)\n\t"
        "movl 4(%esp),%eax\n\tmovl %eax,probe_canaries+4-992b(%ebp)\n\t"
        "movl 56(%esp),%eax\n\tmovl %eax,probe_canaries+8-992b(%ebp)\n\t"
        "movl 60(%esp),%eax\n\tmovl %eax,probe_canaries+12-992b(%ebp)\n\t"
        "fxrstor probe_original_fx-992b(%ebp)\n\t"
        "addl $64,%esp\n\tpopal\n\tpopfl\n\tret");
}

static unsigned short word_at(const unsigned char *data)
{
    unsigned short value;
    memcpy(&value, data, sizeof(value));
    return value;
}

static void check_naked_state(void)
{
    unsigned index;
    unsigned short before_status, after_status;
    memset(probe_output, 0xa5, sizeof(probe_output));
    probe_caller_state();
    /* PUSHA stores EDI, ESI, EBP, its incoming ESP, EBX, EDX, ECX, EAX. */
    assert(probe_registers[0] == 0x56789abc);
    assert(probe_registers[1] == (uintptr_t)(probe_output + 4));
    assert(probe_registers[2] == probe_expected_bp);
    assert(probe_registers[3] == probe_expected_sp - 4);
    assert(probe_registers[4] == 0x456789ab);
    assert(probe_registers[5] == 0x3456789a);
    assert(probe_registers[6] == 0x23456789);
    assert(probe_registers[7] == 0x12345678);
    assert(probe_registers[8] == probe_expected_flags);
    assert(probe_actual_sp == probe_expected_sp);
    assert(probe_canaries[0] == 0x13579bdf && probe_canaries[1] == 0x2468ace0);
    assert(probe_canaries[2] == 0x55aa55aa && probe_canaries[3] == 0xaa55aa55);
    assert(!memcmp(probe_output + 4, &probe_top, 10));
    for (index = 0; index < 4; ++index)
        assert(probe_output[index] == 0xa5 && probe_output[14 + index] == 0xa5);
    assert(word_at(probe_before_fx) == probe_control);
    assert(word_at(probe_after_fx) == probe_control);
    before_status = word_at(probe_before_fx + 2);
    after_status = word_at(probe_after_fx + 2);
    assert((before_status & ~0x3800) == (after_status & ~0x3800));
    assert(((before_status >> 11) + 1) % 8 == (after_status >> 11) % 8);
    assert(probe_before_fx[4] == 0xe0 && probe_after_fx[4] == 0xc0);
    assert(!memcmp(probe_after_fx + 32, &probe_middle, 10));
    assert(!memcmp(probe_after_fx + 48, &probe_bottom, 10));
    assert(!memcmp(probe_before_fx + 24, probe_after_fx + 24, 4));
    assert(!memcmp(probe_before_fx + 160, probe_after_fx + 160, 128));
}

int main(void)
{
    check_naked_state();
    check_naked_state();
    puts("naked ABI preserved");
    return 0;
}
