#pragma once

#include "IDSLCallable.h"


namespace hetarch {
namespace dsl {


// temporary! just to ensure interface
class IRGen {
    // state

    // out-of-dsl constructs
public:
    template<typename T, bool is_const, bool is_volatile>
    void accept(const Var<T, is_const, is_volatile> &var) const {}

    // dsl function and related constructs (function is 'entry' point of ir generation)
public:
    template<typename RetT, typename ...Args>  // full specialisation of func template -- this is how actual work will be implemented
    void accept(const DSLFunction<RetT, Args...> &func) const {}

    // maybe i doesn't even need this if i'll put handling of Return<RetT> in DSLFunction<RetT, Args...>
    template<typename RetT>
    void accept(const Return<RetT> &returnSt) const {}

public:

};


/// Allows static polymorphism.
/// Despite allowing use of Visitor pattern (convenient there), avoids run-time overhead for virtual calls.
/// \tparam T subclass of DSLBase
//template<typename T, std::enable_if<std::is_base_of<DSLBase, T>::value>::type>
template<typename T>
struct IRConvertible {
    void toIR(const IRGen &gen) const {
        gen.accept(*static_cast<T*>(this));
    }
};


}
}
