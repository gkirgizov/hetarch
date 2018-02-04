#pragma once

#include <string_view>

#include "dsl_base.h"
#include "var.h"
#include "../utils.h"


namespace hetarch {
namespace dsl {


template<typename RetT, typename ...Args> class IDSLCallable;
template<typename RetT, typename ...Args> class DSLFunction;
//template<typename RetT> class Return;
//template<typename RetT, typename ...Args> class ECall;


template<typename T> using ParamBase = VarBase<T>;
template<typename T, bool is_const = false, bool is_volatile = false>
using Param = Var<T, is_const, is_volatile>;

// works well when we can refer to enclosed construct independently of its nature
// i.e. toIR override { return "return " + x.getIRReferenceName(); }
template<typename RetT>
class Return : public ESBase {
    const Expr<RetT> &returnee;

    //const Td returnee;
    //
    //public:
    //explicit constexpr Return(Td&& expr) : returnee{std::forward(expr)} {};
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


}
}
