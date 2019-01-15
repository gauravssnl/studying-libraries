#include <event2/event.h>
#include <event2/listener.h>

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
    " and adding listening socket to it. System functions are not used.\n" \
    " Everything is done via libevent."

#define PROGRAMM_USAGE \
    "Usage: simple-close-server [ONESHOT]\n" \
    " use ONESHOT argument to exit after first connection\n"


int is_one_shot = 0;

void accept_cb(struct evconnlistener *listener, evutil_socket_t new_sock,
    struct sockaddr *addr, int len, void *arg)
{
    (void)listener;
    (void)addr;
    (void)len;
    LOG_INFO("accept_cb: new socket %d, description - %s\n", 
        new_sock,
        (const char*)arg);
    LOG_INFO(" closing connection on socket %d\n", new_sock);
    close(new_sock);

    if (is_one_shot) {
        LOG_INFO("Exiting loopevent after first connection\n");
        struct event_base *base = evconnlistener_get_base(listener);
        event_base_loopexit(base, NULL);
    }
}


int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "ONESHOT") == 0) {
        is_one_shot = 1;
    }

    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    /*========= Creating event_base and listening socket to it ======*/
    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        SYS_LOG_ERROR_AND_EXIT("event_base_new failed");

    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(SERVER_PORT);
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener *listener;
    listener = evconnlistener_new_bind(ev_base,
        accept_cb, "Event on list. socket",  LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE ,
        SOMAXCONN, (struct sockaddr*)&addr_in, sizeof(addr_in));

    printf("Server is running on port %d.\n"
        "Try to make connections (e.g.: telnet 0 %d).\n",
        (int)SERVER_PORT,
        (int)SERVER_PORT);

    if (!listener)
        SYS_LOG_ERROR_AND_EXIT("evconnlistener_new_bind failed");

    event_base_dispatch(ev_base);
    evconnlistener_free(listener);
    event_base_free(ev_base);
    return 0;
}