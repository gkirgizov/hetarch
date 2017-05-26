#ifndef HETARCH_I2CCONNECTION_H
#define HETARCH_I2CCONNECTION_H

#include <cstdint>
#include <memory>
#include <unistd.h>

#include "trikMspI2c.h"
#include "conn.h"
#include "i2ccommands.h"

namespace hetarch {
namespace conn {

using trikHal::trik::TrikMspI2c;

template<typename AddrT> class I2CConnection: public IConnection<AddrT> {
    std::unique_ptr<trikHal::trik::TrikMspI2c> trikConn;
public:
    I2CConnection(const std::string &devicePath, int deviceId) {
        trikConn = std::unique_ptr<TrikMspI2c>(new TrikMspI2c());
        trikConn->connect(devicePath, deviceId);
    }
    AddrT AddrWrite(AddrT addr, AddrT size, const unsigned char *buf) override {
        trikConn->send({ CMD_SET_CURSOR, 0x00, (uint8_t)(addr & 0xFF), (uint8_t)((addr >> 8) & 0xFF) });
        for (unsigned i = 0; i < size; i += 2) {
            trikConn->send({ CMD_MEM_WRITE, 0x00, buf[i], buf[i + 1] });
        }
        return 0;
    }
    AddrT AddrRead(AddrT addr, AddrT size, unsigned char *buf) override {
        trikConn->send({ CMD_SET_CURSOR, 0x00, (uint8_t)(addr & 0xFF), (uint8_t)((addr >> 8) & 0xFF) });
        for (unsigned i = 0; i < size; i += 2) {
            *((uint16_t*)(buf + i)) = (uint16_t)trikConn->read({ CMD_MEM_READ, 0x00 });
        }
        return 0;
    }
    bool HealthCheck(std::ostream *out = nullptr) {
        const int TOTAL_ITERATIONS = 3;
        int successSum = 0;
        for (int i = 0; i < TOTAL_ITERATIONS; i++) {
            auto res = healthCheckIteration(*trikConn);
            successSum += res;
            if (out) {
                (*out) << "I2CConnection::HealthCheck: iteration #" << i << ", res: " << res << std::endl;
            }
        }
        return successSum > (TOTAL_ITERATIONS / 2);
    }
    int healthCheckIteration(TrikMspI2c &trikConn) {
        const int sleepTimeout = 200;
        trikConn.send({ CMD_HEALTH_CHECK_REQUEST, 0x00, 0x00 });
        usleep(sleepTimeout * 1000);

        int value1 = trikConn.read({CMD_HEALTH_CHECK_GET, 0x00});
        trikConn.send({ CMD_HEALTH_CHECK_REQUEST, 0x00, 0x00 });
        usleep(sleepTimeout * 1000);

        int value2 = trikConn.read({CMD_HEALTH_CHECK_GET, 0x00});
        usleep(sleepTimeout * 1000);

        std::cout << "healthCheckIteration " << value1 << " " << value2 - 1 << std::endl;

        return value2 == (value1 + 1) ? 1 : 0;
    }
    AddrT Call(AddrT addr) {
        std::cout << "I2CConnection::Call addr:" << addr << std::endl;
        trikConn->send({ 0x69, 0x00, (uint8_t)(addr & 0xFF), (uint8_t)((addr >> 8) & 0xFF) });
        // TODO sleep is bad
        usleep(300 * 1000);
    }

    AddrT ReadAnalogSensor(int num) {
        std::cout << "I2CConnection::Call addr:" << num << ": " << (0x66 + num) << std::endl;
        int value = trikConn->read({(uint8_t)(num + 0x66), 0x00});
        return value;
    }

    AddrT GetBuff(unsigned idx) {
        int value = trikConn->read({CMD_GET_DYN_CODE_BUF, 0x00});
        return value;
    }
    unsigned ScheduleRegular() {
        throw "unimplemented";
    }
    void Close() {
        if (trikConn) {
            trikConn->disconnect();
        }
        trikConn.reset();
    }
    virtual ~I2CConnection() override { trikConn->disconnect(); }
};

}
}


#endif //HETARCH_I2CCONNECTION_H
