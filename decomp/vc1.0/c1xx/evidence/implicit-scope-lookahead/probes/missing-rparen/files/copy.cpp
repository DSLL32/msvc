int alive, copied, assigned;
class Value {
    int value_;
public:
    Value(int value);
    Value(const Value &other);
    Value &operator=(const Value &other);
    Value operator+(const Value &other) const;
    operator int() const;
    ~Value();
};
Value::Value(int value) : value_(value) { ++alive; }
Value::Value(const Value &other) : value_(other.value_) { ++alive; ++copied; }
Value &Value::operator=(const Value &other)
{
    if (this != &other value_ = other.value_;
    ++assigned;
    return *this;
}
Value Value::operator+(const Value &other) const { return Value(value_ + other.value_); }
Value::operator int() const { return value_; }
Value::~Value() { --alive; }
int main()
{
    int result;
    {
        Value a(12), b(a), c(3);
        c = b;
        a = a + Value(5);
        b = b;
        result = int(a) == 17 && int(b) == 12 && int(c) == 12 &&
            copied >= 1 && assigned == 3 ? 0 : 1;
    }
    return result || alive != 0;
}
