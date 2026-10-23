#ifndef DAMAGED_CALLBACK_DISPATCH_H
#define DAMAGED_CALLBACK_DISPATCH_H
typedef int (__cdecl *DispatchCallback)(unsigned);
struct DispatchRecord { unsigned identifier; DispatchCallback callback; };
#endif
