struct Link { int value; struct Link __based(__self) *next; };
struct Link first;
int main(void) { first.value=21; return first.value!=21; }
