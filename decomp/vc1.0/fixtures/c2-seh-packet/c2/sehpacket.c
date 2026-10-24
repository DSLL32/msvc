/* One protected packet-processing unit; runtime inputs remain unknown. */
typedef struct Packet {
    const unsigned char *bytes;
    unsigned length;
    unsigned tag;
} Packet;

unsigned long __cdecl _exception_code(void);
int __cdecl _abnormal_termination(void);
extern int __cdecl packet_callback(const Packet *, unsigned, int);
volatile unsigned long packet_last_exception;
volatile int packet_cleanup_total;

static int packet_filter(unsigned long code, int mode)
{
    packet_last_exception = code;
    return (mode & 2) != 0;
}

static unsigned packet_mix(unsigned value)
{
    __asm {
        mov eax, value
        rol eax, 5
        test eax, 1
        jz even_tag
        xor eax, 05a5a5a5ah
    even_tag:
        add eax, 01357h
        mov value, eax
    }
    return value;
}

int __cdecl process_protected_packet(const Packet *packet, unsigned limit, int mode)
{
    volatile int total = 0;
    unsigned i, count;
    __try {
        __try {
            count = packet->length;
            if (count > limit) count = limit;
            if (mode & 1) __leave;
            for (i = 0; i < count; ++i) {
                unsigned value = packet_mix(packet->tag + packet->bytes[i] + i);
                total += packet_callback(packet, value, total);
                if ((value & 15) == 9) __leave;
                if (mode & 4) return total;
            }
        } __finally {
            packet_cleanup_total += _abnormal_termination() ? 3 : 1;
        }
    } __except (packet_filter(_exception_code(), mode)) {
        total = -(int)(packet_last_exception & 32767) - 1;
    }
    return total;
}
