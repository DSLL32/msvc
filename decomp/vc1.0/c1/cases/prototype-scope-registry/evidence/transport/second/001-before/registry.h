#ifndef PACKET_REGISTRY_H
#define PACKET_REGISTRY_H
/* The first tag reference is deliberately inside a prototype. */
int __cdecl registry_sum(struct RegistryPacket *, const int (*)[3]);
typedef int (__cdecl *PacketCallback)(struct RegistryPacket *, const int (*)[3]);
typedef PacketCallback (__cdecl *CallbackFactory)(unsigned);
struct RegistryPacket {
    unsigned identifier;
    int bias;
    PacketCallback callback;
};
int __cdecl packet_checksum(struct RegistryPacket *, const int (*)[3]);
CallbackFactory __cdecl registry_factory(void);
#endif
