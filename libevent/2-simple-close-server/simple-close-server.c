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


static void exit_with_sys_error(const char *msg)
{
    fprintf(stderr, "error: %s; %s\n", strerror(errno), msg);
    exit(EXIT_FAILURE);
}

static int set_nonblock(int fd)
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


const char* what_to_string(short what)
{
    switch (what) {
        case EV_TIMEOUT: return "EV_TIMEOUT";
        case EV_READ: return "EV_READ";
        case EV_WRITE: return "EV_WRITE";
        case EV_SIGNAL: return "EV_SIGNAL";
        case EV_PERSIST: return "EV_PERSIST";
        case EV_ET: return "EV_ET";
        default: return "!! UNKNOWN !!";
    }

}

void accept_cb(evutil_socket_t sock, short what, void *arg)
{
    int fd = accept(sock, NULL, NULL);
    close(fd);
    printf("accept_cb: socket %d, what %s, arg(event descr) - %s\n", 
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
        exit_with_sys_error("socket failed");

    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(SERVER_PORT);
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    int enable_reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(int)) < 0)
        exit_with_sys_error("setsockopt failed");

    if (bind(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)))
        exit_with_sys_error("bind failed");

    if (set_nonblock(sock))
        exit_with_sys_error("set_nonblock failed");

    if (listen(sock, SOMAXCONN))
        exit_with_sys_error("listen failed");

    printf("Server is running on port %d.\n"
            "Try to make connections (e.g.: telnet 0 %d).\n",
            (int)SERVER_PORT,
            (int)SERVER_PORT);


    /*========= Creating event_base and adding event (new connection on list. socket) to it ======*/
    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        exit_with_sys_error("event_base_new failed");

    // Keep in mind that events by default are not persist and will become
    // not pending after first appearance. As we have only 1 event,
    // when it triggers, if EV_PERSIST is not set, event_base_dispatch
    // will return as there will be no pending events.
    int what = one_shot ? EV_READ : (EV_READ | EV_PERSIST);
    struct event *ev = event_new(ev_base, sock, what, &accept_cb, "New connection event on list. socket");
    if (!ev)
        exit_with_sys_error("event_new failed");

    if (event_add(ev, NULL))
        exit_with_sys_error("event_add failed");

    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    return 0;
}