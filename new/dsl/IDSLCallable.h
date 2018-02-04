#pragma once

#include <iostream>

#include <utility>
#include <type_traits>
#include <tuple>



namespace hetarch {
namespace dsl {


class IRTranslator;
template<typename T> inline void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator);



struct DSLBase {
    virtual inline void toIR(IRTranslator &irTranslator) const {
        std::cerr << "called base class toIR()" << std::endl;
    }
};

struct ESBase : public DSLBase {}; // Expression or Statement

struct ExprBase : public ESBase {};

template<typename T>
struct Expr : public ExprBase { using type = T; };

struct EmptyExpr : public ExprBase {
    inline void toIR(IRTranslator &irTranslator) const override {
        std::cout << "called " << typeid(this).name() << " toIR()" << std::endl;
    }
};
static const auto empty_expr = EmptyExpr{};


template<typename... Args>
using expr_tuple = std::tuple<const Expr<Args>&...>;



}
}
