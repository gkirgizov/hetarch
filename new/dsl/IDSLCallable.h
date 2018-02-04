#pragma once

#include <iostream>

#include <utility>
#include <type_traits>
#include <string_view>
#include <tuple>

#include "../utils.h"



namespace hetarch {
namespace dsl {


class IRTranslator;
template<typename T> inline void toIRImpl(const T &irTranslatable, IRTranslator &irTranslator);


//class DSLBase;
//class ESBase;
template<typename RetT, typename ...Args> class IDSLCallable;
template<typename RetT, typename ...Args> class DSLFunction;
template<typename RetT> class Return;
//template<typename T> class VarBase;
//template<typename T, bool, bool> class Var;

//template<typename RetT, typename ...Args> class ECall;
template<typename T> class EAssign;


struct DSLBase {
    virtual inline void toIR(IRTranslator &irTranslator) const {
        std::cerr << "called base class toIR()" << std::endl;
    }
};

struct ESBase : public DSLBase {}; // Expression or Statement

struct ExprBase : public ESBase {};

template<typename T>
struct Expr : public ExprBase {};

struct EmptyExpr : public ExprBase {
    inline void toIR(IRTranslator &irTranslator) const override {
        std::cout << "called " << typeid(this).name() << " toIR()" << std::endl;
    }
};
static constexpr const auto empty_expr = EmptyExpr{};


template<typename... Args>
using expr_tuple = std::tuple<const Expr<Args>&...>;


// todo: enable_if only for integral etc. simple types?
template<typename T>
class VarBase : public Expr<T> {
    T m_initial_val{};
    bool m_initialised{false};
public:
    const std::string_view name{make_bsv("")};

    constexpr VarBase(VarBase<T> &&) = default;
    constexpr VarBase(const VarBase<T> &) = default; // TODO: somehow preserve 'identity' among copies
    constexpr VarBase<T> &operator=(VarBase<T> &&) = delete; // doesn't make much sense to move-assign
    constexpr VarBase<T> &operator=(VarBase<T> &) = delete; // doesn't make much sense to copy-assign

    explicit constexpr VarBase() = default;
    explicit constexpr VarBase(const std::string_view &name) : name{name} {};
    explicit constexpr VarBase(T &&value) : m_initial_val{std::forward<T>(value)}, m_initialised{true} {}
    explicit constexpr VarBase(T &&value, const std::string_view &name)
            : name{name}, m_initial_val{std::forward<T>(value)}, m_initialised{true} {}

    constexpr void initialise(T &&value) {
        m_initial_val = std::forward(value);
        m_initialised = true;
    }
    constexpr bool initialised() const { return m_initialised; }
    constexpr T initial_val() const { return m_initial_val; }
};


/*
// todo: assign native T&& type?
template<typename T, bool disable = false>
struct Assignable {
    constexpr EAssign<T> assign(Expr<T> &&rhs) const;
    inline constexpr EAssign<T> operator=(Expr<T> &&rhs) const { return this->assign(std::move(rhs)); };
};
template<typename T> struct Assignable<T, true> {};

// cv-qualifiers are part of type signature, so it makes sense to put them here as template parameters
template<typename T, bool is_const = false, bool is_volatile = false>
struct Var : public VarBase<T>, public Assignable<T, is_const> {
    using VarBase<T>::VarBase;
    using Assignable<T>::operator=;

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};
 */


// cv-qualifiers are part of type signature, so it makes sense to put them here as template parameters
template<typename T, bool is_const = false, bool is_volatile = false>
struct Var : public VarBase<T> {
    using VarBase<T>::VarBase;

    inline constexpr EAssign<T> assign(Expr<T> &&rhs) const;
    inline constexpr EAssign<T> operator=(Expr<T> &&rhs) const { return this->assign(std::move(rhs)); };

    inline constexpr EAssign<T> assign(const VarBase<T> &rhs) const;
    inline constexpr EAssign<T> operator=(const VarBase<T> &rhs) const { return this->assign(rhs); };

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

// specialisation for const
template<typename T, bool is_volatile>
struct Var<T, true, is_volatile> : public VarBase<T> {
    using VarBase<T>::VarBase;

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};

template<typename T, bool is_volatile = false>
using Const = Var<T, true, is_volatile>;


// todo: difference with LocalVar is that GlobalVar is Loadable. implement it.
//template<typename T, bool is_const = false, bool is_volatile = false>
//using GlobalVar<T, is_const, is_volatile> = Var<T, is_const, is_volatile>;
//struct GlobalVar : public Var<T, is_const, is_volatile>, public Assignable<T, is_const> {
//    using Var<T, is_const, is_volatile>::Var;
//};


/* Function-related constructs */

template<typename T> using ParamBase = VarBase<T>;
template<typename T, bool is_const = false, bool is_volatile = false>
using Param = Var<T, is_const, is_volatile>;

// todo: convenient container for FuncParam
/* cases:
 * most default case (silliest container)
 * handy factory
 */
template<typename ...Args>
struct FuncParams {
};


// works well when we can refer to enclosed construct independently of its nature
// i.e. toIR override { return "return " + x.getIRReferenceName(); }
template<typename RetT>
class Return : public ESBase {
    const Expr<RetT> &returnee;
public:
    explicit constexpr Return(Expr<RetT> &&expr) : returnee{std::move(expr)} {};
    explicit constexpr Return(const VarBase<RetT> &var) : returnee{var} {};

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }

    ~Return() {
        std::cout << "called " << typeid(*this).name() << " destructor" << std::endl;
    }
};

template<> class Return<void> : public ESBase {
    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


template<typename RetT, typename ...Args> struct ECallEmptyBase : public Expr<RetT> {
    inline void toIR(IRTranslator &irTranslator) const override {
        std::cout << "called " << typeid(this).name() << " toIR()" << std::endl;
    }
};

template<typename RetT, typename... Args>
class ECall: public ECallEmptyBase<RetT, Args...> {
    const DSLFunction<RetT, Args...> &callee;
    expr_tuple<Args...> args;
public:
    explicit constexpr ECall(const DSLFunction<RetT, Args...> &callee, const Expr<Args>&... args)
            : callee{callee}, args{args...} {}

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }

    ~ECall() {
        std::cout << "called " << typeid(*this).name() << " destructor" << std::endl;
    }
};


// todo: do i even need virtual function in this class?
template<typename RetT, typename... Args>
struct IDSLCallable : public DSLBase {
//     TODO: returning by value to EmptyBase Slices object. we don't have overloaded toIR then.
//    virtual ECallEmptyBase<RetT, Args...> call(const Expr<Args>&... args) const = 0;
//    inline ECallEmptyBase<RetT, Args...> operator()(const Expr<Args>&... args) const { return call(args...); };

    // todo: do I even need this?
    // todo: uncommenting THIS line lead to sigsegv in dsl_tests.cpp
//    inline ECallEmptyBase<RetT, Args...> call(Expr<Args>&&... args) const { return call(std::move(args)...); };
//    inline ECallEmptyBase<RetT, Args...> operator()(Expr<Args>&&... args) const { return call(std::move(args)...); };
};

// todo: handle default arguments
// todo: handle empty body
template<typename RetT, typename... Args>
class DSLFunction : public IDSLCallable<RetT, Args...> {
    using params_t = std::tuple<const ParamBase<Args>&...>;

    const params_t params;
    const ExprBase &body;
//    const Return<RetT> &returnSt;
    const Return<RetT> returnSt;

public:
    using raw_params_t = std::tuple<Args...>;

    const std::string_view name{make_bsv("")};


    // Empty body constructors
    constexpr DSLFunction(const ParamBase<Args>&... params, Return<RetT> &&returnSt)
//            : params{params...}, body{empty_expr}, returnSt{returnSt} {}
            : params{params...}, body{empty_expr}, returnSt{std::move(returnSt)} {}
    constexpr DSLFunction(const std::string_view &name,
                          const ParamBase<Args>&... params, Return<RetT> &&returnSt)
//            : params{params...}, body{empty_expr}, returnSt{returnSt}, name{name}, name{name} {}
            : params{params...}, body{empty_expr}, returnSt{std::move(returnSt)}, name{name} {}

    // Ordinary (full) constructors
    constexpr DSLFunction(const ParamBase<Args>&... params, ExprBase &&body, Return<RetT> &&returnSt)
            : params{params...}, body{std::move(body)}, returnSt{std::move(returnSt)} {}
    constexpr DSLFunction(const std::string_view &name,
                          const ParamBase<Args>&... params, ExprBase &&body, Return<RetT> &&returnSt)
            : params{params...}, body{std::move(body)}, returnSt{std::move(returnSt)}, name{name} {}

    // 'return void' constructors
    // todo: do class template spesialiation? enable_if  cannot be used here: compile error
/*    template<typename = typename std::enable_if<std::is_void<RetT>::value>::type>
    explicit constexpr DSLFunction(const ParamBase<Args>&... params, ExprBase &&body)
            : params{params...}, body{std::move(body)}, returnSt{} {}

    template<typename = typename std::enable_if<std::is_void<RetT>::value>::type>
    explicit constexpr DSLFunction(const std::string_view &name,
                                   const ParamBase<Args>&... params, ExprBase &&body)
            : params{params...}, body{std::move(body)}, returnSt{}, name{name} {}*/


//    inline ECallEmptyBase<RetT, Args...> call(const Expr<Args>&... args) const override {
    inline ECall<RetT, Args...> operator()(const Expr<Args>&... args) const { return call(args...); };
    inline ECall<RetT, Args...> call(const Expr<Args>&... args) const {
        return ECall<RetT, Args...>(*this, args...);
    };

    // suppose there's another func which uses __the same__ types
//    constexpr void specialise(Args... args);

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


template<typename T>
class EAssign : public Expr<T> {
    const VarBase<T> &var;
    const Expr<T> &rhs;
public:
    constexpr EAssign(EAssign<T> &&) = default;
    constexpr EAssign<T> &operator=(EAssign<T> &&) = delete;

    constexpr EAssign(const VarBase<T> &var, Expr<T> &&rhs) : var{var}, rhs{rhs} {}
    constexpr EAssign(const VarBase<T> &var, const VarBase<T> &rhs) : var{var}, rhs{rhs} {}

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};



template<typename T, bool is_const, bool is_volatile>
constexpr EAssign<T> Var<T, is_const, is_volatile>::assign(Expr<T> &&rhs) const {
    return EAssign<T>{*this, std::move(rhs)};
}

template<typename T, bool is_const, bool is_volatile>
constexpr EAssign<T> Var<T, is_const, is_volatile>::assign(const VarBase<T> &rhs) const {
    return EAssign<T>{*this, rhs};
}

// nothing there
constexpr void test_thing(IRTranslator &gen) {}


}
}
