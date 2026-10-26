#include <new.h>
#include <recon.h>
extern "C" void __cxa_pure_virtual(void) __attribute__((noreturn));
struct Abstract {
    RECON_FORCEINLINE Abstract() {}
    virtual int RECON_THISCALL Value() const = 0;
};
struct Concrete : Abstract {
    int value;
    RECON_FORCEINLINE Concrete() : value(7) {}
    int RECON_THISCALL Value() const;
};
int RECON_THISCALL Concrete::Value() const { return value; }
extern "C" void *__attribute__((noinline)) make_object(void *storage)
{ return new(storage) Concrete; }
static void (__cdecl *public_purecall)(void)=_purecall;
int main(int argc,char **argv)
{
    unsigned storage[2];
    if(argc>1){if(argv[1][0]=='p')public_purecall();__cxa_pure_virtual();}
    Abstract *object=(Abstract *)make_object(storage);
    return object==(void *)storage && object->Value()==7 ? 0 : 1;
}
