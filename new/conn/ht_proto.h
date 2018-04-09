#ifndef __HT_PROTO_H__
#define __HT_PROTO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../addr_typedef.h"


//typedef uint16_t addr_t; // temp
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

typedef struct {
    addr_t size;
} msg_header_t;

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

