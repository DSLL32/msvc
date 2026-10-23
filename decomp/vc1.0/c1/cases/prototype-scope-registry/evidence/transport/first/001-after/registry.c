/* Typed packet registry: prototype-scope tag binding and callback adapters. */
#include "registry.h"
static int __cdecl sum_packet(struct RegistryPacket *packet, const int (*rows)[3])
{
    int index, sum = packet->bias;
    for (index = 0; index < 3; ++index)
        sum += rows[packet->identifier & 1][index];
    return sum;
}
int __cdecl registry_sum(struct RegistryPacket *packet, const int (*rows)[3])
{
    PacketCallback selected = packet->callback;
    return (*selected)(packet, rows);
}
int __cdecl packet_checksum(struct RegistryPacket *packet, const int (*rows)[3])
{
    return registry_sum(packet, rows) + (int)packet->identifier;
}
static PacketCallback __cdecl choose_callback(unsigned identifier)
{
    return identifier ? sum_packet : packet_checksum;
}
CallbackFactory __cdecl registry_factory(void)
{
    return choose_callback;
}
int main(void)
{
    static const int rows[2][3] = {{3, 5, 7}, {11, 13, 17}};
    struct RegistryPacket packets[2];
    CallbackFactory factory = registry_factory();
    int first, second;
    packets[0].identifier = 0;
    packets[0].bias = 4;
    packets[0].callback = (*factory)(1);
    packets[1].identifier = 1;
    packets[1].bias = -6;
    packets[1].callback = (*factory)(2);
    first = packet_checksum(&packets[0], rows);
    second = packet_checksum(&packets[1], rows);
    return first != 19 || second != 36;
}
