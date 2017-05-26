#include <iostream>
#include <iomanip>
#include <fstream>

#include <unistd.h>
#include <sys/mman.h>
#include <malloc.h>

#include "../include/dsl.h"
#include "../include/MemMgr.h"

#include "../include/cg.h"
#include "../include/TCPTestConn.h"

#include "basic.h"

using namespace hetarch;
using namespace hetarch::dsl;

class ExecutableBuffer {
    size_t _size;
    unsigned char *_data;
public:
    ExecutableBuffer(size_t size, const unsigned char *data = nullptr): _size(size), _data(nullptr) {
        size_t pagesize = (size_t)sysconf(_SC_PAGE_SIZE);
        _data = (unsigned char*)memalign(pagesize, size);


        if (mprotect(_data, size,
                     PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
            throw "mprotect";
        }
        if (data) {
            memcpy(_data, data, sizeof(unsigned char) * _size);
        }
    }
    ExecutableBuffer(const ExecutableBuffer &src) = delete;
    size_t size() { return _size; }
    unsigned char *data() { return _data; }
    ~ExecutableBuffer() { if (_data) { free(_data); }}
};

char message[] = "Hello, World!";
char recvBuf[1024];

void dumpBuffer(const std::vector<uint8_t> &buf, std::ostream &out){
    for (int i = 0; i < buf.size(); ++i) {
        out << std::hex << std::setw(2) << std::setfill('0') << ((unsigned int)(buf[i]));
        out << std::dec;
        //if (i % 2 == 1) { out << " "; }
        out << " ";
    }
    out << std::endl;
}

void prepareFunctionNoSend(cg::ICodeGen &codeGen, mem::MemMgr<uint64_t> &memMgr, FunctionBase<uint64_t> &fn) {
    std::cout << fn.getFunctionModuleIR() << std::endl;
    fn.compile(codeGen, memMgr);
    std::cout << "dumpCompiled: " << fn.Binary().size() << std::endl;
    dumpBuffer(fn.Binary(), std::cout);
}

void prepareFunction(cg::ICodeGen &codeGen, mem::MemMgr<uint64_t> &memMgr,
                     hetarch::conn::TCPTestConnection &conn, FunctionBase<uint64_t> &fn)
{
    prepareFunctionNoSend(codeGen, memMgr, fn);
    fn.Send(conn);
}

void testLocal() {
    std::unique_ptr<cg::ICodeGen> codeGen =
            std::unique_ptr<cg::ICodeGen>(cg::createCodeGen("x86_64-unknown-linux-gnu"));

    mem::MemMgr<uint64_t> memMgr;

    auto conn = std::unique_ptr<hetarch::conn::TCPTestConnection>(new hetarch::conn::TCPTestConnection("localhost", 1334));
    uint64_t bufAddr = conn->GetBuff(0);
    std::cout << "bufAddr: " << bufAddr << std::endl;

    memMgr.DefineArea(bufAddr, bufAddr + 10 * 1024, mem::MemClass::ReadPreferred);
    memMgr.DefineArea(bufAddr + 10 * 1024, bufAddr + 20 * 1024, mem::MemClass::ReadWrite);

    Logger<uint64_t, 50> logger(memMgr, *codeGen, *conn);

    BasicFuncs<uint64_t, 50> funcs(memMgr, logger);

    prepareFunction(*codeGen, memMgr, *conn, funcs.sum2);
    prepareFunction(*codeGen, memMgr, *conn, funcs.initArray);
    prepareFunction(*codeGen, memMgr, *conn, funcs.bubbleSort);
    prepareFunction(*codeGen, memMgr, *conn, funcs.getFixedValue);
    prepareFunction(*codeGen, memMgr, *conn, funcs.getStaticSum);

    auto callRes  = conn->Call(funcs.getFixedValue.Addr());
    std::cout << "getFixedValue callRes: " << callRes << std::endl;

    std::cout << "getStaticSum sent" << std::endl;
    callRes  = conn->Call(funcs.getStaticSum.Addr());
    std::cout << "getStaticSum callRes: " << callRes << std::endl;

    conn->Call(funcs.initArray.Addr());
    conn->Call(funcs.bubbleSort.Addr());

    for (int i = 0; i < 5; i++) {
        int elem = funcs.elems.GetNth<uint32_t>(*conn, i);
        std::cout << "elems " << i << " " << elem << std::endl;
    }
}


void testMsp() {
    std::unique_ptr<cg::ICodeGen> codeGen =
            std::unique_ptr<cg::ICodeGen>(cg::createCodeGen("x86_64-unknown-linux-gnu"));

    mem::MemMgr<uint64_t> memMgr;
}

int main(int argc, char **argv) {


    return 0;
}