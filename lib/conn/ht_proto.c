#include "conn/ht_proto.h"

#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif


inline msg_header_t get_msg_header(addr_t size) {
    msg_header_t msg = { { 0xDA, 0xDA, 0xDA, 0xDA }, size };
    return msg;
}

inline bool check_magic(const msg_header_t* header) {
    return 0 == strncmp(
            (const char*)magic,
            (const char*)header->magic,
            4
    );
}


#ifdef __cplusplus
}
#endif
