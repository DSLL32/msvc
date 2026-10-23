#ifndef DISPATCH_PROTOCOL_H
#define DISPATCH_PROTOCOL_H
struct PendingPacket;
struct DispatchState { unsigned sequence; int status; };
typedef int (__cdecl *DispatchCallback)(int);
int __cdecl dispatch_frame(int);
int __cdecl validate_header(struct PendingPacket *, int *, DispatchCallback);
#endif
