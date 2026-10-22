/* Original-toolchain symbol fixture; no reconstruction implementation. */
enum Shade { light, dark };
class Widget {
    int value;
public:
    static long count;
    Widget(int);
    virtual ~Widget();
    virtual int compute(int) const;
    int operator+(const Widget &) const;
    operator long() const;
    static int twice(int);
};
long Widget::count=0;
Widget::Widget(int n):value(n) { ++count; }
Widget::~Widget() { --count; }
int Widget::compute(int n) const { return value+n; }
int Widget::operator+(const Widget &other) const { return value+other.value; }
Widget::operator long() const { return value; }
int Widget::twice(int n) { return n*2; }
class Gadget:public Widget {
public:
    Gadget(int);
    virtual int compute(int) const;
};
Gadget::Gadget(int n):Widget(n) {}
int Gadget::compute(int n) const { return Widget::compute(n)+1; }
class IntBox {
public:
    int values[3];
    int sum(int (*transform)(int)) const {
        int result=0;
        for (int i=0;i<3;++i) result+=transform(values[i]);
        return result;
    }
};
int overload(int &n) { return n; }
long overload(const char *p) { return *p; }
int overload(Shade s) { return (int)s; }
int __stdcall stdcall_function(int a,const volatile long *p) { return a+(int)*p; }
int __fastcall fastcall_function(int a,int b) { return a+b; }
typedef int (__cdecl *Transform)(int);
Transform return_transform(Transform f) { return f; }
int member_pointer(Widget &w,int (Widget::*method)(int) const) { return (w.*method)(3); }
int array_reference(const int (&a)[3]) { return a[0]+a[2]; }
int use_names(void) {
    Widget w(2); Gadget g(4); IntBox b;
    b.values[0]=1; b.values[1]=2; b.values[2]=3;
    return b.sum(&Widget::twice)+w+g+member_pointer(w,&Widget::compute);
}
