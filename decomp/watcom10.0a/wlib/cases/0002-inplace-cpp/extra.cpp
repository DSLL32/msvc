#include "cpp_types.h"
Bucket<long> long_bucket(0x123456L);
Bucket<int> int_bucket(29);
int overloaded( int n ) { return n + int_bucket.get(); }
long overloaded( long n ) { return long_bucket.add(n); }
int (*overload_address)(int) = overloaded;
Number &reference_to_number( Number &a ) { return a; }
int call_virtual( const Number *p ) { return p->evaluate(5); }
