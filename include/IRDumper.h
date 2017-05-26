#ifndef IRDUMPER_H
#define IRDUMPER_H

#include <string>
#include <map>
#include <vector>

class IRDumper {
    typedef HETARCH_TARGET_ADDRT AddrT;

    static unsigned staticCounter;
    std::string text;
    unsigned counter;
    std::map<std::string, int> allocs;
    std::map<std::string, std::pair<int, int> > allocArrs;
    std::map<std::string, std::pair<int, GlobalBase> > globals;
    std::map<std::string, std::pair<int, std::pair<GlobalBase, int> > > globalArrs;
    std::map<std::string, FunctionBase<AddrT>*> inlines;

    void append(const char *line) { text += line; }
    void append(const std::string &line) { text += line; }
    void append(std::string &&line) { text += line; }

    inline std::string PtrIntType() { return IRTypename<AddrT>(); }
public:
    unsigned indent;
    IRDumper() : counter(1), indent(0) {}

    void appendln(const char *line) { for(int i=0;i<indent;i++){text+="  ";} text += line; text += "\n"; }
    void appendln(const std::string &line) { for(int i=0;i<indent;i++){text+="  ";} text += line; text += "\n"; }
    void appendln(std::string &&line) { for(int i=0;i<indent;i++){text+="  ";} text += line; text += "\n"; }

    //void appendLabel(const std::string &name) { return appendln("; <label>:" + name); }
    //void appendLabel(std::string &&name) { return appendln("; <label>:" + name); }
    void appendLabel(const std::string &name) { text += name; text += ":\n"; }
    void appendLabel(std::string &&name) { text += name; text += ":\n"; }

    void appendBr(const std::string &headreg, const std::string &labelThen, const std::string &labelElse) {
        return appendln(std::string("br i1 ") + " " + headreg + ", label %" + labelThen + ", label %" + labelElse);
    }

    void appendBr(const std::string &name) { return appendln("br label %" + name); }
    void appendBr(std::string &&name) { return appendln("br label %" + name); }

    template<typename T>
    void appendLoad(const std::string &dst, const std::string &srcaddr) {
        appendln(dst + " = load " + IRTypename<T>() +
                 ", " + IRTypename<T>() + "* " + srcaddr + ", align " +
                 std::to_string(sizeof_safe<T>()));
    }

    template<typename T>
    void appendStore(const std::string &dstaddr, const std::string &src) {
        appendln(std::string("store ") + IRTypename<T>() + " " + src + ", " + IRTypename<T>() + "* " + dstaddr + ", align 4");
    }

    void appendBitcast(const std::string &dst, const std::string &tsrc,
                       const std::string &val, const std::string &tdst)
    {
        appendln(dst + " = bitcast " + tsrc + " " + val + " to " + tdst);
    }

    template<typename T>
    void global(const std::string &name, GlobalBase base) {
        globals[name] = std::make_pair(sizeof_safe<T>(), base);
    }
    template<typename T, int N>
    void globalArr(const std::string &name, GlobalBase base) {
        globalArrs[name] = std::make_pair(sizeof_safe<T[N]>(), std::make_pair(base, N));
    }

    void alloc(const std::string &name, int size) { allocs[name] = size; }
    void allocArr(const std::string &name, int size, int count) { allocArrs[name] = std::make_pair(size, count); }

    void inlineFn(const std::string &name, FunctionBase<AddrT> *fn) {
        inlines[name] = fn;
    }

    int nextInd() { return counter++; }
    std::string nextName(const char* pref = "x") { return pref + std::to_string(nextInd()); }
    std::string nextReg() { return std::string("%x") + std::to_string(nextInd()); }

    template<typename T>
    std::string tempVarReg() {
        std::string name = nextName("tmp");
        alloc(name, sizeof_safe_bits<T>());
        return "%" + name;
    }

    std::string getText(std::vector<FunctionBase<AddrT>*> &_inlines);

    template<typename T>
    static std::string functionBody(const RValueBase<T> &e, std::vector<FunctionBase<AddrT>*> &_inlines) {
        return convertToIR(e, _inlines);
    }

    template<typename T>
    static std::string functionBody(RValueBase<T> &&e, std::vector<FunctionBase<AddrT>*> &_inlines) {
        return convertToIR(e, _inlines);
    }

    template<typename T> static std::string function(std::string name, const RValueBase<T> &e) {
        return function(name, RValueBase<T>(e));
    }
    template<typename T> static std::string function(std::string name, RValueBase<T> &&e) {
        std::string rett = std::is_same<T, void>::value ? "void" : "i" + tsize<T>();
        std::string head = "define " + rett + " @" + name + "() #123 {\n";
        std::string body = convertToIR(e);
        return head + body + "}";
    }

    static int nextIndStatic();
};

#endif