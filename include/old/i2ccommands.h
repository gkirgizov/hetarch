#ifndef _HETARCH_I2C_COMMANDS_H
#define _HETARCH_I2C_COMMANDS_H 1

#define RW_MEM_BUF_SIZE 2

#define CMD_SET_CURSOR 0x63     // set current memory address
#define CMD_MEM_WRITE 0x64      // write bytes to memory
#define CMD_EXEC_SINGLE 0x65    // schedule single execution
#define CMD_EXEC_REGULAR 0x66   // schedule regular execution
#define CMD_REMOVE_REGULAR 0x67 // stop regular function execution
#define CMD_HEALTH_CHECK_REQUEST 0x68

#define CMD_GET_DYN_CODE_BUF2  0x58 // GET DYNAMIC CODE BUF ADDR 2
#define CMD_GET_CURSOR         0x59 // GET rw_mem_addr
#define CMD_GET_DYN_CODE_BUF   0x60 // GET DYNAMIC CODE BUF ADDR
#define CMD_GET_PC             0x61 // GET PC
#define CMD_MEM_READ           0x62 // read memory
#define CMD_HEALTH_CHECK_GET   0x63

#endif