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
//    virtual void toIR(const IRGen &gen) const = 0;
};

/// Allows static polymorphism
/// (otherwise we lose compile-time type and can't determine needed overload gen::accept)
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
template<typename T, bool disable = false>
struct Assignable: public Expr<T> {
    template<typename std::enable_if<!disable>::type>
    constexpr EAssign<T> operator=(Expr<T> &&rhs) {
        return EAssign<T>{*this, std::move(rhs)};
    }
};


// todo: enable_if only for integral etc. simple types?
template<typename T>
struct VarBase : public DSLBase, public IRConvertible<VarBase<T>> {
    explicit constexpr VarBase() = default;
    explicit constexpr VarBase(const std::string_view &name) : initial_val{}, name{name} {}
    explicit constexpr VarBase(T &&value, const std::string_view &name = "")
            : initial_val{std::forward(value)}, name{name}
    {}

    constexpr void substitute(T &&value) {
        initial_val = std::forward(value);
    }

    constexpr const std::string_view name;
protected:
    constexpr T initial_val;
};

// todo: handle virtual inheritance (somewhere...) higher in the hierarchy (VarBase + Assignable (as Expr))
// cv-qualifiers are part of type signature, so it makes sense to put them here as template parameters
template<typename T, bool is_const = false, bool is_volatile = false>
struct Var : public VarBase<T>, public Assignable<T, is_const>, public IRConvertible<Var<T, is_const, is_volatile>> {
    using VarBase<T>::VarBase<T>;
};

//template<typename T>
//struct LocalVar : public VarBase<T>, public Assignable<T> {
//    using VarBase<T>::VarBase<T>;
//};

// todo: difference with LocalVar is that it GlobalVar is Loadable. implement it.
template<typename T, bool is_const = false, bool is_volatile = false>
//using GlobalVar<T, is_const, is_volatile> = Var<T, is_const, is_volatile>;
struct GlobalVar : public Var<T, is_const, is_volatile>, public Assignable<T, is_const> {
    using Var<T, is_const, is_volatile>::Var<T, is_const, is_volatile>;
};


// because FuncParam should behave exactly in the same way as usual Var does
template<typename T, bool is_const = false, bool is_volatile = false>
struct FuncParam : public Var<T, is_const, is_volatile>, public IRConvertible<FuncParam<T, is_const, is_volatile>> {
    using Var<T, is_const, is_volatile>::Var<T, is_const, is_volatile>;

    explicit constexpr FuncParam(T &&default_value, const std::string_view &name = "")
//            : Var<T, is_const, is_volatile>{default_value, name}, defaulted{true}
            : initial_val{default_value}, name{name}, defaulted{true}
    {}

    constexpr bool defaulted{false};
};


// works well when we can refer to enclosed construct independently of its nature
// i.e. toIR override { return "return " + x.getIRReferenceName(); }
template<typename RetT>
class Return : public DSLBase, public IRConvertible<Return<RetT>> {
    constexpr const DSLBase &returnee;
public:
    explicit constexpr Return(const VarBase<RetT> &var) : returnee{var} {};
    explicit constexpr Return(const Expr<RetT> &expr) : returnee{expr} {};

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
//    std::tuple<Args...>

public:
//    template<typename ...Ts>
//    constexpr explicit DSLFunction(Expr<Ts>&&... statements, Expr<RetT> &&returnSt); // imitates Seq, may even use it

    template<typename T>
    constexpr DSLFunction(const FuncParams<Args...> &args, Seq<T> statements, Return<RetT> &&returnSt);
    explicit constexpr DSLFunction(const FuncParams<Args...> &args, Seq<RetT> statements);

    // suppose there's another func which uses __the same__ types
    void specialise(Args... args);


};

// todo: convenient container for FuncParams
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




