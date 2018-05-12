#ifndef __HT_PROTO_H__
#define __HT_PROTO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../addr_typedef.h"


typedef uint8_t action_int_t;

typedef enum {
    ActionEcho = 10,
    ActionGetBuffer,
    ActionRead,
    ActionWrite,
    ActionCall,
    ActionCall2,
    ActionSchedule,
    ActionAddrMmap = 21
} action_t;


// bytes are supported on any platform (as opposed to single uint32_t)
typedef uint8_t magic_t[4];
static const magic_t magic = { 0xDA, 0xDA, 0xDA, 0xDA };

typedef struct {
    magic_t magic;
    addr_t size;
} msg_header_t;

msg_header_t get_msg_header(addr_t size);
bool check_magic(const msg_header_t* header);


typedef enum {
    ErrRead = -1,
    // ErrWrite = -1,
    ErrCall = -1,
    ErrBuf = 0,
    Err = 0,
    CallOK = 1
} cmd_ret_code_t;


/*typedef struct {
    addr_t size;
} cmd_echo_t;*/

typedef struct {
    uint8_t id;
    addr_t size;
} cmd_get_buffer_t;

typedef struct {
    addr_t addr;
    addr_t size;
} cmd_read_t;

typedef struct {
    addr_t addr;
    addr_t size;
} cmd_write_t;

typedef struct {
    addr_t addr;
} cmd_call_t;

typedef struct {
    addr_t addr;
    addr_t x1;
    addr_t x2;
} cmd_call2_t;

typedef struct {
    addr_t addr;
    addr_t ms_delay;
} cmd_schedule_t;

typedef struct {
    addr_t addr;
    addr_t prot;
} cmd_mmap_t;

#ifdef __cplusplus
}
#endif

#endif /* __HT_PROTO_H__ */

