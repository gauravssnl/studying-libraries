#include <event2/event.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>
#include <fcntl.h>

#include "../libevent_helpers.h"


#define SERVER_PORT 12345
#define PROGRAMM_DESCRIPTION \
    "Simple programm that sets 2 timers. \n" \
    " First timer is  persist. It will print 10 messages each second and then will be removed from the event loop.\n" \
    " Second timer will print one message after 5 seconds." \

#define PROGRAMM_USAGE \
    "Usage: timer\n" 

int one_sec_timer_n = 10;

struct timer
{
    struct timeval timeout;
    const char *description;
    struct event *timer_event;
};

void process_timeout(evutil_socket_t fd, short what, void *arg)
{
    struct timer *t = arg;

    LOG_INFO("process_timeout: fd: %d, what: \"%s\", timer description - %s\n", 
        fd,
        what_to_string(what),
        t->description);

    /* If it is 1 sec. timer. */
    if (t->timeout.tv_sec == 1) {
        --one_sec_timer_n;
        /* Remove 1 second timer from the event queue */
        if (one_sec_timer_n == 0) {
            event_del(t->timer_event);
        }
    }
}

int main()
{
    /*========= Parse arguments and print info for the user ======*/
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    /*========= Creating event_base and adding events (timers) to it ======*/
    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        SYS_LOG_ERROR_AND_EXIT("event_base_new failed");

    struct timer one_sec_timer;
    one_sec_timer.timeout.tv_sec =  1;
    one_sec_timer.timeout.tv_usec =  0;
    one_sec_timer.description = "Persistent 1 second timer";

    struct timer five_sec_timer;
    five_sec_timer.timeout.tv_sec =  5;
    five_sec_timer.timeout.tv_usec =  0;
    five_sec_timer.description = "Oneshot 5 second timer";

     //Actually there is no need to set EV_TIMEOUT in event_new - it will be ignored anyway.
    one_sec_timer.timer_event = event_new(ev_base, -1, EV_TIMEOUT | EV_PERSIST, &process_timeout, &one_sec_timer);
    if (!one_sec_timer.timer_event)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");
    if (event_add(one_sec_timer.timer_event, &one_sec_timer.timeout))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");

    five_sec_timer.timer_event = event_new(ev_base, -1, EV_TIMEOUT , &process_timeout, &five_sec_timer);
    if (!five_sec_timer.timer_event)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");
    if (event_add(five_sec_timer.timer_event, &five_sec_timer.timeout))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");

    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    return 0;
}