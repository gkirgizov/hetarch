#pragma once


template<typename T>
class IDSLVariable {
    T m_value;
public:
    virtual T read() const = 0;
    virtual void write(T value) = 0;
};


class IDSLCallable {

public:
    template<typename RetT, typename... Args>
    virtual IDSLVariable<RetT> call(IDSLVariable<Args...>... args) = 0;
};


template<typename RetT, typename... Args>
class DSLFunction : public IDSLCallable {

public:
    IDSLVariable<RetT> call(IDSLVariable<Args>... args) override;

    // suppose there's another func which uses __the same__ types
    void specialise(Args... args);

};



