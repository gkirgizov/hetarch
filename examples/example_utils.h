#ifndef HETARCH_EXAMPLES_UTILS_H
#define HETARCH_EXAMPLES_UTILS_H 1

#include <vector>
#include <string>

void dumpBufferForDisasm(const std::vector<uint8_t> &buf, std::ostream &out){
    for (int i = 0; i < buf.size(); ++i) {
        int diff = i % 2 == 0 ? 1 : -1;
        //int diff = 0;
        out << std::hex << std::setw(2) << std::setfill('0') << ((unsigned int)(buf[i + diff]));
        out << std::dec;
        if (i % 2 == 1) { out << " "; }
    }
    out << std::endl;
}

void log(const std::string &s) {
    std::cout << s << std::endl;
}

#endif