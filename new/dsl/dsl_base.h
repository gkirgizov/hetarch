#pragma once

#include <iostream>

#include <utility>
#include <type_traits>
#include <tuple>

#include "../utils.h"



namespace hetarch {
namespace dsl {


// Forward declaration for use in other dsl/*.h files
class IRTranslator;
template<typename T> inline void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator);

#define IR_TRANSLATABLE \
inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }


struct DSLBase {};

struct ESBase : DSLBase {}; // Expression or Statement

struct ExprBase : ESBase {};

template<typename T>
struct Expr : ExprBase { using type = T; };

namespace {
    std::size_t val_counter{0};
}

struct ValueBase : DSLBase {
    using iid_t = std::size_t;
    const iid_t iid;

    ValueBase() : iid{ val_counter++ } {
        if constexpr (utils::is_debug) {
            std::cerr << "construct ValueBase with iid=" << iid << std::endl;
        }
    }
};

struct CallableBase : DSLBase {
    CallableBase() = default;
//    CallableBase(const CallableBase &) = delete;
    CallableBase(CallableBase &&) = default;

    virtual ~CallableBase() = default; // it's needed
};

struct VoidExpr : ValueBase {
    using type = void;
    IR_TRANSLATABLE
};
const VoidExpr Unit{};


// Literal
template<typename T>
struct Lit : Expr<T> {
// todo: move in appropriate place -- avoid link problems due to to_dsl_t use
//    static_assert(!std::is_void_v< to_dsl_t<T> >, "This type can't be translated to DSL type system!");
    static_assert(std::is_scalar_v<T>, "This type can't be translated to DSL type system!");
    const T val;
    constexpr Lit(T val) : val{val} {}
    IR_TRANSLATABLE
};


#define MEMBER_ASSIGN_OP(TD, SYM) \
template<typename T2> \
constexpr auto operator SYM##= (T2 rhs) const { \
    return this->assign( static_cast<const TD &>(*this) SYM rhs); \
}

#define MEMBER_XX_OP(TD, X) \
constexpr auto operator X##X () { return this->assign(static_cast<const TD &>(*this) X Lit{1}); }
// unclear what to do with postfix version
//constexpr auto operator X##X (int) { return this->assign(static_cast<const TD &>(*this) X Lit{1}); } \


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
