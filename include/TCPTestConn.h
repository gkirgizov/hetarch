#include <cstdint>
#include <asio.hpp>

#include "conn.h"

namespace hetarch {
namespace conn {

using asio::ip::tcp;

enum class Actions: uint32_t {
    AddrRead = 1,
    AddrWrite = 2,
    GetBuffAddr = 3,
    Call = 4,
};

class TCPTestConnection: public IConnection<uint64_t> {
    std::unique_ptr<tcp::socket> socket;
public:
    TCPTestConnection(const std::string &host, uint16_t port);
    uint64_t AddrWrite(uint64_t addr, uint64_t size, const unsigned char *buf) override;
    uint64_t AddrRead(uint64_t addr, uint64_t size, unsigned char *buf) override;
    uint32_t Call(uint64_t addr);
    uint64_t GetBuff(unsigned idx);
    unsigned ScheduleRegular() override;
    void Close();
};

std::vector<uint8_t> readBuffer(tcp::socket &socket);
void writeBuffer(tcp::socket &socket, const std::vector<uint8_t> &buffer);

}
}