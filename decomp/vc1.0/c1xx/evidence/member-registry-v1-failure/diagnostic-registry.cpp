/* A self-checking callback registry spanning all four member-pointer models.
   Virtual inheritance is separate from the nonvirtual conversion spine:
   Flat -> Offset -> Virtual -> Twin -> General -> Further. */
#include "registry.h"
extern "C" int __cdecl printf(const char *, ...);
static void dump(const char *name,const void *value,unsigned bytes) { const unsigned *word=(const unsigned *)value; unsigned i; printf("%s size%u",name,bytes); for(i=0;i<bytes/4;++i)printf(" %08x",word[i]);printf("\n"); }

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
    int virtual_only;
    Virtual() : virtual_only(23) {}
    virtual int project(int bias) const { return 3 * value + bias; }
    virtual int score(int amount) { return root + amount; }
    int reduce(int amount) { value -= amount; return value; }
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
    int own_field;
    General() : own_field(53) {}
    virtual int project(int bias) const { return 4 * value + bias; }
    int own(int amount) { return value + own_field + amount; }
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
    VirtualAction virtual_flat = flat;
    VirtualAction virtual_offset = offset;
    TwinAction twin = virtual_offset;
    GeneralAction general_flat = flat;
    GeneralAction general_offset = offset;
    GeneralAction general = virtual_offset;
    GeneralAction general_twin = twin;
    FurtherAction further = general;
    FlatAction back_flat = (FlatAction)virtual_offset;
    OffsetAction back_offset = (OffsetAction)general;
    VirtualAction back_virtual = (VirtualAction)general;
    GeneralAction back_general = (GeneralAction)further;
    VirtualAction virtual_own = &Virtual::reduce;
    GeneralAction general_own = &General::own;
    FlatAction flat_null = 0;
    VirtualAction virtual_null = flat_null;
    GeneralAction general_null = virtual_null;
    VirtualProjection virtual_project = &Virtual::project;
    GeneralProjection general_project = virtual_project;
    RootAction root_score = &Root::score;

    int Flat::*flat_slot = &Flat::value;
    int Offset::*offset_slot = flat_slot;
    int Virtual::*virtual_flat_slot = flat_slot;
    int Virtual::*virtual_slot = offset_slot;
    int Twin::*twin_slot = virtual_slot;
    int General::*general_flat_slot = flat_slot;
    int General::*general_offset_slot = offset_slot;
    int General::*general_slot = virtual_slot;
    int General::*general_twin_slot = twin_slot;
    int Further::*further_slot = general_slot;
    int Flat::*back_flat_slot = (int Flat::*)virtual_slot;
    int Offset::*back_offset_slot = (int Offset::*)general_slot;
    int Virtual::*back_virtual_slot = (int Virtual::*)general_slot;
    int General::*back_general_slot = (int General::*)further_slot;
    int Flat::*flat_null_slot = 0;
    int Virtual::*virtual_null_slot = flat_null_slot;
    int General::*general_null_slot = virtual_null_slot;

    printf("equal_flat_offset=%d distinct_own=%d\n",virtual_flat == virtual_offset, virtual_offset != virtual_own);
    dump("flat",&flat,sizeof(flat)); dump("offset",&offset,sizeof(offset)); dump("virtual_flat",&virtual_flat,sizeof(virtual_flat)); dump("virtual_offset",&virtual_offset,sizeof(virtual_offset)); dump("virtual_own",&virtual_own,sizeof(virtual_own));
    if (!(virtual_flat == virtual_offset)) return 110;
    if (!(virtual_offset != virtual_own)) return 111;
    CHECK(11, general_flat == general && general_offset == general_twin);
    CHECK(12, back_flat == flat && back_offset == offset);
    CHECK(13, back_virtual == virtual_offset && back_general == general);
    CHECK(14, virtual_null == 0 && general_null == 0);
    CHECK(15, virtual_offset != virtual_null && general != general_null);
    CHECK(16, virtual_flat_slot == virtual_slot && general_flat_slot == general_slot);
    CHECK(17, general_offset_slot == general_twin_slot);
    CHECK(18, back_flat_slot == flat_slot && back_offset_slot == offset_slot);
    CHECK(19, back_virtual_slot == virtual_slot && back_general_slot == general_slot);
    CHECK(20, virtual_null_slot == 0 && general_null_slot == 0);
    CHECK(21, virtual_slot != virtual_null_slot && general_slot != general_null_slot);
    CHECK(22, general_slot != &General::own_field);
    CHECK(23, (object.*general)(2) == 7);
    CHECK(24, (object.*twin)(3) == 10);
    CHECK(25, (object.*further)(4) == 14);
    CHECK(26, object.*general_slot == 14 && object.*further_slot == 14);
    object.*twin_slot = 12;
    CHECK(27, (object.*general_project)(1) == 49);
    CHECK(28, (object.*root_score)(2) == 19 && object.tag() == 31);
    CHECK(29, (object.*general_own)(1) == 66);
    CHECK(30, (object.*choose_virtual(virtual_offset))(1) == 13);
    CHECK(31, (object.*choose_general(general))(1) == 14 && choices == 2);
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
