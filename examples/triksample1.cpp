#include <iostream>
#include <string>
#include "../include/I2CConnection.h"

typedef HETARCH_TARGET_ADDRT Platform;

void log(const std::string &s) {
    std::cout << s << std::endl;
}

int main(int argc, char **argv) {
    log("Start...");
    log("Creating connection...");
    auto conn = std::unique_ptr<hetarch::conn::I2CConnection<Platform> >(
            new hetarch::conn::I2CConnection<Platform>("/dev/i2c-2", 0x48));
    log("");
    log("Connection created!");

    log("Starting health check...");
    bool checkRes = conn->HealthCheck(&std::cout);
    log(std::string("Health check result: ") + (checkRes ? "true" : "false"));
    return 0;
}