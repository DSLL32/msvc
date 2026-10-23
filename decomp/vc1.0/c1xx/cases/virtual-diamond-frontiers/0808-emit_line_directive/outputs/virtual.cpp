int destroyed;
class Base {
protected:
    int value_;
public:
    Base(int value);
    virtual int value();
    virtual ~Base();
};
class Left : virtual public Base {
public:
    Left(int value);
    int left();
    virtual ~Left();
};
class Right : virtual public Base {
public:
    Right(int value);
    int right();
    virtual ~Right();
};
class Derived : public Left, public Right {
public:
    Derived(int value);
    virtual int value();
    virtual ~Derived();
};
Base::Base(int value) : value_(value) {}
int Base::value() { return value_; }
Base::~Base() { destroyed |= 1; }
Left::Left(int value) : Base(value) {}
int Left::left() { return value_; }
Left::~Left() { destroyed |= 2; }
Right::Right(int value) : Base(value) {}
int Right::right() { return value_; }
Right::~Right() { destroyed |= 4; }
Derived::Derived(int value) : Base(value), Left(value), Right(value) {}
int Derived::value() { return left() + right() + 2; }
Derived::~Derived() { destroyed |= 8; }
int main()
{
    int result;
    {
        Derived object(20);
        Left *left = &object;
        Right *right = &object;
        Base *from_left = left;
        Base *from_right = right;
        result = from_left == from_right && from_left->value() == 42 &&
            right->right() == 20 ? 0 : 1;
    }
    return result || destroyed != 15;
}
