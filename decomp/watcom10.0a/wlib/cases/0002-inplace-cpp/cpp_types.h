#ifndef CPP_TYPES_H
#define CPP_TYPES_H
class Number {
    int value;
public:
    Number( int n = 0 ) : value(n) {}
    virtual ~Number() {}
    virtual int evaluate( int n ) const { return value + n; }
    Number operator+( const Number &rhs ) const { return Number(value + rhs.value); }
    int &operator[]( int ) { return value; }
    operator int() const { return value; }
    static int count;
};
class ScaledNumber : public Number {
    int scale;
public:
    ScaledNumber( int n, int s ) : Number(n), scale(s) {}
    virtual ~ScaledNumber() {}
    virtual int evaluate( int n ) const { return Number::evaluate(n) * scale; }
};
template<class T> class Bucket {
    T value;
public:
    Bucket( T n ) : value(n) {}
    T get() const { return value; }
    T add( T n ) { value += n; return value; }
};
#endif
