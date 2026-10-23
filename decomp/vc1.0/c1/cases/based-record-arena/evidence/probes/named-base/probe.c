char *arena_base;
int __based(arena_base) *item;
int main(void) { int cells[3]; arena_base=(char *)cells; item=(int __based(arena_base) *)sizeof(int); item[0]=17; return cells[1]!=17; }
