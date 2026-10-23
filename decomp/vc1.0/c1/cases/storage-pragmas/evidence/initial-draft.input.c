extern int __cdecl abs(int);
#pragma function(abs)
#pragma intrinsic(abs)

#pragma pack(push, outer_packet, 2)
struct Packet {
    unsigned char marker;
    unsigned sequence;
    unsigned low : 5;
    unsigned high : 7;
    int values[3];
};
#pragma pack(push, inner_packet, 1)
struct Tiny {
    unsigned char tag;
    unsigned value;
};
#pragma pack(pop, inner_packet)
#pragma pack(pop, outer_packet)

struct ThreadState { int calls; int checksum; };
__declspec(thread) int thread_bias = 3;
__declspec(thread) struct ThreadState thread_state = { 0, 11 };

int __cdecl accumulate(int input);
#pragma alloc_text(CHECKSUM_TEXT, accumulate)

int __cdecl accumulate(int input)
{
    static int total = 5;
    static struct Tiny remembered = { 2, 9 };
    struct Packet packet = { 7, input + 1, input & 31, (input + 2) & 127,
                             { input, input + 3, input + 5 } };
    struct Tiny copy = { 4, input + 9 };
    int value = abs(-input);
    total += value;
    remembered.value += copy.tag;
    thread_state.calls++;
    thread_state.checksum += packet.values[0] + packet.values[2];
    return total + remembered.value + packet.sequence + packet.low +
           packet.high + thread_bias + thread_state.checksum;
}

int main(void)
{
    int first = accumulate(6);
    int second = accumulate(9);
    return sizeof(struct Packet) != 22 || sizeof(struct Tiny) != 5 ||
           first != 76 || second != 119 || thread_state.calls != 2;
}
