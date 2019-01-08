#include <event2/event.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <linux/limits.h>
#include <stdlib.h>

#include "libevent_helpers.h"

const char* what_to_string(short what)
{
    int unknown_what = 1;
    static char str[256];
    str[0] = '\0';
    strcat(str, "(");
    if (what & EV_TIMEOUT) {
        strcat(str, "EV_TIMEOUT ");
        unknown_what = 0;
    } 
    if (what & EV_READ) {
        strcat(str, "EV_READ ");
        unknown_what = 0;
    } 

    if (what & EV_WRITE) {
        strcat(str, "EV_WRITE ");
        unknown_what = 0;
    } 

    if (what & EV_SIGNAL) {
        strcat(str, "EV_SIGNAL ");
        unknown_what = 0;
    } 

    if (what & EV_PERSIST) {
        strcat(str, "EV_PERSIST ");
        unknown_what = 0;
    } 

    if (what & EV_ET) {
        strcat(str, "EV_ET ");
        unknown_what = 0;
    } 

    if (unknown_what)
        strcat(str, "UNKNOWN");


    strcat(str, ")");

    return str;
}

int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}



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