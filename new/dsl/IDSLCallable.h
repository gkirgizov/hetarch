#pragma once


#include <utility>
#include <type_traits>
#include <string_view>



// todo: remove
template<typename T>
class IDSLVariable {
public:
    virtual T read() const = 0;
    virtual void write(T value) = 0;
};



class DSLBase;
class DSLFunction;

class Seq;

class ECall
class EAssign;
class EBinOp;



// temporary! just to ensure interface
class IRGen {
public:
//    template<typename T, typename std::enable_if<std::is_base_of<DSLBase, T>::value>::type>

    // general template
//    template<typename T>
//    void accept(const T &thing) const {}
//
    // full specialisation of func template -- this is how actual work will be implemented
//    template<>
//    void accept(const ThingType &thing) const {}

    // todo: check is it compiles given other overloads of accept
    template<typename RetT, typename ...Args>  // full specialisation of func template -- this is how actual work will be implemented
    void accept(const DSLFunction<RetT, Args...> &func) const {}

    // maybe i doesn't even need this if i'll put handling of Return<RetT> in DSLFunction<RetT, Args...>
    template<typename RetT>
    void accept(const Return<RetT> &returnSt) const {}
};



struct DSLBase {
    virtual void toIR(const IRGen &gen) const = 0;
};

/// Allows static polymorphism
/// (otherwise we lose compile-time type and can't determine needed overload gen::accept)
/// \tparam T conceptually a subclass of DSLBase
template<typename T>
struct DSLBaseCRTP : public DSLBase {
    void toIR(const IRGen &gen) const override {
        gen.accept(*static_cast<T*>(this));
    }
};

struct ExprBase : public DSLBase {
};

template<typename T>
struct Expr : public ExprBase {
//    using type = T;
};



namespace {

template<typename RetT, typename ...Args>
class ECall : Expr<RetT> {
    const IDSLCallable<RetT, Args...> &callee;
    const std::tuple<Expr<Args>...> args;
public:
    constexpr ECall(const IDSLCallable<RetT, Args...> &callee, Expr<Args>&&... args)
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
    Expr<T> lhs;
    Expr<T> rhs;
public:
    constexpr EBinOp(Expr<T> &&lhs, Expr<T> &&rhs) : lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

};

// example of bin op overload
template<typename T>
constexpr EBinOp<T> operator+(Expr<T> &&lhs, Expr<T> &&rhs) {
    return EBinOp<T>{std::move(lhs), std::move(rhs)};
}


} // namespace



// todo: is it Expr?
template<typename T>
struct Assignable: public Expr<T> {
    constexpr EAssign<T> operator=(Expr<T> &&rhs) {
        return EAssign<T>{*this, std::move(rhs)};
    }
};


// todo: enable_if only for integral etc.?
// todo: maybe move is_volatile to c-tor?
//  --wouldn't be very good. conceptually volatile is a part of type
//  todo: same about Const. maybe move there?
template<typename T, bool is_volatile = false>
struct Var : public DSLBase {
    explicit constexpr Var() = default;
    explicit constexpr Var(const std::string_view &name) : initial_val{}, name{name} {}
    explicit constexpr Var(T &&value, const std::string_view &name = "")
            : initial_val{std::forward(value)}, name{name}
    {}

    constexpr const std::string_view name;
protected:
    constexpr T initial_val;
};

// todo: handle virtual inheritance (somewhere...) higher in the hierarchy
template<typename T>
struct LocalVar : public Var<T>, public Assignable<T> {
    using Var<T>::Var<T>;
};

// todo: the difference with LocalVar is that it is Loadable
template<typename T>
struct GlobalVar : public Var<T>, public Assignable<T> {
    using Var<T>::Var<T>;
};

template<typename T>
struct ConstVar : public Var<T> {
    using Var<T>::Var<T>;

    constexpr void substitute(T &&value) {
        initial_val = std::forward(value);
    }
};


//template<typename T, typename std::enable_if<std::is_base_of<Var, T>::value>::type>
//class FuncParam : public DSLBase

// this elaborate templated inheritance is to mimic the behaviour of Var<T>
// because FuncParam should behave exactly in the same way as usual Var does
// todo: doesn't make sense for VarT == GlobalVar
template<template<typename T> typename VarT,
        typename std::enable_if<std::is_base_of<Var<T>, VarT<T>>::value>::type>
struct FuncParam : public VarT<T> {
    using VarT<T>::VarT<T>;

    explicit constexpr FuncParam(T &&default_value, const std::string_view &name = "")
            : VarT<T>{default_value, name}, defaulted{true}
    {}

    constexpr bool defaulted{false};
};


// works well when we can refer to enclosed construct independently of its nature
// i.e. toIR override { return "return " + x.getIRReferenceName(); }
template<typename RetT>
class Return : public DSLBase {
    constexpr const DSLBase &returnee;
public:
    explicit constexpr Return(const Var<RetT> &var) : returnee{var} {};
    explicit constexpr Return(const Expr<RetT> &expr) : returnee{expr} {};

};


template<typename RetT, typename... Args>
struct IDSLCallable {
    constexpr ECall<RetT, Args...> operator()(Expr<Args>... args) {
        return ECall<RetT, Args...>(*this, args...);
    };
};

template<typename RetT, typename... Args>
class DSLFunction : public IDSLCallable<RetT, Args...> {
//    std::tuple<Args...>

public:
//    template<typename ...Ts>
//    constexpr explicit DSLFunction(Expr<Ts>&&... statements, Expr<RetT> &&returnSt); // imitates Seq, may even use it

    template<typename T>
    constexpr DSLFunction(const FuncParams<Args...> &args, Seq<T> statements, Return<RetT> &&returnSt);
    constexpr DSLFunction(const FuncParams<Args...> &args, Seq<RetT> statements);

//    constexpr IDSLVariable<RetT> call(IDSLVariable<Args>... args) override;

    // suppose there's another func which uses __the same__ types
    void specialise(Args... args);


};

template<typename ...Args>
struct FuncParams {
    explicit constexpr FuncParams() {};
};


template<typename RetT, typename ...Args>
static void generateIR(const DSLFunction<RetT, Args...> &f) {
    int n, test;
    n, test;
    n;
    test;
};




