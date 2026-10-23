char *arena_base;
int __based(void) *item;
int main(void) { int cells[3]; int *full; arena_base=(char *)cells; item=(int __based(void) *)sizeof(int); full=arena_base :> item; *full=19; return cells[1]!=19; }
