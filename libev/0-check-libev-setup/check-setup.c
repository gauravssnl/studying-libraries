#include <ev.h>
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
#include <assert.h>

#include "../libev_helpers.h"


#define PROGRAMM_DESCRIPTION \
    "Program to check that libev is installed on the system \n" \
    " and print some general info about it.\n"

#define PROGRAMM_USAGE \
    "Usage: check-setup\n" \

int main()
{
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    printf("\nCurrent libev version: %d.%d\n",
        ev_version_major(), ev_version_minor());

    /* Print supported backends */
    printf("\nBackend      Supported\n");
    printf("---------- --------------\n");
    printf("select       %s\n",
        ev_supported_backends() & EVBACKEND_SELECT ? "yes" : "no");
    printf("poll         %s\n",
        ev_supported_backends() & EVBACKEND_POLL ? "yes" : "no");
    printf("epoll        %s\n",
        ev_supported_backends() & EVBACKEND_EPOLL ? "yes" : "no");
    printf("kqueue       %s\n",
        ev_supported_backends() & EVBACKEND_KQUEUE ? "yes" : "no");
    printf("devpoll      %s\n",
        ev_supported_backends() & EVBACKEND_DEVPOLL ? "yes" : "no");    
    printf("port         %s\n",
        ev_supported_backends() & EVBACKEND_PORT ? "yes" : "no");    

    /* Create default loop and print its backend*/
    struct ev_loop *def_loop = ev_default_loop(0);
    if (!def_loop)
        SYS_LOG_ERROR_AND_EXIT("ev_default_loop failed");
    assert(ev_is_default_loop(def_loop));

    printf("\n\nCreated default libev loop.\n");
    const char *backend = "UNDEFINED";
    switch (ev_backend(def_loop)) {
        case EVBACKEND_SELECT: backend = "select"; break;
        case EVBACKEND_POLL: backend = "poll"; break;
        case EVBACKEND_EPOLL: backend = "epoll"; break;
        case EVBACKEND_KQUEUE: backend = "kqueue"; break;
        case EVBACKEND_DEVPOLL: backend = "devpoll"; break;
        case EVBACKEND_PORT: backend = "port"; break;
    }
    printf("Default loop backend is %s\n", backend);

    ev_loop_destroy(def_loop);
    return 0;
}