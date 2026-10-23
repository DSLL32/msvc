struct Packet {
    unsigned char payload[4];
    const unsigned marker;
    volatile unsigned flags;
};
typedef unsigned (__stdcall PacketCallback)(const struct Packet *, unsigned);
typedef PacketCallback *CallbackPointer;
typedef const unsigned *ReadOnlyWord;
typedef ReadOnlyWord WordList[2];
typedef WordList *WordListPointer;

static unsigned __stdcall sum_packet(const struct Packet *packet, unsigned seed)
{
    unsigned i;
    for (i = 0; i < 4; ++i) seed += packet->payload[i];
    return seed + packet->marker;
}

static unsigned __stdcall weighted_packet(const struct Packet *packet, unsigned seed)
{
    return seed + packet->payload[0] * 2 + packet->payload[3] * 3;
}

static unsigned dispatch(CallbackPointer callback, const struct Packet *packet,
                         unsigned seed)
{
    return (*callback)(packet, seed);
}

int main(void)
{
    struct Packet packet = { { 3, 5, 7, 11 }, 19, 0 };
    unsigned char *view = &packet;
    const unsigned *read_only = &packet.marker;
    unsigned *plain = read_only;
    unsigned wrapped = -17U;
    unsigned other = 23, sum = 0, i;
    WordList words = { &packet.marker, &other };
    WordListPointer list = &words;
    CallbackPointer callbacks[2] = { sum_packet, weighted_packet };

    for (i = 0; i < 4; ++i) sum += view[i];
    if (sum != 26 || *plain != 19 || (*list)[1][0] != 23) return 1;
    if (wrapped + 17U != 0) return 2;
    packet.flags = sum;
    if (packet.flags != 26) return 3;
    if (dispatch(callbacks[0], &packet, 13) != 58) return 4;
    if (dispatch(callbacks[1], &packet, 7) != 46) return 5;
    return 0;
}
