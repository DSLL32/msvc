/* Authored build control: distinct class declarations, one physical body. */
#ifdef __linux__
#define MEMBER __attribute__((thiscall))
#else
#define MEMBER
#endif
struct CompilerLexer {
    virtual void MEMBER Reset(void);
    virtual int MEMBER Trace(int value);
};
struct CompilerPlainText {
    virtual int MEMBER TakeText(void);
    virtual void MEMBER Reset(void);
    virtual int MEMBER Trace(int value);
};
struct CompilerOtherText {
    virtual int MEMBER TakeText(void);
    virtual void MEMBER Reset(void);
};
void MEMBER CompilerLexer::Reset(void) {}
int MEMBER CompilerLexer::Trace(int value) { return value+1; }
int MEMBER CompilerPlainText::TakeText(void) { return 1; }
int MEMBER CompilerOtherText::TakeText(void) { return 2; }
CompilerLexer lexer;
CompilerPlainText text;
CompilerOtherText other;
extern "C" int main(void) {
    void **canonical=*(void ***)&lexer;
    void **aliases=*(void ***)&text;
    return canonical[0]==aliases[1] && canonical[1]==aliases[2] &&
           canonical[0]==(*(void ***)&other)[1] && text.Trace(41)==42 ? 0 : 1;
}
#ifndef __linux__
extern "C" int __stdcall DllMain(void *,unsigned,void *) { return 1; }
#endif
