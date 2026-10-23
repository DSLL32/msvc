struct Duplicate {
    int member;
    int member;
};

int negative_bound[-3];
int conflicting_declaration;
double conflicting_declaration;
int declared_call(int first, double second);

int broken_initializer(int value)
{
    int local = ;
    local = missing_object + value;
    return declared_call(local);
}

int implicit_call(int value)
{
    return missing_function(value);
}

int recovered_tail(int value)
{
    struct TailRecord { int left; int right; } tail;
    tail.left = value;
    tail.right = value + 7;
    return tail.left + tail.right;
}
#pragma message("RECOVERY_TAIL_REACHED")
