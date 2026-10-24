/* One self-checking workload: interpret a small command table through ordinary
   and virtual member pointers, and verify cleanup after a scoped jump. */
extern "C" {
    int __cdecl abs(int);
}

static int live;
static int destroyed;
static int marks;

enum CommandCode { Add = 1, Multiply, Skip, Stop };
struct Command {
    unsigned code : 4;
    unsigned repeat : 4;
    int operand;
};
static Command program[] = {
    { Add, 2, 3 }, { Skip, 1, 99 }, { Multiply, 1, 2 }, { Stop, 0, 0 }
};
union Number {
    long integer;
    double real;
};

class Counter {
    int secret;
public:
    int value;
    static int entries;
    Counter(int initial = 1) : secret(7), value(initial) { ++live; ++entries; }
    virtual ~Counter() { --live; ++destroyed; }
    int add(int amount = 1) { value += amount; return value; }
    int multiply(int amount) { value *= amount; return value; }
    virtual int project(int bias) const { return value + bias; }
    operator int() const { return value; }
    int operator+(int bias) const { return value + bias; }
    friend int inspect(const Counter &counter);
};
int Counter::entries;
int inspect(const Counter &counter) { return counter.secret; }

class Scaled : public Counter {
public:
    Scaled(int initial) : Counter(initial) {}
    virtual int project(int bias) const { return 2 * value + bias; }
};

class ScopeMark {
    int tag;
public:
    ScopeMark(int value) : tag(value) {}
    ~ScopeMark() { marks = marks * 10 + tag; }
};

class Proxy {
    Counter *object;
public:
    Proxy(Counter *value) : object(value) {}
    Counter *operator->() { return object; }
};

typedef int (Counter::*Action)(int);
typedef int (Counter::*Projection)(int) const;

static int interpret(Counter &counter, int extra = 2)
{
    Action add = &Counter::add;
    Action multiply = &Counter::multiply;
    Projection project = &Counter::project;
    int Counter::*slot = &Counter::value;
    int position, repetition, ignored = 0;
    int total = sizeof(program) / sizeof(program[0]);
    Number scratch;
    const char *label = "state" "\n";
    scratch.real = 1.5 * 2.0;
    if (scratch.real != 3.0 || label[5] != '\n') return -100;
    for (position = 0; position < total; ++position) {
        ScopeMark outer(1);
        switch (program[position].code) {
        case Add:
            repetition = 0;
            do {
                (counter.*add)(program[position].operand);
                ++repetition;
            } while (repetition < (int)program[position].repeat);
            break;
        case Multiply:
            (counter.*multiply)(program[position].operand);
            break;
        case Skip:
            while (ignored < 2) {
                ++ignored;
                if (ignored == 1) continue;
                break;
            }
            continue;
        default:
            {
                ScopeMark inner(2);
                goto finished;
            }
        }
    }
finished:
    if (marks != 11121 || ignored != 2) return -101;
    if (add == multiply || add == 0 || slot == 0) return -102;
    counter.*slot += extra;
    return (counter.*project)(inspect(counter));
}

static int cached_bias()
{
    static int cached = 3;
    return ++cached;
}


/* Two polymorphic bases require real member-pointer adjustments. */
class Prefix {
public:
    int prefix;
    Prefix() : prefix(41) {}
    virtual int tag() const { return prefix; }
};
class Shifted : public Prefix, public Counter {
public:
    Shifted(int initial) : Counter(initial) {}
    virtual int project(int bias) const { return 3 * value + bias; }
    int own(int amount) { value -= amount; return value; }
};
typedef int (Shifted::*ShiftAction)(int);
typedef int (Shifted::*ShiftProjection)(int) const;

static int exercise_shifted_members()
{
    Shifted value(4);
    Action base_add = &Counter::add;
    Projection base_project = &Counter::project;
    int Counter::*base_slot = &Counter::value;
    ShiftAction add = base_add;
    ShiftProjection project = base_project;
    int Shifted::*slot = base_slot;
    ShiftAction null_action = 0;
    int Shifted::*null_slot = 0;
    Action round_trip = (Action)add;
    int Counter::*round_slot = (int Counter::*)slot;
    Counter *base = &value;
    if (add == null_action || slot == null_slot || add == &Shifted::own) return 11;
    if ((value.*add)(3) != 7 || value.*slot != 7) return 12;
    if ((value.*project)(2) != 23 || value.tag() != 41) return 13;
    if (base->project(1) != 22) return 14;
    if (round_trip != base_add || round_slot != base_slot) return 15;
    return 0;
}

int main()
{
    int result, converted;
    {
        Scaled counter(1);
        Proxy proxy(&counter);
        int Counter::*base_slot = &Counter::value;
        int Scaled::*derived_slot = base_slot;
        result = interpret(counter);
        if (result != 39 || counter.*derived_slot != 16) return 1;
        proxy->add();
        converted = counter;
        if (converted != 17 || counter + 2 != 19) return 2;
        if (abs(-7) != 7 || cached_bias() != 4 || cached_bias() != 5) return 3;
        if (live != 1 || Counter::entries != 1) return 4;
    }
    if (live != 0 || destroyed != 1) return 5;
    result = exercise_shifted_members();
    if (result) return result;
    return live != 0 || destroyed != 2 || Counter::entries != 2;
}
