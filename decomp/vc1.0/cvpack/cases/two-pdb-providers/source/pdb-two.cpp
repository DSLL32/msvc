enum Tone2 { tone2first=1, tone2last=9 };
struct Node2 { Node2 *next; int values[4]; Tone2 tone; };
class Shape2 {
public:
    virtual int read();
    Node2 chain;
    int member;
};
int Shape2::read() { return chain.values[0]+chain.values[3]+member; }
extern "C" __declspec(dllexport) int __stdcall provider_two(Shape2 *value)
{
    int Shape2::*field=&Shape2::member;
    return value->read()+value->*field+value->chain.tone;
}
