#ifndef LIBLUA_HELPERS_H
#define LIBLUA_HELPERS_H

/* Logger stuff */
#define TRACE_SYSTEM_ERROR    TRACE_LEVEL_ERROR, 1, EXTRA_INFO
#define TRACE_LEVEL_ERROR     0
#define TRACE_LEVEL_INFO      1

#define EXTRA_INFO __FILE__, __LINE__

#define TRACE_INFO            TRACE_LEVEL_INFO, 0, EXTRA_INFO
#define TRACE_ERROR           TRACE_LEVEL_ERROR, 0, EXTRA_INFO

void trace_event(int event_trace_level,
                 int add_strerr_info_and_exit,
                 const char* file,
                 const int line,
                 const char * format,
                 ...);

#define SYS_LOG_ERROR_AND_EXIT(...)  trace_event(TRACE_SYSTEM_ERROR  , __VA_ARGS__)
#define LOG_ERROR(...)               trace_event(TRACE_ERROR         , __VA_ARGS__)
#define LOG_INFO(...)		         trace_event(TRACE_INFO          , __VA_ARGS__)

#endif /* LIBLUA_HELPERS_H */
