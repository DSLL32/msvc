template<class T> class Box { public: T value; T get(); };
template<class T> T Box<T>::get() { return value; }
Box<int> box;
int template_use() { return box.get(); }
