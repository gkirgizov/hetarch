#include "gtest/gtest.h"

#include <iostream>
#include <type_traits>

#include "../new/MemoryManager.h"


using namespace std;
//using namespace hetarch::mem;


//template<typename AddrT>
//void print_mem_region(const MemRegion<AddrT> &region) {
//    std::cout << '[' << region.start << ", " << region.end << ", s:" << region.size << ']' << std::endl;
//}

class Base;
class Derived;
class Derived2;


struct Delegator {
    static void accept(const Base &x) {
//        std::cout << "runtime typeid(*this): " << typeid(base).name() << std::endl;
//        std::cout << "accepted " << typeid(x).name() << std::endl;
        std::cout << "accepted " << "Base" << std::endl;
    }

    static void accept(const Derived &x) {
        std::cout << "accepted " << "Derived" << std::endl;
    }

    static void accept(const Derived2 &x) {
        std::cout << "accepted " << "Derived2" << std::endl;
    }

//    template<typename T, typename std::enable_if<std::is_base_of<Base, T>::value>::type>
//    static void acceptT(const T &x) {
//        std::cout << "accepted<T> " << typeid(x).name() << std::endl;
//    };

};


class Base {
    const std::string namePrivate{"Base"};
public:
    void foo()         { std::cout << namePrivate << "::" << "foo" << std::endl; }
    virtual void bar() { std::cout << namePrivate << "::" << "bar" << std::endl; }

    virtual void baz() { std::cout << namePrivate << "::" << "baz" << std::endl; }

    virtual void printType() {
        std::cout << "runtime typeid(*this): " << typeid(*this).name() << std::endl;
        Delegator::accept(*this);
    }
};

template<typename T>
class BaseF : public Base {
public:
    void printType() override {
        std::cout << "runtime typeid(*this): " << typeid(*this).name() << std::endl;
        Delegator::accept(*static_cast<T*>(this));
    }
};

class Derived : public BaseF<Derived> {
    const std::string namePrivate{"Derived"};
public:
    void foo()          { std::cout << namePrivate << "::" << "foo" << std::endl; }
    void bar() override { std::cout << namePrivate << "::" << "bar" << std::endl; }
};

class Derived2 : public BaseF<Derived2> {
};


TEST(scratchTests, callBaseMethodFromDerived) {
    Derived d;
    Base* b = &d;
    b->foo(); // calls Base::foo
    b->bar(); // calls Derived::bar

    b->baz(); // calls Base::baz
    d.baz(); // calls ??? anyway, uses Base::namePrivate

    d.printType(); // prints Derived
    b->printType(); // prints Derived
    Base *b2 = new Derived2{};
    b2->printType();  // prints Derived2

}
