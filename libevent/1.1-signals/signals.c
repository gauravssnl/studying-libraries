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
    "Simple programm to illustrate creation of event base\n" \
    " and adding 2 simple events to it (events are processings of \n" \
    " signals SIGUSR1 and SIGUSR2).\n"

#define PROGRAMM_USAGE \
    "Usage: signals [ONESHOT]\n" \
    " use ONESHOT argument to process signal SIGUSR1 only once (SIGUSR2 is always processed by the app)\n"




void process_signal(evutil_socket_t signal_id, short what, void *arg)
{
    LOG_INFO("process_signal: signal: \"%s\", what: \"%s\", arg(event descr) - %s\n", 
        strsignal(signal_id),
        what_to_string(what),
        (const char*)arg);
}



int main(int argc, char *argv[])
{
    /*========= Parse arguments and print info for the user ======*/
    int one_shot = 0;
    if (argc > 1 && strcmp(argv[1], "ONESHOT") == 0) {
        one_shot = 1;
    }

    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    printf("Current process id %d\n", (int)getpid());
    printf("To see results run: kill -<SIGNAL_NAME> %d\n", (int)getpid());


    /*========= Creating event_base and adding events (signal processing) to it ======*/
    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        SYS_LOG_ERROR_AND_EXIT("event_base_new failed");

    // Keep in mind that events by default are not persist and will become
    // not pending after first appearance. 
    int what = one_shot ? EV_SIGNAL : (EV_SIGNAL | EV_PERSIST);
    struct event *ev1 = event_new(ev_base, SIGUSR1, what, &process_signal, "Signal processing 1");
    if (!ev1)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");
    if (event_add(ev1, NULL))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");

    // Doing the same but use evsignal_new instead of event_new (evsignal_new - is just more
    // convenient wrapper around event_new(ev_base, signal_id, EV_SIGNAL | EV_PERSIST, arg))
    // NB! evsignal_new always setup flag  EV_PERSIST  !!!
    struct event *ev2 = evsignal_new(ev_base, SIGUSR2, &process_signal, "Signal processing 2");
    if (!ev2)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");
    if (event_add(ev2, NULL))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");

    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    return 0;
}