class Counter {
    int value_;
public:
    Counter(int value);
    int next();
};
Counter::Counter(int value) : value_(value) {}
int Counter::next() { return ++value_; }
int main() {
    Counter counter(41);
    return counter.next() == 42 ? 0 : 1;
}
