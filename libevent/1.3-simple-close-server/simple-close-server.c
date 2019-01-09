#include <event2/event.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

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
    " and adding simple event to it (event is based on listening\n" \
    " tcp socket, when new connection is established event is\n" \
    " triggered, message is printed and connection is immediately\n" \
    " closed by the server).\n"

#define PROGRAMM_USAGE \
    "Usage: simple-close-server [ONESHOT]\n" \
    " use ONESHOT argument to exit after first connection\n"


void accept_cb(evutil_socket_t sock, short what, void *arg)
{
    int fd = accept(sock, NULL, NULL);
    close(fd);
    LOG_INFO("accept_cb: socket %d, what %s, arg(event descr) - %s\n", 
        sock,
        what_to_string(what),
        (const char*)arg);

}


int main(int argc, char *argv[])
{
    int one_shot = 0;
    if (argc > 1 && strcmp(argv[1], "ONESHOT") == 0) {
        one_shot = 1;
    }

    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    /*============  Starting server ================*/
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 1)
        SYS_LOG_ERROR_AND_EXIT("socket failed");

    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(SERVER_PORT);
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    int enable_reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(int)) < 0)
        SYS_LOG_ERROR_AND_EXIT("setsockopt failed");

    if (bind(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)))
        SYS_LOG_ERROR_AND_EXIT("bind failed");

    if (set_nonblock(sock))
        SYS_LOG_ERROR_AND_EXIT("set_nonblock failed");

    if (listen(sock, SOMAXCONN))
        SYS_LOG_ERROR_AND_EXIT("listen failed");

    printf("Server is running on port %d.\n"
            "Try to make connections (e.g.: telnet 0 %d).\n",
            (int)SERVER_PORT,
            (int)SERVER_PORT);


    /*========= Creating event_base and adding event (new connection on list. socket) to it ======*/
    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        SYS_LOG_ERROR_AND_EXIT("event_base_new failed");

    // Keep in mind that events by default are not persist and will become
    // not pending after first appearance. As we have only 1 event,
    // when it triggers, if EV_PERSIST is not set, event_base_dispatch
    // will return as there will be no pending events.
    int what = one_shot ? EV_READ : (EV_READ | EV_PERSIST);
    struct event *ev = event_new(ev_base, sock, what, &accept_cb, "New connection event on list. socket");
    if (!ev)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");

    if (event_add(ev, NULL))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");

    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    return 0;
}