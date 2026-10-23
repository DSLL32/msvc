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
    static Counter cached(3);
    return cached.add();
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
        if (live != 2 || Counter::entries != 2) return 4;
    }
    return live != 1 || destroyed != 1;
}
