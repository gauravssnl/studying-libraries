#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

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
    "Echo server written with libevent.\n" \
    " Echo server uses libevent's bufferevents and evbuffers which \n" \
    " significantly ease work with sockets and buffers \n" \
    " while reading and writing data to socket."

#define PROGRAMM_USAGE \
    "Usage: echo-server\n" \


void readcb(struct bufferevent *bev, void *ptr)
{
    (void)ptr;
    char buf[1024];
    int n;
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        LOG_INFO("Read %d bytes from bufferevent %p, sending them back\n", n, bev);
        evbuffer_add(output, buf, n);
    }
}

void eventcb(struct bufferevent *bev, short events, void *ptr)
{
    if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
         LOG_ERROR("Closing bufferevent %p (event: %s:%s)\n",
            bev,
            (events & BEV_EVENT_ERROR) ? "BEV_EVENT_ERROR" : "",
            (events & BEV_EVENT_EOF) ? "BEV_EVENT_EOF" : ""
            );
         bufferevent_free(bev);
    } else {
        LOG_ERROR("Unknown event %d\n", (int)events);
        LOG_ERROR("Closing bufferevent %p\n", bev);
        bufferevent_free(bev);
    }
}


void accept_cb(struct evconnlistener *listener, evutil_socket_t new_sock,
    struct sockaddr *addr, int len, void *arg)
{
    (void)addr;
    (void)len;
    struct event_base *ev_base = evconnlistener_get_base(listener);

    LOG_INFO("accept_cb: new socket %d, description - %s\n", 
        new_sock,
        (const char*)arg);
    if (evutil_make_socket_nonblocking(new_sock))
        SYS_LOG_ERROR_AND_EXIT("evutil_make_socket_nonblocking failed");

    struct bufferevent *bev = bufferevent_socket_new(ev_base, new_sock, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, readcb, NULL, eventcb, ev_base);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    LOG_INFO("bufferevent %p was created\n", bev);
}


int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    /*========= Creating event_base and  add listening socket to it ======*/
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