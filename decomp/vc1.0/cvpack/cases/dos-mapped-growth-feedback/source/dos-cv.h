enum Colour { red, green, blue };
union Value { long number; char text[8]; };
struct Node {
    struct Node __near *next;
    union Value value;
    enum Colour colour;
    short samples[5];
};
extern struct Node roots[3];
extern long total;
int __near visit(struct Node __near *, int);
int __far combine(struct Node __far *, short);
