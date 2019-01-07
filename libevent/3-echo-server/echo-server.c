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

// This structure is used so that we can pass somehow event to the callback.
// In libevent v. >= 2.1.1 for this purpose event_self_cbarg is used.
struct event_holder
{
    struct event *ev;
};

void *event_self_cbarg();



#define SERVER_PORT 12345
#define PROGRAMM_DESCRIPTION \
    "Simple echo server written with libevent.\n" \
    " Create event on each connection and wait till there \n" \
    " will be data to read and immediately send them back. We don't create \n" \
    " event to writing data back.\n"

#define PROGRAMM_USAGE \
    "Usage: echo-server\n" \


static void exit_with_error(const char *msg)
{
    fprintf(stderr, "error: %s\n", msg);
    exit(EXIT_FAILURE);
}

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


void read_cb(evutil_socket_t sock, short what, void *arg)
{
    struct event_holder* ev_holder = arg;
    struct event *me = ev_holder->ev;

    /* Read and write are written in a very simplified manner */
    if (what == EV_READ) {
        char msg[129] = {'\0'};
        int n = read((int)sock, &msg, 128);
        if (n == 0) {
            printf("Closing connection with socket %d\n", sock);
            goto close_conn_and_del_event;
        } else if (n < 0) {
            printf("Closing connection with socket %d because of read error\n", sock);
            goto close_conn_and_del_event;
        }

        printf("Recieved message from socket %d: %s\n", sock, msg);
        int k = write(sock, &msg, n);
        if (k < 0) {
            printf("Closing connection with socket %d because of write error\n", sock);
            goto close_conn_and_del_event;
        }
        return;
    } else {
        exit_with_error("unknown event in rw_cb\n");
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
        exit_with_sys_error("accept failed");

    printf("accept_cb: New event: %s; New connection (socket %d)\n", 
        what_to_string(what),
        fd);

    /* Create new event to read data from connection when they will be ready */
    struct event_holder *ev_holder = malloc(sizeof(struct event_holder));
    ev_holder->ev = event_new(ev_base, fd, EV_READ | EV_PERSIST, &read_cb, ev_holder);
    if (!ev_holder->ev)
        exit_with_sys_error("event_new failed");
    if (event_add(ev_holder->ev, NULL))
        exit_with_sys_error("event_add failed");
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
            "Try to make connections and put some data(e.g.: telnet 0 %d).\n",
            (int)SERVER_PORT,
            (int)SERVER_PORT);


    /*========= Creating event_base and adding event (new connection on list. socket) to it ======*/
    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        exit_with_sys_error("event_base_new failed");

    struct event_holder *ev_holder = malloc(sizeof(struct event_holder));
    ev_holder->ev = event_new(ev_base, sock, EV_READ | EV_PERSIST, &accept_cb, ev_holder);
    if (!ev_holder->ev)
        exit_with_sys_error("event_new failed");

    if (event_add(ev_holder->ev, NULL))
        exit_with_sys_error("event_add failed");

    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    return 0;
}