#ifndef __ADDR_TYPEDEF_H__
#define __ADDR_TYPEDEF_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifndef HETARCH_TARGET_ADDRT
#include <stddef.h>
# define HETARCH_TARGET_ADDRT size_t
#endif
typedef HETARCH_TARGET_ADDRT addr_t;


#ifdef __cplusplus
}
#endif

#endif /* __ADDR_TYPEDEF_H__ */
