int order[16], count, next_id = 40;
class Guard {
    int id_;
public:
    Guard();
    Guard(int id);
    ~Guard();
    int id() const;
};
Guard::Guard() : id_(next_id++) {}
Guard::Guard(int id) : id_(id) {}
Guard::~Guard() { order[count++] = id_; }
int Guard::id() const { return id_; }
int consume(const Guard &guard) { return guard.id(); }
int leave_early(int value)
{
    Guard guard(6);
    if (value) return guard.id();
    return 0;
}
int main()
{
    {
        Guard outer(1);
        {
            Guard inner(2);
            if (consume(Guard(3)) != 3) return 1;
        }
    }
    {
        Guard array[3];
        if (array[0].id() != 40 || array[2].id() != 42) return 2;
    }
    if (leave_early(1) != 6 || count != 7) return 3;
    return order[0] != 3 || order[1] != 2 || order[2] != 1 ||
        order[3] != 42 || order[4] != 41 || order[5] != 40 || order[6] != 6;
}
