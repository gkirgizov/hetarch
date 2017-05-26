#include <iostream>
#include <string>
#include <cstdint>

#include <unistd.h>
#include <sys/mman.h>
#include <malloc.h>
#include <thread>

#include <asio.hpp>

#include "../include/TCPTestConn.h"

using asio::ip::tcp;

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

class ExecutableBuffer {
    size_t _size;
    unsigned char *_data;
public:
    ExecutableBuffer(size_t size, const unsigned char *data = nullptr): _size(size), _data(nullptr) {
        size_t pagesize = (size_t)sysconf(_SC_PAGE_SIZE);
        _data = (unsigned char*)memalign(pagesize, size);
        if (_data == NULL) {
            handle_error("memalign");
        }

        if (mprotect(_data, size,
                     PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
            handle_error("mprotect");
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

using hetarch::conn::vecRead;
using hetarch::conn::vecWrite;
using hetarch::conn::vecAppend;

bool handleRequest(tcp::socket &socket, ExecutableBuffer &execBuf) {
    auto collected = hetarch::conn::readBuffer(socket);

    if (!collected.size()) {
        std::cout << "Connection closed by client" << std::endl;
        return false;
    }
    std::cout << "Got request, len(" << collected.size() << ")" << std::endl;

    uint32_t actionFlags = vecRead<uint32_t>(collected, 0);

    auto action = static_cast<hetarch::conn::Actions>(actionFlags & 0xFF);

    switch (action) {
        case hetarch::conn::Actions::GetBuffAddr: {
            std::cout << "Request is: GetBuffAddr" << std::endl;

            uint32_t bufIdx = vecRead<uint32_t>(collected, 4);
            std::vector<uint8_t> response;
            uint64_t addr = reinterpret_cast<uint64_t>(execBuf.data());
            vecAppend<uint64_t>(response, addr);
            hetarch::conn::writeBuffer(socket, response);
            break;
        }
        case hetarch::conn::Actions::AddrRead: {
            std::cout << "Request is: AddrRead" << std::endl;

            uint64_t addr = vecRead<uint64_t>(collected, 4);
            uint64_t size = vecRead<uint64_t>(collected, 12);
            std::vector<uint8_t> response(static_cast<size_t>(size));
            std::copy((uint8_t*)addr, (uint8_t*)(addr + size), response.begin());
            hetarch::conn::writeBuffer(socket, response);
            break;
        }
        case hetarch::conn::Actions::AddrWrite: {
            std::cout << "Request is: AddrWrite" << std::endl;

            uint64_t addr = vecRead<uint64_t>(collected, 4);
            uint64_t size = vecRead<uint64_t>(collected, 12);

            std::cout << "Request is: AddrWrite - everything read" << std::endl;
            std::copy(collected.begin() + 20, collected.begin() + 20 + size, (uint8_t*)addr);

            std::vector<uint8_t> response;
            vecAppend<uint32_t>(response, 0);
            hetarch::conn::writeBuffer(socket, response);
            break;
        }
        case hetarch::conn::Actions::Call: {
            std::cout << "Request is: Call" << std::endl;

            uint64_t addr = vecRead<uint64_t>(collected, 4);
            std::vector<uint8_t> response;
            uint32_t(*fnptr)() = (uint32_t(*)())(addr);
            uint32_t callRes = fnptr();
            vecAppend<uint32_t>(response, callRes);
            hetarch::conn::writeBuffer(socket, response);
            break;
        }
        default: {
            std::cout << "Request is: default" << std::endl;
            break;
        }
    }
    return true;
}

void handleClientConnection(tcp::socket &socket, ExecutableBuffer &execBuf) {
    try {
        while (handleRequest(socket, execBuf));
    }
    catch (...) {
        std::cout << "EXCEPTION!!!" << std::endl;
    }
}

void runServer(ExecutableBuffer *buf) {
    sleep(1);
    try
    {
        asio::io_service io_service;

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 1334));

        std::cout << "Server started..." << std::endl;

        for (;;)
        {
            tcp::socket socket(io_service);
            acceptor.accept(socket);
            handleClientConnection(socket, *buf);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void mainApplicationLoop() {
    std::cout << "Main application loop started..." << std::endl;
}

int main() {
    ExecutableBuffer codeBuf(2 * 1024 * 1024);
    std::cout <<
            "Alocated executable buffer at 0x" <<
            std::hex << std::uppercase <<
            reinterpret_cast<uint64_t>(codeBuf.data())
            << std::nouppercase << std::dec << std::endl;
    std::thread t1(runServer, &codeBuf);
    mainApplicationLoop();
    t1.join();
    return 0;
}
