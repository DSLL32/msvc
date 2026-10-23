#include <new.h>
void *__cdecl operator new(size_t);
void __cdecl operator delete(void *);
int alive, destroyed, allocated, released;
class Item {
    int values_[3];
public:
    Item();
    ~Item();
    int &operator[](int index);
    static void *operator new(size_t bytes);
    static void operator delete(void *memory);
};
Item::Item() { ++alive; values_[0] = 1; values_[1] = 2; values_[2] = 3; }
Item::~Item() { --alive; ++destroyed; }
int &Item::operator[](int index) { return values_[index]; }
void *Item::operator new(size_t bytes) { ++allocated; return ::operator new(bytes); }
void Item::operator delete(void *memory) { ++released; ::operator delete(memory); }
int main()
{
    Item *one = new Item;
    (*one)[1] = 42;
    int result = (*one)[1] == 42 ? 0 : 1;
    delete one;
    Item *many = new Item[3];
    many[2][0] = 17;
    if (many[2][0] != 17) result = 2;
    delete [] many;
    return result || alive != 0 || destroyed != 4 || allocated != 1 || released != 1;
}
