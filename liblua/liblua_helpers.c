#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "liblua_helpers.h"




void trace_event(int event_trace_level,
                 int add_strerr_info_and_exit,
                 const char* _file,
                 const int line,
                 const char * format,
                 ...)
{
    FILE *s_log_file = stderr;

    va_list va_ap;
    struct tm result;

    int errnum = errno;

    char buf[65536], color_out_buf[65536], out_buf[65536];
    char theDate[32];
    char *file = (char*)_file;
    char extra_msg[100] = {'\0'};
    time_t theTime = time(NULL);

    va_start(va_ap, format);

    memset(buf, 0, sizeof(buf));
    strftime(theDate, 32, "%d/%b/%Y %H:%M:%S", localtime_r(&theTime, &result));
    vsnprintf(buf, sizeof(buf) - 1, format, va_ap);

    if (event_trace_level == TRACE_LEVEL_ERROR) {
        strcat(extra_msg, "ERROR: ");
        if (add_strerr_info_and_exit) {
            strcat(extra_msg, "(");
            strcat(extra_msg, strerror(errnum));
            strcat(extra_msg, ") ");
        }
    }

    while(buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = '\0';

    const char *begin_color_tag = "";
    switch (event_trace_level) {
        case (TRACE_LEVEL_ERROR):
            begin_color_tag = "\x1b[31m";
            break;
        case (TRACE_LEVEL_INFO):
            begin_color_tag = "\x1b[34m";
            break;
    };
    const char *end_color_tag = "\x1b[0m";

    snprintf(color_out_buf, sizeof(color_out_buf), "%s%s [%s:%d]%s %s%s",
                begin_color_tag, theDate, file, line, end_color_tag, extra_msg, buf);
    snprintf(out_buf, sizeof(out_buf), "%s [%s:%d] %s%s", theDate, file, line, extra_msg, buf);

    fprintf(s_log_file, "%s\n", isatty(fileno(s_log_file)) ? color_out_buf : out_buf);
    fflush(s_log_file);

    va_end(va_ap);

    if (add_strerr_info_and_exit)
        exit(EXIT_FAILURE);
}