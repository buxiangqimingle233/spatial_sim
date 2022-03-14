#include <iostream>
using namespace std;

class A {
public:
    A() {};
    virtual void enter() {
        func();
    }

    virtual void func() {
        cout << "call A" << endl;
    }
};


class B : public A {
public:
    B() {};

    virtual void func() override {
        cout << "call B" << endl;
    }
};


int main() {
    B a = B();
    a.enter();
}