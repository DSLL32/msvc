/* Original-toolchain input fixture: embedded types, symbols and source lines. */
struct Node {
    Node *next;
    int value;
    unsigned short tags[5];
};

enum LargeCode { CodeNegative = -40000L, CodeLarge = 70000L, CodeMask = 0x12345678L };
typedef int (__cdecl *Transform)(int, int);

class Base {
protected:
    int seed;
public:
    Base(int value);
    virtual ~Base();
    virtual int evaluate(int value) const;
    int get_seed() const;
};
class Left : virtual public Base {
public:
    Left(int value);
    virtual int left_value() const;
};
class Right : virtual public Base {
public:
    Right(int value);
    virtual int right_value() const;
};
class Derived : public Left, public Right {
    Node head;
    int values[7];
    Transform transform;
public:
    Derived(int value, Transform fn);
    virtual ~Derived();
    virtual int evaluate(int value) const;
    int alter(int value);
};

typedef int (Base::*ConstMember)(int) const;
typedef int (Derived::*Member)(int);

Base::Base(int value) : seed(value) {}
Base::~Base() { seed = -1; }
int Base::evaluate(int value) const { return seed + value; }
int Base::get_seed() const { return seed; }
Left::Left(int value) : Base(value) {}
int Left::left_value() const { return get_seed() + 2; }
Right::Right(int value) : Base(value) {}
int Right::right_value() const { return get_seed() + 3; }

static int combine(int a, int b)
{
    const long negative = CodeNegative;
    const unsigned long mask = CodeMask;
    return a + b + (int)(mask & 7) + (int)(negative & 3);
}

Derived::Derived(int value, Transform fn) : Base(value), Left(value), Right(value)
{
    int i;
    head.next = &head;
    head.value = value;
    for (i = 0; i < 5; ++i) head.tags[i] = (unsigned short)(i + CodeLarge);
    for (i = 0; i < 7; ++i) values[i] = value + i;
    transform = fn;
}
Derived::~Derived() { head.next = 0; }
int Derived::evaluate(int value) const
{
    Base const *base = this;
    int local = head.value + values[value % 7];
    return transform(local, base->get_seed()) + left_value() + right_value();
}
int Derived::alter(int value)
{
    int previous = values[0];
    values[0] = value;
    return previous;
}

static int local_counter = 11;
extern "C" __declspec(dllexport) int __stdcall evaluate_model(int value)
{
    Derived object(value, combine);
    Base *base = &object;
    ConstMember read = &Base::evaluate;
    Member change = &Derived::alter;
    Node first, second;
    LargeCode code = CodeLarge;
    int result;
    first.next = &second; first.value = value;
    second.next = &first; second.value = value + 1;
    result = (object.*change)(first.next->value);
    result += (base->*read)(value % 7);
    result += (int)code + local_counter++;
    return result;
}

extern "C" int __stdcall DllMain(void *module, unsigned long reason, void *reserved)
{
    if (reason == 1) return evaluate_model(5) != 0;
    return 1;
}
