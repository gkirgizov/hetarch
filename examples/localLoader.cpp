#include <iostream>
#include <string>
#include <cstdint>

#include <unistd.h>
#include <sys/mman.h>
#include <malloc.h>
#include <thread>

#include <asio.hpp>
#include <cxxopts.hpp>

#include "../new/TCPConnection.h"

using namespace hetarch::conn;
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


bool handleRequest(tcp::socket &socket, ExecutableBuffer &execBuf) {
    auto collected = detail::readBuffer(socket);

    if (!collected.size()) {
        std::cerr << "Connection closed by client" << std::endl;
        return false;
    }
    std::cerr << "Got request, len(" << collected.size() << ")" << std::endl;

    auto actionFlags = detail::vecRead<action_int_t>(collected, 0);
    size_t offset = sizeof(action_int_t);
    auto action = static_cast<hetarch::conn::Actions>(actionFlags & 0xFF);
    switch (action) {
        case hetarch::conn::Actions::GetBuffAddr: {
            std::cerr << "Request is: GetBuffAddr" << std::endl;

            auto bufIdx = detail::vecRead<unsigned>(collected, offset);
            std::vector<uint8_t> response;
            auto addr = reinterpret_cast<size_t>(execBuf.data());
            detail::vecAppend(response, addr);
            detail::writeBuffer(socket, response);
            break;
        }
        case hetarch::conn::Actions::AddrRead: {
            std::cerr << "Request is: AddrRead" << std::endl;

            auto addr = detail::vecRead<size_t>(collected, offset);
            offset+=sizeof(addr);
            auto size = detail::vecRead<size_t>(collected, offset);
            offset+=sizeof(size);
            std::vector<uint8_t> response(static_cast<size_t>(size));
            std::copy((uint8_t*)addr, (uint8_t*)(addr + size), response.begin());
            detail::writeBuffer(socket, response);
            break;
        }
        case hetarch::conn::Actions::AddrWrite: {
            std::cerr << "Request is: AddrWrite" << std::endl;

            auto addr = detail::vecRead<size_t>(collected, offset);
            offset+=sizeof(addr);
            auto size = detail::vecRead<size_t>(collected, offset);
            offset+=sizeof(size);

            std::cerr << "Request is: AddrWrite - everything read" << std::endl;
            std::copy(collected.begin() + offset, collected.begin() + offset + size, (uint8_t*)addr);

            std::vector<uint8_t> response;
            detail::vecAppend(response, 0);
            detail::writeBuffer(socket, response);
            break;
        }
        case hetarch::conn::Actions::Call: {
            std::cerr << "Request is: Call" << std::endl;

            auto addr = detail::vecRead<size_t>(collected, offset);
            offset+=sizeof(addr);
            std::vector<uint8_t> response;
            int(*fnptr)() = (int(*)())(addr);
            auto callRes = fnptr();
            detail::vecAppend(response, callRes);
            detail::writeBuffer(socket, response);
            break;
        }
        default: {
            std::cerr << "Request is: default" << std::endl;
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
        std::cerr << "EXCEPTION!!!" << std::endl;
    }
}

void runServer(ExecutableBuffer *buf, uint16_t port) {
    sleep(1);
    try
    {
        asio::io_service io_service;

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));

        std::cerr << "Server started..." << std::endl;

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


int main(int argc, char* argv[]) {
    cxxopts::Options opts("local_loader", "Simple TCP server that can give you access to an executable memory buffer.");
    opts.add_options()
            ("p,port", "port to listen for incoming connections",
             cxxopts::value<uint16_t>()->default_value("1334"))
            ("s,size", "size of the buffer to allocate",
             cxxopts::value<unsigned>()->default_value(std::to_string(1024*1024)))
            ;
    auto result = opts.parse(argc, argv);
    auto buf_size = result["size"].as<unsigned>();
    auto port = result["port"].as<uint16_t>();

    ExecutableBuffer codeBuf(buf_size);
    std::cerr << std::hex
              << "Alocated executable buffer of size 0x" << buf_size
              << " at 0x" << std::hex << reinterpret_cast<size_t>(codeBuf.data())
              << std::dec << std::endl;
    std::thread t1(runServer, &codeBuf, port);
    std::cerr << "Main application loop started at port " << port << "..." << std::endl;
    t1.join();
    return 0;
}
