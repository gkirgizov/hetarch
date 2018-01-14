#pragma once


#include <utility>
#include <type_traits>


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
    template<typename T>
    void accept(const T &thing) const {}

    template<>  // full specialisation of func template -- this is how actual work will be implemented
    void accept(const DSLFunction &func) const {}
};



struct DSLBase {
    // void is temporary. maybe it is so, but also maybe there's some llvm::IRBase object
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



template<typename T>
struct Assignable: public Expr<T> {
    constexpr EAssign<T> operator=(Expr<T> &&rhs) {
        return EAssign<T>{*this, std::move(rhs)};
    }
};


// todo: enable_if only for integral etc.?
// todo: maybe move is_volatile to c-tor?
template<typename T, bool is_volatile = false>
class Var {
protected:
    T val;
public:
    explicit constexpr Var() : val{} {}
    explicit constexpr Var(T &&value) : val{std::forward(value)} {}  // actually non-explicit is okey

//    explicit constexpr Var(const IDSLVariable<T> &value) : val{value.read()} {}
//    explicit constexpr Var(IDSLVariable<T> &&value) : val{value.read()} {}
};

template<typename T>
class LocalVar : public Var<T>, public Assignable<T> {
};

// the difference with LocalVar is that it is Loadable
template<typename T>
class GlobalVar : public Var<T>, public Assignable<T> {
};

template<typename T>
class ConstVar : public Var<T> {
public:
    constexpr void substitute(T &&value) {
        val = std::forward(value);
    }
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

    constexpr DSLFunction(const FuncParams<Args...> &args, ExprBase&&... statements, Expr<RetT> &&returnSt); // imitates Seq, may even use it
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

};




