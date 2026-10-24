/* A self-checking callback registry spanning all four member-pointer models.
   Only compact Flat/Offset values cross representations. Virtual/general
   member pointers name their own class members directly; the original
   compiler's failed widening case is preserved separately in failed-v1. */
#include "registry.h"

static int live_slots;
static int choices;

class __single_inheritance Root {
public:
    int root;
    Root() : root(17) {}
    virtual int score(int value) = 0;
};
class __single_inheritance Prefix {
public:
    int prefix;
    Prefix() : prefix(31) {}
    virtual int tag() const { return prefix; }
};
class __single_inheritance Flat {
public:
    int value;
    Flat() : value(BASE_SEED) {}
    int add(int amount) { value += amount; return value; }
    int subtract(int amount) { return value - amount; }
};
class __multiple_inheritance Offset : public Prefix, public Flat {
public:
    int offset_only;
    Offset() : offset_only(19) {}
};
class __virtual_inheritance Virtual : public Offset, virtual public Root {
public:
    int virtual_only, virtual_spare;
    Virtual() : virtual_only(23), virtual_spare(27) {}
    virtual int project(int bias) const { return 3 * value + bias; }
    virtual int score(int amount) { return root + amount; }
    int reduce(int amount) { value -= amount; return value; }
    int inspect(int amount) { return value + amount; }
};
class __single_inheritance Extra {
public:
    int extra;
    Extra() : extra(29) {}
    virtual int label() const { return extra; }
};
class __virtual_inheritance Twin : public Extra, public Virtual {
public:
    int twin_only;
    Twin() : twin_only(37) {}
};
/* /vmg gives the unspecified General/Further classes model four. The
   explicitly annotated bases retain their single/multiple/virtual models. */
class General : public Twin {
public:
    int own_field, own_spare;
    General() : own_field(53), own_spare(59) {}
    virtual int project(int bias) const { return 4 * value + bias; }
    int own(int amount) { return value + own_field + amount; }
    int other(int amount) { return value - own_field + amount; }
};
class __single_inheritance ExtraFirst {
public:
    int extra_first;
    ExtraFirst() : extra_first(41) {}
    virtual int first() const { return extra_first; }
};
class Further : public ExtraFirst, public General {
public:
    int further_only;
    Further() : further_only(43) {}
};

ACTION_TYPE(Flat,Flat);
ACTION_TYPE(Offset,Offset);
ACTION_TYPE(Virtual,Virtual);
ACTION_TYPE(Twin,Twin);
ACTION_TYPE(General,General);
ACTION_TYPE(Further,Further);
ACTION_TYPE(Root,Root);
typedef int (Virtual::*VirtualProjection)(int) const;
typedef int (General::*GeneralProjection)(int) const;

static VirtualAction choose_virtual(VirtualAction action)
{
    ++choices;
    return action;
}
static GeneralAction choose_general(GeneralAction action)
{
    ++choices;
    return action;
}

/* The containing class's implicit copy/assignment must handle member arrays.
   These are automatic objects, with no synthetic local-static initializer. */
class Slot {
public:
    int value;
    Slot() : value(2) { ++live_slots; }
    Slot(const Slot &other) : value(other.value) { ++live_slots; }
    ~Slot() { --live_slots; }
};
class Registry {
public:
    struct Meta { int count; };
    Slot slots[2];
    Meta meta;
    Registry() { meta.count = 2; }
};

static int exercise_registry()
{
    Further object;
    FlatAction flat = &Flat::add;
    OffsetAction offset = flat;
    OffsetAction offset_other = &Flat::subtract;
    FlatAction back_flat = (FlatAction)offset;
    FlatAction flat_null = 0;
    OffsetAction offset_null = flat_null;
    VirtualAction virtual_own = &Virtual::reduce;
    VirtualAction virtual_other = &Virtual::inspect;
    VirtualAction virtual_copy = virtual_own;
    VirtualAction virtual_null = 0;
    GeneralAction general_own = &General::own;
    GeneralAction general_other = &General::other;
    GeneralAction general_copy = general_own;
    GeneralAction general_null = 0;
    VirtualProjection virtual_project = &Virtual::project;
    GeneralProjection general_project = &General::project;
    RootAction root_score = &Root::score;

    int Flat::*flat_slot = &Flat::value;
    int Offset::*offset_slot = flat_slot;
    int Flat::*back_flat_slot = (int Flat::*)offset_slot;
    int Flat::*flat_null_slot = 0;
    int Offset::*offset_null_slot = flat_null_slot;
    int Virtual::*virtual_slot = &Virtual::virtual_only;
    int Virtual::*virtual_other_slot = &Virtual::virtual_spare;
    int Virtual::*virtual_copy_slot = virtual_slot;
    int Virtual::*virtual_null_slot = 0;
    int General::*general_slot = &General::own_field;
    int General::*general_other_slot = &General::own_spare;
    int General::*general_copy_slot = general_slot;
    int General::*general_null_slot = 0;

    CHECK(10, back_flat == flat && offset != offset_other && offset != offset_null);
    CHECK(11, back_flat_slot == flat_slot && offset_slot != offset_null_slot);
    CHECK(12, virtual_own == virtual_copy && virtual_own != virtual_other);
    CHECK(13, general_own == general_copy && general_own != general_other);
    CHECK(14, virtual_null == 0 && general_null == 0);
    CHECK(15, virtual_own != virtual_null && general_own != general_null);
    CHECK(16, virtual_slot == virtual_copy_slot && virtual_slot != virtual_other_slot);
    CHECK(17, general_slot == general_copy_slot && general_slot != general_other_slot);
    CHECK(18, virtual_null_slot == 0 && general_null_slot == 0);
    CHECK(19, virtual_slot != virtual_null_slot && general_slot != general_null_slot);
    CHECK(20, (object.*offset)(2) == 7 && (object.*flat)(3) == 10);
    CHECK(21, (object.*virtual_own)(2) == 8 && (object.*general_own)(1) == 62);
    CHECK(22, object.*virtual_slot == 23 && object.*general_slot == 53);
    object.*virtual_slot = 29;
    object.*general_slot = 55;
    CHECK(23, object.*virtual_slot == 29 && object.*general_slot == 55);
    CHECK(24, (object.*virtual_project)(1) == 33);
    CHECK(25, (object.*general_project)(2) == 34);
    CHECK(26, (object.*root_score)(2) == 19 && object.tag() == 31);
    CHECK(27, (object.*choose_virtual(virtual_own))(1) == 7);
    CHECK(28, (object.*choose_general(general_own))(1) == 63 && choices == 2);
    return 0;
}

int main()
{
    RegistryHeader header;
    const char *name = TEXT(REGISTRY_VERSION);
    int result;
    CHECK(1, sizeof(header) == 6 && name[0] == '3' && name[1] == 0);
    {
        Registry first;
        CHECK(2, live_slots == 2 && first.meta.count == 2);
        first.slots[0].value = 7;
        {
            Registry second(first);
            CHECK(3, live_slots == 4 && second.slots[0].value == 7);
            second.slots[1].value = 11;
            first = second;
            CHECK(4, first.slots[1].value == 11 && live_slots == 4);
        }
        CHECK(5, live_slots == 2);
        result = exercise_registry();
        if (result) return result;
    }
    return live_slots != 0 || choices != 2;
}
