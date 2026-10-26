#include "cpp_types.h"
int Number::count = 3;
Number global_number(17);
ScaledNumber global_scaled(11,4);
int use_numbers( Number &a, const Number &b ) { return a.evaluate(b) + a[0]; }
Number combine_numbers( const Number &a, const Number &b ) { return a+b; }
Bucket<int> global_bucket(13);
int use_bucket( int n ) { return global_bucket.add(n) + global_bucket.get(); }
int Number::* member_pointer = 0;
#define PUBLIC(n) extern "C" int public_##n( int x ) { return x + n; }
#define TEN(n) PUBLIC(n##0) PUBLIC(n##1) PUBLIC(n##2) PUBLIC(n##3) PUBLIC(n##4) PUBLIC(n##5) PUBLIC(n##6) PUBLIC(n##7) PUBLIC(n##8) PUBLIC(n##9)
TEN(10) TEN(11) TEN(12) TEN(13) TEN(14) TEN(15) TEN(16) TEN(17) TEN(18) TEN(19)
TEN(20) TEN(21) TEN(22) TEN(23) TEN(24) TEN(25) TEN(26) TEN(27) TEN(28) TEN(29)
