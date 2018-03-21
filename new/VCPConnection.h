#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include <cassert>
#include <iostream>
#include <string>

#include "Transmission.h"
#include "ht_proto.h"

#define passert(boolean, error_str) \
    if( !(boolean) ) { perror(error_str); assert(false); }

namespace hetarch {
namespace conn {


/// Virtual Com Port Connection
template<typename AddrT>
class VCPConnection : public ITransmission<AddrT> {
    int fd = -1;

	static int init(const char* device, speed_t baud_rate = B115200) {
	  	// Open the Port. We want read/write, no "controlling tty" status, 
	  	// 	and open it no matter what state DCD is in
		std::cerr << "Trying to open " << device << std::endl;
        int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
        if (fd < 0) {
			perror("Failed to open device");
        } else {
            std::cerr << "Opened " << device << " successfully" << std::endl;
        }

		// Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
		fcntl(fd, F_SETFL, 0);
		// Linux Raw serial setup options
		struct termios options;
		tcgetattr(fd, &options);   //Get current serial settings in structure
		cfsetspeed(&options, baud_rate);   //Change a only few
		options.c_cflag &= ~CSTOPB;
		options.c_cflag |= CLOCAL;
		options.c_cflag |= CREAD;
		cfmakeraw(&options);
		tcsetattr(fd, TCSANOW, &options);    //Set serial to new settings

		return fd;
    }

public:
    explicit VCPConnection(const char* device, speed_t baud_rate = B115200)
			: fd{init(device, baud_rate)} {}

    ~VCPConnection() {
		if (fd > 0) {
			tcdrain(fd);
			close(fd);
		}
	}

    std::vector<uint8_t> recv() override {
        uint8_t msg_header_buf[sizeof(msg_header_t)] = { 0 };
        auto header_read = read(fd, msg_header_buf, sizeof(msg_header_t));
        passert(header_read >= 0, "msg header read failed");
        assert(header_read == sizeof(msg_header_t));
        msg_header_t header = *reinterpret_cast<msg_header_t*>(msg_header_buf);

//        uint8_t out_buf[header.size];
        uint8_t out_buf[512];
        auto n_read = read(fd, out_buf, header.size);
        passert(n_read > 0, "data read failed");
        assert(n_read == header.size && "read less bytes than expected!");

        std::vector<uint8_t> vec{out_buf, out_buf + header.size};
        return vec;
    }

private:
    inline AddrT sendImpl(const uint8_t* buf, AddrT size) override {
        msg_header_t cmd_header = { size };
        auto header_written = write(fd, reinterpret_cast<const uint8_t*>(&cmd_header), sizeof(cmd_header));
        passert(header_written >= 0, "msg header write failed");
        assert(header_written == sizeof(cmd_header));

        auto n_written = write(fd, buf, size);
        passert(n_written >= 0, "data write failed");
        assert(n_written == size && "written less bytes than expected");
        return n_written;
    }

};

}
}


