/* Proposed original-compiler case, not yet validated or frozen. */
typedef struct Packet {
    unsigned length;
    unsigned index;
    unsigned char *data;
} Packet;
extern int packet_external(Packet *, unsigned);
int packet_small[17];
unsigned char packet_medium[1024];
unsigned char packet_large[70000];
static unsigned short packet_weights[512] = { 3, 5, 7, 11 };
static unsigned char packet_marks[2048] = { 1, 2, 3, 4 };
static int packet_step(Packet *packet, unsigned position)
{
    unsigned index = (packet->index + position) & 1023;
    int value = packet_medium[index] + packet_weights[position & 511];
    if (value & 1) value += packet_small[position % 17];
    else value -= packet_marks[position & 2047];
    packet_medium[index] = (unsigned char)value;
    return value;
}
static int packet_mix(Packet *packet, int value)
{
    unsigned position = (packet->index + (unsigned)value) % 70000;
    int prior = packet_large[position];
    packet_large[position] = (unsigned char)(value ^ prior);
    return value + prior + packet_external(packet, position);
}
__declspec(dllexport) int packet_process(Packet *packet, unsigned count)
{
    unsigned i;
    int total = packet_small[packet->index % 17];
    for (i = 0; i < count; ++i) {
        int step = packet_step(packet, i);
        total = packet_mix(packet, total + step);
        packet_small[i % 17] ^= total;
    }
    return total;
}
