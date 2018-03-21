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
#include "../new/conn_utils.h"
#include "../new/ht_proto.h"


using namespace hetarch::conn;
using asio::ip::tcp;


#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

// addr_t is also defined in hetarch::conn
// using addr_t = std::size_t;
static_assert(sizeof(addr_t) == sizeof(size_t));

class ExecutableBuffer {
    addr_t _size;
    unsigned char *_data;
public:
    ExecutableBuffer(addr_t size, const unsigned char *data = nullptr): _size(size), _data(nullptr) {
        addr_t pagesize = (addr_t)sysconf(_SC_PAGE_SIZE);
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
    addr_t size() { return _size; }
    unsigned char *data() { return _data; }
    ~ExecutableBuffer() { if (_data) { free(_data); }}
};

void assertMemAccess(ExecutableBuffer& execBuf, addr_t addr, addr_t size = 0) {
    auto buf_addr = reinterpret_cast<size_t>(execBuf.data());
    auto buf_size = reinterpret_cast<size_t>(execBuf.size());
    assert(addr >= buf_addr);
    assert(addr + size <= buf_addr + buf_size);
}

bool handleRequest(TCPTrans<addr_t>& tr, ExecutableBuffer &execBuf) {
    auto collected = tr.recv();

    if (!collected.size()) {
        std::cerr << "Connection closed by client" << std::endl;
        return false;
    }
    std::cerr << "Got request, len(" << collected.size() << ")" << std::endl;

    auto actionFlags = detail::vecRead<action_int_t>(collected, 0);
    addr_t offset = sizeof(action_int_t);
//    auto action = static_cast<action_t>(actionFlags & 0xFF);
    auto action = static_cast<action_t>(actionFlags);
    switch (action) {
        case ActionEcho: {
            std::cerr << "Request is: ActionEcho:"
                      << " len " << collected.size() - offset
                      << " data: '" << reinterpret_cast<const char*>(collected.data()) + offset << "'"
                      << std::endl;
            std::vector<uint8_t> response{collected.begin() + offset, collected.end()};
            tr.send(response);
            break;
        }
        case ActionGetBuffer: {
            std::cerr << "Request is: ActionGetBuffer:";

            auto cmd = detail::vecRead<cmd_get_buffer_t>(collected, offset);
            offset += sizeof(cmd);

            auto addr0 = reinterpret_cast<size_t>(execBuf.data());
            auto addr = static_cast<addr_t>(addr0);

            std::cerr << " idx " << static_cast<unsigned>(cmd.id)
                      << " size " << cmd.size
                      << " responding with addr 0x" << std::hex << addr
                      << std::dec << std::endl;

            std::vector<uint8_t> response;
            detail::vecAppend(response, addr);
            tr.send(response);
            break;
        }
        case ActionRead: {
            std::cerr << "Request is: ActionRead:";

            auto cmd = detail::vecRead<cmd_read_t>(collected, offset);
            offset += sizeof(cmd);

            std::cerr << std::hex << " addr 0x" << cmd.addr
                      << std::dec << " size " << cmd.size
                      << std::endl;
            assertMemAccess(execBuf, cmd.addr, cmd.size);

            std::vector<uint8_t> response(cmd.size);
            std::copy((uint8_t*)cmd.addr, (uint8_t*)cmd.addr + cmd.size, response.begin());
            tr.send(response);
            break;
        }
        case ActionWrite: {
            std::cerr << "Request is: ActionWrite:";

            auto cmd = detail::vecRead<cmd_write_t>(collected, offset);
            offset += sizeof(cmd);

            std::cerr << std::hex << " addr 0x" << cmd.addr
                      << std::dec << " size " << cmd.size
                      << std::endl;
            assertMemAccess(execBuf, cmd.addr, cmd.size);

            std::copy(collected.begin() + offset, collected.begin() + offset + cmd.size, (uint8_t*)cmd.addr);
            std::vector<uint8_t> response;
            detail::vecAppend(response, cmd.size);
            tr.send(response);
            break;
        }
        case ActionCall: {
            std::cerr << "Request is: ActionCall: ";

            // todo: receive timeout?
            auto cmd = detail::vecRead<cmd_call_t>(collected, offset);
            offset += sizeof(cmd);

            std::cerr << std::hex << "addr 0x" << cmd.addr
                      << std::dec << std::endl;
            assertMemAccess(execBuf, cmd.addr);

            auto fnptr = (void(*)())(cmd.addr);
            fnptr(); // todo: execute in separate thread with timeout?

            std::vector<uint8_t> response;
            detail::vecAppend(response, CallOK);
            tr.send(response);
            break;
        }
        case ActionAddrMmap: {
            std::cerr << "Request is: ActionAddrMmap: ";

            auto cmd = detail::vecRead<cmd_mmap_t>(collected, offset);
            offset += sizeof(cmd);

            std::cerr << std::hex << " addr 0x" << cmd.addr
                      << std::hex << " prot 0x" << cmd.prot
                      << std::dec << std::endl;

            addr_t ptr_val = 0;
            // todo: check sanity of prot; get PROT_X; get according file perm-s for open()
            const auto dev = "/dev/mem";
            int fd = -1;
            if ((fd = open(dev, O_RDWR | O_SYNC)) < 0) {
                std::cerr << "Unable to open " << dev << std::endl;
            }

//            auto ptr = mmap(0, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, cmd.addr);
            auto ptr = mmap(0, getpagesize(), cmd.prot, MAP_SHARED, fd, cmd.addr);
            if (ptr) {
                ptr_val = reinterpret_cast<addr_t>(ptr);
                std::cerr << "mmap returned addr 0x" << std::hex << ptr_val << std::dec << std::endl;
            } else {
                std::cerr << "mmap failed" << std::endl;
            }

            std::vector<uint8_t> response;
            detail::vecAppend(response, ptr_val);
            tr.send(response);
//            if (fd > 0) { close(fd); } // need it?
            break;
        }
        case ActionCall2: {
            std::cerr << "Request is: ActionCall2: ";

            auto cmd = detail::vecRead<cmd_call2_t>(collected, offset);
            offset += sizeof(cmd);

            std::cerr << std::hex << "addr 0x" << cmd.addr
                      << std::dec << "; x1=" << cmd.x1
                      << std::dec << ", x2=" << cmd.x2
                      << "... ";
            auto fnptr = (addr_t(*)(addr_t, addr_t))(cmd.addr);
            addr_t res = fnptr(cmd.x1, cmd.x2);
            std::cerr << std::dec << "result=" << res << std::endl;

            std::vector<uint8_t> response;
            detail::vecAppend(response, res);
            tr.send(response);
            break;
        }
        default: {
            std::cerr << "Request is: unknown - do nothing" << std::endl;
//            std::vector<uint8_t> response;
//            detail::vecAppend(response, 666);
//            tr.send(response);
            break;
        }
    }
    return true;
}

void handleClientConnection(TCPTrans<addr_t>& tr, ExecutableBuffer &execBuf) {
    try {
        while (handleRequest(tr, execBuf));
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
            tcp::socket socket{io_service};
            acceptor.accept(socket);
            TCPTrans<addr_t> tr{std::move(socket)};
            handleClientConnection(tr, *buf);
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
             cxxopts::value<unsigned>()->default_value(std::to_string(1024)))
            ;
    auto result = opts.parse(argc, argv);
    auto buf_size = result["size"].as<unsigned>();
    auto port = result["port"].as<uint16_t>();

    ExecutableBuffer codeBuf(buf_size);
    std::cerr << std::hex
              << "Allocated executable buffer of size 0x" << buf_size
              << " at 0x" << std::hex << reinterpret_cast<size_t>(codeBuf.data())
              << std::dec << std::endl;
    std::thread t1(runServer, &codeBuf, port);
    std::cerr << "Main application loop started at port " << port << "..." << std::endl;
    t1.join();
    return 0;
}
