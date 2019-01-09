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


// This structure is used so that we can pass somehow event to the callback.
// In libevent v. >= 2.1.1 for this purpose event_self_cbarg is used.
struct event_holder
{
    struct event *ev;
};

#define SERVER_PORT 12345
#define PROGRAMM_DESCRIPTION \
    "Simple echo server written with libevent.\n" \
    " Create event on each connection and wait till there \n" \
    " will be data to read and immediately send them back. We don't create \n" \
    " event to writing data back.\n"

#define PROGRAMM_USAGE \
    "Usage: echo-server\n" \



void read_cb(evutil_socket_t sock, short what, void *arg)
{
    struct event_holder* ev_holder = arg;
    struct event *me = ev_holder->ev;

    /* Read and write are written in a very simplified manner */
    if (what == EV_READ) {
        char msg[129] = {'\0'};
        int n = read((int)sock, &msg, 128);
        if (n == 0) {
            LOG_INFO("Closing connection with socket %d\n", sock);
            goto close_conn_and_del_event;
        } else if (n < 0) {
            LOG_ERROR("Closing connection with socket %d because of read error\n", sock);
            goto close_conn_and_del_event;
        }

        LOG_INFO("Recieved message from socket %d: %s\n", sock, msg);
        int k = write(sock, &msg, n);
        if (k < 0) {
            LOG_ERROR("Closing connection with socket %d because of write error\n", sock);
            goto close_conn_and_del_event;
        }
        return;
    } else {
        SYS_LOG_ERROR_AND_EXIT("unknown event in rw_cb\n");
    }

close_conn_and_del_event:
    close(sock);
    event_del(me);
    free(ev_holder);
    return;
}



void accept_cb(evutil_socket_t sock, short what, void *arg)
{
    struct event_holder* accept_ev_holder = arg;
    struct event *accept_ev = accept_ev_holder->ev;
    struct event_base *ev_base = event_get_base(accept_ev);

    int fd = accept(sock, NULL, NULL);
    if (fd < 0)
        SYS_LOG_ERROR_AND_EXIT("accept failed");

    LOG_INFO("accept_cb: New event: %s; New connection (socket %d)\n", 
        what_to_string(what),
        fd);

    /* Create new event to read data from connection when they will be ready */
    struct event_holder *ev_holder = malloc(sizeof(struct event_holder));
    ev_holder->ev = event_new(ev_base, fd, EV_READ | EV_PERSIST, &read_cb, ev_holder);
    if (!ev_holder->ev)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");
    if (event_add(ev_holder->ev, NULL))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");
}



int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
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
            "Try to make connections and put some data(e.g.: telnet 0 %d).\n",
            (int)SERVER_PORT,
            (int)SERVER_PORT);


    /*========= Creating event_base and adding event (new connection on list. socket) to it ======*/
    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        SYS_LOG_ERROR_AND_EXIT("event_base_new failed");

    struct event_holder *ev_holder = malloc(sizeof(struct event_holder));
    ev_holder->ev = event_new(ev_base, sock, EV_READ | EV_PERSIST, &accept_cb, ev_holder);
    if (!ev_holder->ev)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");

    if (event_add(ev_holder->ev, NULL))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");

    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    return 0;
}