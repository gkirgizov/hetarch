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

struct ESBase : public DSLBase {}; // Expression or Statement

struct ExprBase : public ESBase {};

template<typename T>
struct Expr : public ExprBase { using type = T; };

namespace {
    std::size_t val_counter{0};
}

struct ValueBase : public DSLBase {
    using iid_t = std::size_t;
    const iid_t iid;

    ValueBase() : iid{ val_counter++ } {
        if constexpr (utils::is_debug) {
            std::cerr << "construct ValueBase with iid=" << iid << std::endl;
        }
    }
};

struct CallableBase : public DSLBase {
    CallableBase() = default;
//    CallableBase(const CallableBase &) = delete;
    CallableBase(CallableBase &&) = default;

    virtual ~CallableBase() = default;
};

struct VoidExpr : public ValueBase {
    using type = void;
    IR_TRANSLATABLE
};
const VoidExpr Unit{};


// todo: temp
template<typename T, typename = typename std::enable_if_t< std::is_arithmetic_v<T> >>
struct DSLConst : public Expr<T> {
    const T val;
    constexpr DSLConst(T val) : val{val} {}
    IR_TRANSLATABLE
};
//template<typename T, typename = typename std::enable_if_t< std::is_arithmetic_v<T> >>
//constexpr auto operator"" _dsl (T val) { return DSLConst<T>{val}; };

#define MEMBER_ASSIGN_OP(TD, SYM) \
template<typename T2> \
constexpr auto operator SYM##= (T2 rhs) const { \
    return this->assign( static_cast<const TD &>(*this) SYM rhs); \
}

#define MEMBER_XX_OP(TD, X) \
constexpr auto operator X##X () { return this->assign(static_cast<const TD &>(*this) X DSLConst{1}); }
// unclear what to do with postfix version
//constexpr auto operator X##X (int) { return this->assign(static_cast<const TD &>(*this) X DSLConst{1}); } \


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
