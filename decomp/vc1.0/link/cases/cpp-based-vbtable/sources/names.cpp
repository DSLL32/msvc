int *based_base;
int __based(based_base) *based_pointer;
class Base { public: int value; };
class Derived : virtual public Base { public: int add(int); };
int Derived::add(int argument) { return value+argument; }
Derived object;
