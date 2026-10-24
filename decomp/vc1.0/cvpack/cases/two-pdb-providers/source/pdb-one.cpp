enum Tone1 { tone1first=1, tone1last=9 };
struct Node1 { Node1 *next; int values[4]; Tone1 tone; };
class Shape1 {
public:
    virtual int read();
    Node1 chain;
    int member;
};
int Shape1::read() { return chain.values[0]+chain.values[3]+member; }
extern "C" __declspec(dllexport) int __stdcall provider_one(Shape1 *value)
{
    int Shape1::*field=&Shape1::member;
    return value->read()+value->*field+value->chain.tone;
}
