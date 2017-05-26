#ifndef HETARCH_EXAMPLES_MSP_UTILS_H
#define HETARCH_EXAMPLES_MSP_UTILS_H 1

#include <vector>
#include <string>
#include <map>
#include "../include/I2CConnection.h"

template<typename Platform>
void healthcheck(hetarch::conn::I2CConnection<Platform> &conn) {
    log("Starting health check...");
    bool checkRes = conn.HealthCheck(&std::cout);
    log(std::string("Health check result: ") + (checkRes ? "true" : "false"));
}

extern std::map<std::string, int> _intrinsics;

inline void initMSP430Intrinsics() {
    _intrinsics["__mulhi3hw_noint"] = 20;
    _intrinsics["__mulsi3"] = 20;
    _intrinsics["__divsi3"] = 20;
}

#endif