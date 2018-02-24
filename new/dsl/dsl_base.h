#pragma once

#include <iostream>

#include <utility>
#include <type_traits>
#include <tuple>

#include "../utils.h"



namespace hetarch {
namespace dsl {


class IRTranslator;
template<typename T> inline void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator);


struct DSLBase {
//    virtual inline void toIR(IRTranslator &irTranslator) const {
//        std::cerr << "called base class toIR()" << std::endl;
//    }
};

struct ESBase : public DSLBase {}; // Expression or Statement

struct ExprBase : public ESBase {};

template<typename T>
struct Expr : public ExprBase { using type = T; };

struct VoidExpr : public Expr<void> {};
const VoidExpr empty_expr{};


struct ValueBase : public DSLBase {};

struct CallableBase : public DSLBase {
    CallableBase() = default;
    CallableBase(const CallableBase &) = delete;

    virtual ~CallableBase() = default;
};



class Named {
    std::string_view m_name;
public:
    explicit constexpr Named() : m_name{make_bsv("")} {};
    explicit constexpr Named(const std::string_view &name) : m_name{name} {};
    explicit constexpr Named(const char* str) : m_name{make_bsv(str)} {};

    constexpr auto name() const { return m_name; }
    constexpr void rename(std::string_view name) { m_name = name; }
};



}
}
