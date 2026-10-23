#ifndef BINDING_H
#define BINDING_H
#define BINDING_BIAS 11

enum BindingCode {
    binding_negative = -32769,
    binding_idle = -7,
    binding_ready = 13,
    binding_wide = 70000,
    binding_high = 0x80000010U
};
extern char *binding_base;
typedef struct BindingNode BindingNode;
typedef BindingNode __based(binding_base) *BindingLink;
struct BindingNode {
    BindingLink next;
    enum BindingCode code;
    unsigned active : 1;
    unsigned rank : 7;
    short coordinate[2];
    double sample;
};
typedef int (__cdecl *BindingCallback)(const BindingNode *, enum BindingCode);
union BindingStorage {
    double alignment;
    char bytes[256];
};
struct BindingTable {
    union BindingStorage arena;
    BindingLink first;
    BindingCallback dispatch;
    enum BindingCode last_code;
    const char *caption;
};
#pragma pack(2)
struct BindingCompact {
    char marker;
    unsigned short count;
    BindingLink owner;
};
#pragma pack()
extern struct BindingTable binding_table;
extern struct BindingCompact binding_compact;
extern enum BindingCode binding_request;
int binding_inspect(const BindingNode *, enum BindingCode);
int binding_initialize(void);
#endif
