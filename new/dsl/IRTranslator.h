#pragma once


#include <iostream>

#include "IDSLCallable.h"


namespace hetarch {
namespace dsl {


// temporary! just to ensure interface
class IRTranslator {
    // state

public: // out-of-dsl constructs
    template<typename T, bool is_const, bool is_volatile>
    void accept(const Var<T, is_const, is_volatile> &var) const {
        std::cout << "accept " << typeid(var).name() << std::endl;
    }


public: // dsl function and related constructs (function is 'entry' point of ir generation)
    template<typename RetT, typename ...Args>  // full specialisation of func template -- this is how actual work will be implemented
    void accept(const DSLFunction<RetT, Args...> &func) const {
        std::cout << "accept " << typeid(func).name() << std::endl;

//        for(auto i = 0; i < )
        func.body.toIR(*this);
        func.returnSt.toIR(*this);
    }


public: // helper classes
    template<typename T>
    void accept(const Seq<T> &seq) const {
        std::cout << "accept " << typeid(seq).name() << std::endl;

        seq.lhs.toIR(*this);
        seq.rhs.toIR(*this);
    }

    // maybe i doesn't even need this if i'll put handling of Return<RetT> in DSLFunction<RetT, Args...>
    template<typename RetT>
    void accept(const Return<RetT> &returnSt) const {
        std::cout << "accept " << typeid(returnSt).name() << std::endl;
    }

    template<typename T>
    void accept(const EAssign<T> &e) const {
        std::cout << "accept " << typeid(e).name() << std::endl;
    }

    template<typename T>
    void accept(const ECall<T> &e) const {
        std::cout << "accept " << typeid(e).name() << std::endl;
    }

};

template<>
void IRTranslator::accept(const Return<void> &returnSt) const {
    std::cout << "accept " << typeid(returnSt).name() << std::endl;
}



template<typename T>
inline void toIRImpl(const T &irTranslatable, const IRTranslator &irTranslator) {
    irTranslator.accept(irTranslatable);
}

/// Allows static polymorphism.
/// Despite allowing use of Visitor pattern (convenient there), avoids run-time overhead for virtual calls.
/// \tparam T subclass of DSLBase
//template<typename T, std::enable_if<std::is_base_of<DSLBase, T>::value>::type>
template<typename T>
struct IRTranslatable : virtual public DSLBase {
    void toIR(const IRTranslator &gen) const override {
        gen.accept(*static_cast<const T*>(this));
    }
};

//template<template<typename T> typename Tdsl>
//struct IRTranslatable1 : public Expr<T> {
//    void toIR(const IRTranslator &gen) const override {
//        gen.accept(*static_cast<const Tdsl<T>*>(this));
//    }
//};


}
}
