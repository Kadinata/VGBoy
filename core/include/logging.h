#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>

#define LOG_LEVEL_TRACE ("TRACE")
#define LOG_LEVEL_DEBUG ("DEBUG")
#define LOG_LEVEL_INFO ("INFO ")
#define LOG_LEVEL_WARN ("WARN ")
#define LOG_LEVEL_ERROR ("ERROR")
#define LOG_LEVEL_FATAL ("FATAL")

#define Log_T(...) (Log(LOG_LEVEL_TRACE, __VA_ARGS__))
#define Log_D(...) (Log(LOG_LEVEL_DEBUG, __VA_ARGS__))
#define Log_I(...) (Log(LOG_LEVEL_INFO, __VA_ARGS__))
#define Log_W(...) (Log(LOG_LEVEL_WARN, __VA_ARGS__))
#define Log_E(...) (Log(LOG_LEVEL_ERROR, __VA_ARGS__))
#define Log_F(...) (Log(LOG_LEVEL_FATAL, __VA_ARGS__))

#define LOG_FN(...) (fprintf(stderr, __VA_ARGS__))

#define Log(level, ...) ({LOG_FN("%s [%s:%u]: ", level, __FILE_NAME__, __LINE__); LOG_FN(__VA_ARGS__); LOG_FN("\n"); })

#endif
