/* Authored native i386 controller probe, not reconstructed target source. */
#include <sys/mman.h>

unsigned char *probe_code;
unsigned char *probe_stack;
volatile unsigned marker;
unsigned probe_pointer;

__attribute__((noinline)) void probe_ready(void)
{
    __asm__ __volatile__("" : : : "memory");
}

int main(void)
{
    probe_code = mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    probe_stack = mmap(0, 4096, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (probe_code == MAP_FAILED || probe_stack == MAP_FAILED)
        return 2;
    probe_ready();
    return 0;
}
