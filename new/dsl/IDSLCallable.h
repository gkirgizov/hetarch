#pragma once


#include <utility>
#include <type_traits>
#include <string_view>


class DSLBase;
template<typename RetT, typename ...Args> class IDSLCallable;
template<typename RetT, typename ...Args> class DSLFunction;
template<typename RetT> class Return;
template<typename T> class VarBase;
template<typename T, bool, bool> class Var;

//template<typename T, bool = false> class Assignable;

template<typename T> class Seq;


template<typename RetT, typename ...Args> class ECall;
template<typename T> class EAssign;
template<typename T> class EBinOp;



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



struct DSLBase {
//    virtual void toIR(const IRGen &gen) const = 0;
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

struct ExprBase : public DSLBase {
};

template<typename T>
struct Expr : public ExprBase {
//    using type = T;
};



// todo: is it Expr?
template<typename T, bool disable = false>
struct Assignable : public Expr<T> {
    template<typename std::enable_if<!disable>::type>
    constexpr EAssign<T> operator=(Expr<T> &&rhs) {
        return EAssign<T>{*this, std::move(rhs)};
    }
};


// todo: how to pass string_view parameter
// todo: enable_if only for integral etc. simple types?
template<typename T>
struct VarBase : public DSLBase {
    const std::string_view name;
protected:
    T initial_val{};
    bool initialised{false};

    // forbid instantiation of VarBase
    explicit constexpr VarBase() = default;
    explicit constexpr VarBase(const std::string_view &name) : name{name} {}
    explicit constexpr VarBase(T &&value, const std::string_view &name = "")
            : name{name}, initial_val{std::forward(value)}, initialised{true}
    {}

    constexpr void initialise(T &&value) {
        initial_val = std::forward(value);
        initialised = true;
    }
};

// todo: handle virtual inheritance (somewhere...) higher in the hierarchy (VarBase + Assignable (as Expr))
// cv-qualifiers are part of type signature, so it makes sense to put them here as template parameters
template<typename T, bool is_const = false, bool is_volatile = false>
struct Var : public VarBase<T>, public Assignable<T, is_const>, public IRConvertible<Var<T, is_const, is_volatile>> {
    using VarBase<T>::VarBase;  // allow instantiation using same constructors
};

// todo: difference with LocalVar is that GlobalVar is Loadable. implement it.
template<typename T, bool is_const = false, bool is_volatile = false>
//using GlobalVar<T, is_const, is_volatile> = Var<T, is_const, is_volatile>;
struct GlobalVar : public Var<T, is_const, is_volatile>, public Assignable<T, is_const> {
    using Var<T, is_const, is_volatile>::Var;
};



/* Function-related constructs */

template<typename T>
using ParamBase = VarBase<T>;
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
class Return : public DSLBase, public IRConvertible<Return<RetT>> {
    const DSLBase &returnee;
public:
    explicit constexpr Return() = default;
    explicit constexpr Return(const Expr<RetT> &expr) : returnee{expr} {};
    explicit constexpr Return(const VarBase<RetT> &var) : returnee{var} {};
};

// maybe do IRConvertible here if we can query 'the thing to call' independently of subtype... ???
template<typename RetT, typename... Args>
struct IDSLCallable {
    constexpr ECall<RetT, Args...> operator()(Expr<Args>... args) {
        return ECall<RetT, Args...>(*this, args...);
    };
};

template<typename RetT, typename... Args>
class DSLFunction : public IDSLCallable<RetT, Args...> {
    const std::tuple<ParamBase<Args>...> params;  // const? hardly.
    const ExprBase body;
    const Return<RetT> returnSt;
//    const Seq<RetT> body;  // alternative; less flexible

public:
    const std::string_view name{};


    constexpr DSLFunction(const ParamBase<Args>&... params, ExprBase &&body, Return<RetT> &&returnSt)
            : params{params...}, body{std::move(body)}, returnSt{std::move(returnSt)} {}

    constexpr DSLFunction(const std::string_view &name,
                          const ParamBase<Args>&... params, ExprBase &&body, Return<RetT> &&returnSt)
            : params{params...}, body{std::move(body)}, returnSt{std::move(returnSt)}, name{name} {}


    /// Specialisation of c-tor for function returning void
    /// \param params
    /// \param body
    template<typename std::enable_if<std::is_void<RetT>::value>::type>
    explicit constexpr DSLFunction(const ParamBase<Args>&... params, ExprBase &&body)
            : params{params...}, body{std::move(body)}, returnSt{} {}

    template<typename std::enable_if<std::is_void<RetT>::value>::type>
    explicit constexpr DSLFunction(const std::string_view &name,
                                   const ParamBase<Args>&... params, ExprBase &&body)
            : params{params...}, body{std::move(body)}, returnSt{}, name{name} {}


    // suppose there's another func which uses __the same__ types
//    constexpr void specialise(Args... args);

};



namespace {

template<typename RetT, typename ...Args>
class ECall : Expr<RetT> {
    const IDSLCallable<RetT, Args...> &callee;
    const std::tuple<Expr<Args>...> args;
public:
    explicit constexpr ECall(const IDSLCallable<RetT, Args...> &callee, Expr<Args>&&... args)
            : callee{callee}, args{std::move(args)...} {}

};

template<typename T>
class EAssign : public Expr<T> {
    const Assignable<T> &var;
    const Expr<T> rhs;
public:
    constexpr EAssign(const Assignable<T> &var, Expr<T> &&rhs) : var{var}, rhs{std::move(rhs)} {}
};


// maybe it is possible to extend on 2 compatible types T1 and T2
// todo: enable_if
// todo: kind of specifier of OP which will somehow enable toIR() specification for each OP
template<typename T>
class EBinOp : public Expr<T> {
    const Expr<T> lhs;
    const Expr<T> rhs;
public:
    constexpr EBinOp(Expr<T> &&lhs, Expr<T> &&rhs) : lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

};

// example of bin op overload
template<typename T>
constexpr EBinOp<T> operator+(Expr<T> &&lhs, Expr<T> &&rhs) {
    return EBinOp<T>{std::move(lhs), std::move(rhs)};
}


} // namespace


// nothing there
constexpr void test_thing(const IRGen &gen) {
}

