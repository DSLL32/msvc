#include <stddef.h>
#include <new.h>
#include <recon.h>
typedef char NativeSizeTIs32[sizeof(size_t)==4?1:-1];
static unsigned entries;
extern "C" void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *,void *)
{ ++entries; }
extern "C" void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *,void *) {}
struct InlineBase {
    RECON_FORCEINLINE InlineBase() {}
    virtual unsigned RECON_THISCALL Value() const;
};
struct InlineItem : InlineBase {
    unsigned word;
    RECON_FORCEINLINE InlineItem(unsigned value) : word(value) {}
    unsigned RECON_THISCALL Value() const;
};
typedef char MustBeEightBytes[sizeof(InlineItem)==8?1:-1];
unsigned RECON_THISCALL InlineBase::Value() const { return 0; }
unsigned RECON_THISCALL InlineItem::Value() const { return word; }
extern "C" void *__attribute__((noinline)) construct(void *storage,unsigned value)
{ return new(storage) InlineItem(value); }
/* Compile/link edge only: no new allocation semantics are exercised here. */
extern "C" void *__attribute__((noinline)) allocate(unsigned size)
{ return ::operator new(size); }
int main(void)
{
    unsigned storage[2];
    InlineBase *item=(InlineBase *)construct(storage,42);
    unsigned value=item->Value();
    return item==(void *)storage && value==42 && entries==3 ? 0 : 1;
}
