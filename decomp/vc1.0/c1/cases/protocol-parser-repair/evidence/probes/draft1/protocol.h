#ifndef PROTOCOL_H
#define PROTOCOL_H
/* Deliberately damaged declarations from a generated protocol interface. */
typedef struct WireHeader {
    unsigned length
    unsigned flags;
} WireHeader;

int export_route stray (int value);
struct Pending;
int packet_extent(struct Pending item) { return 0; }

int codec;
int codec(int value) { return value; }
#endif
