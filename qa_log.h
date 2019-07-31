#ifndef _QA_LOG_H_
#define _QA_LOG_H_
#include "tfc_debug_log.h"

#define LOG_PRINT(LOG_LEVEL,format,...) \
        DEBUG_P(LOG_LEVEL, "File:%s\tLine:%d\tFuction:%s\t"format, \
        __FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)

#endif
