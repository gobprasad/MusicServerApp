#ifndef __LOGGING_FRAMEWORK__
#define __LOGGING_FRAMEWORK__
#include <syslog.h>

#ifdef NDEBUG
#define LOG_MSG(...)
#define LOG_ERROR(...)
#define LOG_WARN(...)
#else
#define LOG_MSG(M,...)	syslog(LOG_INFO,"[ %s:%d ]: " M ,__FUNCTION__,__LINE__,##__VA_ARGS__);
#define LOG_ERROR(M,...)	syslog(LOG_ERR,"[ %s:%d ]: " M ,__FUNCTION__,__LINE__,##__VA_ARGS__);
#define LOG_WARN(M,...)	syslog(LOG_WARNING,"[ %s:%d ]: " M ,__FUNCTION__,__LINE__,##__VA_ARGS__);

#endif

#endif
